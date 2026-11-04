#include <filesystem>
#include <fstream>
#include <memory>
#include <stdexcept>

#include "IPlugin.h"
#include "NetworkManager.h"
#include "PathResolver.h"
#include "PluginAPI.h"
#include "PluginContext.h"

namespace {
void Trace(const char* Event) {
  std::ofstream(PathResolver::GetExecutableDirectory() / "network-lifecycle.txt", std::ios::app)
      << Event << '\n';
}

struct FFactoryLifetime {
  ~FFactoryLifetime() { Trace("factory destroyed"); }
};

class FTestTransport final : public INetworkTransport {
 public:
  ~FTestTransport() override { Trace("transport destroyed"); }
  bool StartHost(uint16_t Port, size_t MaxConnections, size_t ChannelCount) override {
    Running = true;
    return true;
  }
  bool Connect(
      const FNetworkEndpoint& Endpoint, size_t ChannelCount, FNetworkPeerId& OutPeerId
  ) override {
    return false;
  }
  bool Service() override { return false; }
  bool Send(
      FNetworkPeerId PeerId,
      uint8_t Channel,
      ENetPacketReliability Reliability,
      const uint8_t* Data,
      size_t Size
  ) override {
    return false;
  }
  bool Broadcast(
      uint8_t Channel, ENetPacketReliability Reliability, const uint8_t* Data, size_t Size
  ) override {
    return false;
  }
  void Disconnect(FNetworkPeerId PeerId) override {}
  void Stop() override {
    Running = false;
    Trace("transport stopped");
  }
  bool IsRunning() const override { return Running; }
  void SetConnectedCallback(ConnectedCallback Callback) override {}
  void SetDisconnectedCallback(DisconnectedCallback Callback) override {}
  void SetConnectionStateChangedCallback(ConnectionStateChangedCallback Callback) override {}
  void SetPacketReceivedCallback(PacketReceivedCallback Callback) override {}

 private:
  bool Running = false;
};

class FLifecyclePlugin final : public IPlugin {
 public:
  const char* GetName() const override {
#if defined(TEST_BAD_API)
    return "BadVersionPlugin";
#elif defined(TEST_FAIL_LOAD)
    return "ZFailurePlugin";
#elif defined(TEST_WRONG_NAME)
    return "DifferentPlugin";
#else
    return "LifecyclePlugin";
#endif
  }
  uint32_t GetApiVersion() const override {
#ifdef TEST_BAD_API
    return BROCCOLI_PLUGIN_API_VERSION + 1;
#else
    return BROCCOLI_PLUGIN_API_VERSION;
#endif
  }
  bool OnLoad(PluginContext& Context) override {
#ifdef TEST_FAIL_LOAD
    Context.RegisterNetworkTransport("FailedLoad", [] {
      return std::make_unique<FTestTransport>();
    });
    return false;
#else
    return Context.RegisterNetworkTransport(
        "Lifecycle", [Lifetime = std::make_shared<FFactoryLifetime>()] {
          return std::make_unique<FTestTransport>();
        }
    );
#endif
  }
  void OnUpdate(float DeltaTime) override { throw std::runtime_error("Expected test exception"); }
  void OnUnload() override { Trace("plugin unloaded"); }
};
}  // namespace

extern "C" __declspec(dllexport) IPlugin* CreateBroccoliPlugin() { return new FLifecyclePlugin(); }
extern "C" __declspec(dllexport) void DestroyBroccoliPlugin(IPlugin* Plugin) { delete Plugin; }
