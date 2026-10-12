#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <thread>

#include "BEPlugins.h"
#include "Log.h"
#include "NetPacketType.h"
#include "NetworkManager.h"
#include "NetworkTransportRegistry.h"
#include "PluginHost.h"
#ifdef TEST_EOS_PLUGIN
#include <EOSPlugin/EOSPlugin.h>
#endif

namespace {
void Check(bool Result, const char* Message) {
  if (!Result) throw std::runtime_error(Message);
}

void TestRegistry() {
  auto& Registry = NetworkTransportRegistry::GetInstance();
  Check(Registry.IsRegistered("ENet"), "Built-in ENet missing");
  Check(!Registry.IsRegistered("enet"), "Names must be case sensitive");
  Check(!Registry.Create("Unknown"), "Unknown transport must fail");
  Check(!Registry.RegisterOwned("", [] { return nullptr; }, "Test"), "Empty name accepted");
  Check(!Registry.RegisterOwned("Test", {}, "Test"), "Empty factory accepted");
  Check(!Registry.RegisterOwned("Test", [] { return nullptr; }, ""), "Empty owner accepted");
  Check(
      Registry.RegisterOwned(
          "Test", [&Registry] { return Registry.Create("ENet"); }, "Test"
      ),
      "Registration failed"
  );
  Check(!Registry.RegisterOwned("Test", [] { return nullptr; }, "Other"), "Duplicate accepted");
  auto Transport = Registry.Create("Test");
  Check(Transport != nullptr, "Factory creation failed");
  Transport.reset();
  Registry.UnregisterModule("Test");
  Check(!Registry.IsRegistered("Test") && Registry.IsRegistered("ENet"), "Owner cleanup failed");
  auto& Network = NetworkManager::GetInstance();
  Check(Network.GetTransportName() == "ENet", "Default transport changed");
  Check(
      !Network.SetTransportName("") && !Network.SetTransportName("Unknown"),
      "Invalid selection accepted"
  );
}

void TestENet() {
  auto& Registry = NetworkTransportRegistry::GetInstance();
  auto Host = Registry.Create("ENet");
  auto Client = Registry.Create("ENet");
  bool HostConnected = false;
  bool ClientConnected = false;
  bool HostReceived = false;
  bool ClientReceived = false;
  FNetworkPeerId HostPeer = 0;
  Host->SetConnectedCallback([&](FNetworkPeerId Peer) {
    HostPeer = Peer;
    HostConnected = true;
  });
  Client->SetConnectedCallback([&](FNetworkPeerId Peer) { ClientConnected = true; });
  const std::vector<uint8_t> Payload{1, 2, 3, 4};
  Host->SetPacketReceivedCallback([&](FReceivedPacket&& Packet) {
    HostReceived = Packet.Data == Payload;
  });
  Client->SetPacketReceivedCallback([&](FReceivedPacket&& Packet) {
    ClientReceived = Packet.Data == Payload;
  });
  uint16_t Port = 39770;
  while (Port < 39800 && !Host->StartHost(Port, 4, 2)) ++Port;
  Check(Host->IsRunning(), "Host start failed");
  FNetworkPeerId ClientPeer = 0;
  Check(Client->Connect({"127.0.0.1", Port}, 2, ClientPeer), "Client connect failed");
  const auto Deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
  while ((!HostConnected || !ClientConnected) && std::chrono::steady_clock::now() < Deadline) {
    Host->Service();
    Client->Service();
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
  Check(HostConnected && ClientConnected, "ENet handshake timeout");
  Check(
      Client->Send(ClientPeer, 0, ENetPacketReliability::Reliable, Payload.data(), Payload.size()),
      "Client send failed"
  );
  Check(
      Host->Send(HostPeer, 0, ENetPacketReliability::Reliable, Payload.data(), Payload.size()),
      "Host send failed"
  );
  while ((!HostReceived || !ClientReceived) && std::chrono::steady_clock::now() < Deadline) {
    Host->Service();
    Client->Service();
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
  Check(HostReceived && ClientReceived, "Reliable bidirectional packet failed");
  Client->Stop();
  Host->Stop();
  auto& Network = NetworkManager::GetInstance();
  Check(Network.StartServer(Port), "NetworkManager ENet start failed");
  Check(!Network.SetTransportName("ENet"), "Running transport selection accepted");
  FNetworkConnectionId Connection = 0;
  const auto ConnectedHandle =
      Network.AddOnConnected([&](FNetworkConnectionId Id) { Connection = Id; });
  ClientConnected = false;
  Check(Client->Connect({"127.0.0.1", Port}, 2, ClientPeer), "Manager client connect failed");
  const auto ManagerDeadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
  auto Pump = [&] {
    Network.Service();
    Client->Service();
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  };
  while ((!Connection || !ClientConnected) && std::chrono::steady_clock::now() < ManagerDeadline)
    Pump();
  Check(Connection && ClientConnected, "Manager handshake failed");
  bool ManagerReceived = false;
  std::vector<uint8_t> ExpectedMessage;
  const auto PacketHandle =
      Network.AddOnPacketReceived([&](FNetworkConnectionId Id, FNetBuffer& Buffer) {
        ManagerReceived = Id == Connection && Buffer.GetData() == ExpectedMessage;
      });
  Client->SetPacketReceivedCallback([&](FReceivedPacket&& Packet) {
    ClientReceived = Packet.Data == ExpectedMessage;
  });
  for (const auto Type :
       {ENetPacketType::ActorState, ENetPacketType::ActorRPC, ENetPacketType::ServerTravel}) {
    FNetBuffer Message;
    Message.Write(Type);
    Message.Write(uint32_t{42});
    Message.WriteString("network packet round trip");
    ExpectedMessage = Message.GetData();
    ManagerReceived = ClientReceived = false;
    Check(
        Client->Send(
            ClientPeer, 0, ENetPacketReliability::Reliable, Message.Data(), Message.Size()
        ),
        "Manager packet send failed"
    );
    while (!ManagerReceived && std::chrono::steady_clock::now() < ManagerDeadline) Pump();
    Check(ManagerReceived, "Manager changed the upper-layer packet bytes");
    Check(Network.SendToClient(Connection, Message), "Manager reply failed");
    while (!ClientReceived && std::chrono::steady_clock::now() < ManagerDeadline) Pump();
    Check(ClientReceived, "Manager reply packet changed");
  }
  bool TimeResponseReceived = false;
  Client->SetPacketReceivedCallback([&](FReceivedPacket&& Packet) {
    FNetBuffer Response(std::move(Packet.Data));
    ENetPacketType Type = ENetPacketType::None;
    double EchoTime = 0.0;
    double ServerTime = 0.0;
    TimeResponseReceived = Response.Read(Type) && Type == ENetPacketType::ServerTimeResponse &&
                           Response.Read(EchoTime) && EchoTime == 123.5 &&
                           Response.Read(ServerTime) && ServerTime > 0.0;
  });
  FNetBuffer TimeRequest;
  TimeRequest.Write(ENetPacketType::ServerTimeRequest);
  TimeRequest.Write(123.5);
  Check(
      Client->Send(
          ClientPeer, 0, ENetPacketReliability::Reliable, TimeRequest.Data(), TimeRequest.Size()
      ),
      "Server time request failed"
  );
  while (!TimeResponseReceived && std::chrono::steady_clock::now() < ManagerDeadline) Pump();
  Check(TimeResponseReceived, "Server time response format changed");
  Network.RemoveOnConnected(ConnectedHandle);
  Network.RemoveOnPacketReceived(PacketHandle);
  Network.Stop();
  Client->Stop();
}

void TestPluginConfig(const std::filesystem::path& Directory) {
  auto& Host = PluginHost::GetInstance();
  const auto Config = Directory / "network-tests-plugins.json";
  Check(!Host.Initialize(Directory / "missing-plugins.json"), "Missing config accepted");
  for (const char* Text :
       {R"({"schema_version":2,"required_plugins":[]})",
        R"({"schema_version":true,"required_plugins":[]})",
        R"({"schema_version":1,"required_plugins":["../Bad"]})",
        R"({"schema_version":1,"required_plugins":["ExamplePlugin","ExamplePlugin"]})",
        R"({"schema_version":1,"required_plugins":["MissingPlugin"]})",
        R"({"schema_version":1,"required_plugins":["BadVersionPlugin"]})",
        R"({"schema_version":1,"required_plugins":["WrongNamePlugin"]})",
        R"({"schema_version":1,"required_plugins":["LifecyclePlugin","ZFailurePlugin"]})",
        "invalid json"}) {
    std::ofstream(Config) << Text;
    Check(!Host.Initialize(Config), "Invalid config accepted");
    Check(
        !Host.IsInitialized() && Host.GetActivePluginCount() == 0, "Failed init was not rolled back"
    );
    Check(
        !NetworkTransportRegistry::GetInstance().IsRegistered("Lifecycle") &&
            !NetworkTransportRegistry::GetInstance().IsRegistered("FailedLoad"),
        "Failed initialization leaked a factory"
    );
  }
  std::ofstream(Config) << R"({"schema_version":1,"required_plugins":[]})";
  Check(Host.Initialize(Config), "Core-only plugin init failed");
  Check(!BEPlugins::IsAvailable("EOSPlugin"), "Unloaded plugin available");
  Host.Shutdown();
  std::filesystem::remove(Config);
}

void TestPluginTransportLifecycle(const std::filesystem::path& Directory) {
  const auto Config = Directory / "network-tests-plugins.json";
  const auto TracePath = Directory / "network-lifecycle.txt";
  std::filesystem::remove(TracePath);
  std::ofstream(Config) << R"({"schema_version":1,"required_plugins":["LifecyclePlugin"]})";
  auto& Host = PluginHost::GetInstance();
  auto& Network = NetworkManager::GetInstance();
  auto& Registry = NetworkTransportRegistry::GetInstance();
  Check(Host.Initialize(Config), "Lifecycle plugin init failed");
  Check(
      Network.SetTransportName("Lifecycle") && Network.StartServer(0),
      "Plugin transport start failed"
  );
  Host.Update(0.01f);
  Check(!Host.IsPluginActive("LifecyclePlugin"), "Throwing plugin stayed Active");
  Check(
      Registry.IsRegistered("Lifecycle") && Network.IsRunning(),
      "Throwing plugin unloaded before shutdown"
  );
  Check(!std::filesystem::exists(TracePath), "Plugin was deactivated on update exception");
  Network.Stop();
  Host.Shutdown();
  Check(!Registry.IsRegistered("Lifecycle"), "Factory survived plugin unload");
  std::ifstream TraceFile(TracePath);
  const std::string Events{
      std::istreambuf_iterator<char>(TraceFile), std::istreambuf_iterator<char>()
  };
  Check(
      Events == "transport stopped\ntransport destroyed\nplugin unloaded\nfactory destroyed\n",
      "Transport/factory/plugin destruction order changed"
  );
  TraceFile.close();
  Check(Network.SetTransportName("ENet"), "ENet unavailable after plugin unload");
  std::filesystem::remove(Config);
  std::filesystem::remove(TracePath);
}

#ifdef TEST_EOS_PLUGIN
void TestEOSLogin(const std::filesystem::path& Directory) {
  auto& Host = PluginHost::GetInstance();
  Check(Host.Initialize(Directory / "plugins.json"), "EOS plugin initialization failed");
  Check(BEPlugins::IsAvailable("EOSPlugin"), "EOS plugin is not Active");
  Check(NetworkManager::GetInstance().GetTransportName() == "ENet", "Plugin load changed default");
  Check(
      NetworkTransportRegistry::GetInstance().IsRegistered("EOSP2P"), "EOS transport not registered"
  );
#ifdef TEST_EOS_NO_CREDENTIALS
  Check(!BEPlugins::EOS::IsEOSInitialized(), "EOS unexpectedly initialized without credentials");
#else
  Check(BEPlugins::EOS::IsEOSInitialized(), "EOS platform not initialized");
#endif
  bool Completed = false;
  bool Success = false;
  BEPlugins::EOS::Login(
      "BroccoliNetworkTests", [&](const BEPlugins::EOS::FOnlinePlayResult& Result) {
        Completed = true;
        Success = Result.Success;
        std::cout << "Device ID login: " << Result.Message << '\n';
      }
  );
  const auto Deadline = std::chrono::steady_clock::now() + std::chrono::seconds(60);
  while (!Completed && std::chrono::steady_clock::now() < Deadline) {
    Host.Update(0.01f);
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
#ifdef TEST_EOS_NO_CREDENTIALS
  Check(Completed && !Success, "EOS login did not reject missing initialization");
#else
  Check(Completed && Success, "Device ID login failed or timed out");
#endif
  NetworkManager::GetInstance().Stop();
  Host.Shutdown();
  Check(
      !NetworkTransportRegistry::GetInstance().IsRegistered("EOSP2P"), "EOS factory survived unload"
  );
}
#endif
}  // namespace

int main(int ArgumentCount, char** Arguments) {
  try {
    const auto Directory = std::filesystem::absolute(Arguments[0]).parent_path();
    TestRegistry();
    TestENet();
    TestPluginConfig(Directory);
    TestPluginTransportLifecycle(Directory);
#ifdef TEST_EOS_PLUGIN
    TestEOSLogin(Directory);
#endif
    std::cout << "Network tests passed\n";
    MLog::Shutdown();
    return 0;
  } catch (const std::exception& Exception) {
    std::cerr << Exception.what() << '\n';
    NetworkManager::GetInstance().Stop();
    PluginHost::GetInstance().Shutdown();
    MLog::Shutdown();
    return 1;
  }
}
