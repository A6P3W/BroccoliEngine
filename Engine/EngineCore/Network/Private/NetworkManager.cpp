#include "NetworkManager.h"

#include <algorithm>
#include <cstdlib>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "Log.h"
#include "NetPacketType.h"

namespace {
const char* GetPacketTypeName(ENetPacketType PacketType) {
  switch (PacketType) {
    case ENetPacketType::AssignNetId:
      return "AssignNetId";
    case ENetPacketType::ActorSpawn:
      return "ActorSpawn";
    case ENetPacketType::ActorState:
      return "ActorState";
    case ENetPacketType::ActorDestroy:
      return "ActorDestroy";
    case ENetPacketType::ActorRPC:
      return "ActorRPC";
    case ENetPacketType::ServerTravel:
      return "ServerTravel";
    case ENetPacketType::ClientTravelReady:
      return "ClientTravelReady";
    case ENetPacketType::None:
    default:
      return "None";
  }
}

const char* GetTransportTypeName(ENetworkTransportType Type) {
  return Type == ENetworkTransportType::EOSP2P ? "EOSP2P" : "ENet";
}

ENetworkTransportType GetConfiguredTransportType() {
  constexpr ENetworkTransportType DefaultType = ENetworkTransportType::ENet;
  char* ConfigValue = nullptr;
  size_t ConfigLength = 0;
  const errno_t ConfigResult = _dupenv_s(&ConfigValue, &ConfigLength, "NetworkTransport");
  const std::string ConfigString = ConfigValue ? ConfigValue : "";
  std::free(ConfigValue);
  if (ConfigResult != 0 || ConfigString.empty()) {
    M_LOG(
        "[NetworkManager] NetworkTransport is not set. default={}",
        GetTransportTypeName(DefaultType)
    );
    return DefaultType;
  }
  if (std::string_view(ConfigString) == "EOSP2P") {
    return ENetworkTransportType::EOSP2P;
  }
  if (std::string_view(ConfigString) == "ENet") {
    return ENetworkTransportType::ENet;
  }
  M_LOG(
      "[NetworkManager] Unknown NetworkTransport value: value={} default={}",
      ConfigString,
      GetTransportTypeName(DefaultType)
  );
  return DefaultType;
}
}  // namespace

struct NetworkManager::Impl {
  struct FManagedPeer {
    FNetworkPeerId TransportPeerId = 0;
    FNetworkConnectionId ConnectionId = 0;
    ENetworkTransportType TransportType = ENetworkTransportType::ENet;
    std::string RemoteProductUserId;
    ETransportConnectionState ConnectionState = ETransportConnectionState::Connected;
  };

  std::unique_ptr<INetworkTransport> Transport;
  ENetworkTransportType TransportType = ENetworkTransportType::ENet;
  INetworkTransport::PeerAuthorizationCallback PeerAuthorization;
  std::unordered_map<FNetworkConnectionId, FManagedPeer> PeersByConnectionId;
  std::unordered_map<FNetworkPeerId, FNetworkConnectionId> ConnectionIdsByPeer;
  std::unordered_set<uint8_t> LoggedReceivedPacketTypes;
  FNetworkPeerId ServerPeerId = 0;
  bool bAcceptingSends = false;
  CallbackHandle NextCallbackHandle = 1;
  std::vector<std::pair<CallbackHandle, ConnectedCallback>> OnConnectedCallbacks;
  std::vector<std::pair<CallbackHandle, DisconnectedCallback>> OnDisconnectedCallbacks;
  std::vector<std::pair<CallbackHandle, PacketReceivedCallback>> OnPacketReceivedCallbacks;
};

NetworkManager& NetworkManager::GetInstance() {
  static NetworkManager Instance;
  return Instance;
}

NetworkManager::NetworkManager() : ImplPtr(new Impl()) {
  ImplPtr->TransportType = GetConfiguredTransportType();
  M_LOG(
      "[NetworkManager] Transport configured: type={}", GetTransportTypeName(ImplPtr->TransportType)
  );
}

NetworkManager::~NetworkManager() {
  Stop();
  delete ImplPtr;
  ImplPtr = nullptr;
}

bool NetworkManager::SetTransportType(ENetworkTransportType Type) {
  if (IsRunning()) {
    M_LOG("[NetworkManager] Transport change rejected while running.");
    return false;
  }
  if (Type != ENetworkTransportType::ENet && Type != ENetworkTransportType::EOSP2P) {
    M_LOG("[NetworkManager] Transport change rejected: unknown type={}", static_cast<int>(Type));
    return false;
  }

  ImplPtr->Transport.reset();
  ImplPtr->TransportType = Type;
  M_LOG("[NetworkManager] Transport selected: type={}", GetTransportTypeName(Type));
  return true;
}

ENetworkTransportType NetworkManager::GetTransportType() const { return ImplPtr->TransportType; }

void NetworkManager::SetPeerAuthorizationCallback(
    INetworkTransport::PeerAuthorizationCallback Callback
) {
  ImplPtr->PeerAuthorization = std::move(Callback);
  if (ImplPtr->Transport) {
    ImplPtr->Transport->SetPeerAuthorizationCallback(ImplPtr->PeerAuthorization);
  }
}

bool NetworkManager::StartServer(uint16_t Port, size_t MaxConnections, size_t ChannelCount) {
  Stop();
  if (!CreateSelectedTransport() ||
      !ImplPtr->Transport->StartHost(Port, MaxConnections, ChannelCount)) {
    M_LOG(
        "[NetworkManager] Listen server start failed: transport={} port={}",
        GetTransportTypeName(ImplPtr->TransportType),
        Port
    );
    Stop();
    return false;
  }

  bIsServer = true;
  bIsClient = false;
  NextConnectionId = 1;
  LocalConnectionId = 0;
  ImplPtr->LoggedReceivedPacketTypes.clear();
  ImplPtr->bAcceptingSends = true;
  M_LOG(
      "[NetworkManager] Listen server ready: transport={} port={}",
      GetTransportTypeName(ImplPtr->TransportType),
      Port
  );
  return true;
}

bool NetworkManager::ConnectToServer(
    const std::string& HostName, uint16_t Port, size_t ChannelCount
) {
  Stop();
  if (!CreateSelectedTransport()) {
    return false;
  }

  const FNetworkEndpoint Endpoint{HostName, Port};
  if (!ImplPtr->Transport->Connect(Endpoint, ChannelCount, ImplPtr->ServerPeerId)) {
    M_LOG(
        "[NetworkManager] Client connection start failed: transport={} target={} port={}",
        GetTransportTypeName(ImplPtr->TransportType),
        HostName,
        Port
    );
    Stop();
    return false;
  }

  bIsServer = false;
  bIsClient = true;
  LocalConnectionId = 0;
  ImplPtr->LoggedReceivedPacketTypes.clear();
  ImplPtr->bAcceptingSends = true;
  M_LOG(
      "[NetworkManager] Client connection started: transport={} target={} port={}",
      GetTransportTypeName(ImplPtr->TransportType),
      HostName,
      Port
  );
  return true;
}

void NetworkManager::Service() {
  if (!IsRunning()) {
    return;
  }

  bIsServicing = true;
  while (!bStopRequested && IsRunning() && ImplPtr->Transport->Service()) {
  }
  bIsServicing = false;

  if (bStopRequested) {
    bStopRequested = false;
    Stop();
  }
}

void NetworkManager::Disconnect() {
  if (!IsRunning()) {
    return;
  }

  ImplPtr->bAcceptingSends = false;
  if (bIsClient && ImplPtr->ServerPeerId != 0) {
    ImplPtr->Transport->Disconnect(ImplPtr->ServerPeerId);
  } else {
    std::vector<FNetworkPeerId> PeerIds;
    PeerIds.reserve(ImplPtr->PeersByConnectionId.size());
    for (const auto& Pair : ImplPtr->PeersByConnectionId) {
      PeerIds.push_back(Pair.second.TransportPeerId);
    }
    for (FNetworkPeerId PeerId : PeerIds) {
      ImplPtr->Transport->Disconnect(PeerId);
    }
  }
  M_LOG("[NetworkManager] Graceful disconnect requested.");
}

void NetworkManager::Stop() {
  if (bIsServicing) {
    bStopRequested = true;
    return;
  }

  ImplPtr->bAcceptingSends = false;
  std::vector<FNetworkConnectionId> DisconnectedConnectionIds;
  DisconnectedConnectionIds.reserve(ImplPtr->PeersByConnectionId.size());
  for (const auto& Pair : ImplPtr->PeersByConnectionId) {
    DisconnectedConnectionIds.push_back(Pair.first);
  }
  if (ImplPtr && ImplPtr->Transport) {
    ImplPtr->Transport->Stop();
    ImplPtr->Transport.reset();
  }
  bIsServer = false;
  bIsClient = false;
  NextConnectionId = 1;
  LocalConnectionId = 0;
  ClearPeers();
  for (FNetworkConnectionId ConnectionId : DisconnectedConnectionIds) {
    BroadcastDisconnected(ConnectionId, ESessionDisconnectReason::LocalLeave);
  }
  if (!DisconnectedConnectionIds.empty()) {
    M_LOG("[NetworkManager] Stop cleared peers: count={}", DisconnectedConnectionIds.size());
  }
}

bool NetworkManager::SendToServer(
    const FNetBuffer& Buffer, ENetPacketReliability Reliability, uint8_t ChannelId
) {
  if (!ImplPtr->bAcceptingSends || !bIsClient || ImplPtr->ServerPeerId == 0) {
    return false;
  }
  return SendToPeer(ImplPtr->ServerPeerId, Buffer, Reliability, ChannelId);
}

bool NetworkManager::SendToClient(
    FNetworkConnectionId ConnectionId,
    const FNetBuffer& Buffer,
    ENetPacketReliability Reliability,
    uint8_t ChannelId
) {
  if (!ImplPtr->bAcceptingSends || !bIsServer) {
    return false;
  }

  const auto Iterator = ImplPtr->PeersByConnectionId.find(ConnectionId);
  if (Iterator == ImplPtr->PeersByConnectionId.end()) {
    return false;
  }
  return SendToPeer(Iterator->second.TransportPeerId, Buffer, Reliability, ChannelId);
}

bool NetworkManager::Broadcast(
    const FNetBuffer& Buffer, ENetPacketReliability Reliability, uint8_t ChannelId
) {
  if (!ImplPtr->bAcceptingSends || !IsRunning() || !bIsServer || Buffer.Size() == 0) {
    return false;
  }
  bool bSentAny = false;
  bool bAllSucceeded = true;
  for (const auto& Pair : ImplPtr->PeersByConnectionId) {
    if (Pair.second.ConnectionState != ETransportConnectionState::Connected) {
      continue;
    }
    bSentAny = true;
    bAllSucceeded =
        SendToPeer(Pair.second.TransportPeerId, Buffer, Reliability, ChannelId) && bAllSucceeded;
  }
  return bSentAny && bAllSucceeded;
}

bool NetworkManager::IsRunning() const {
  return ImplPtr && ImplPtr->Transport && ImplPtr->Transport->IsRunning();
}

size_t NetworkManager::GetConnectedClientCount() const {
  if (!bIsServer) {
    return 0;
  }
  return static_cast<size_t>(std::count_if(
      ImplPtr->PeersByConnectionId.begin(),
      ImplPtr->PeersByConnectionId.end(),
      [](const auto& Pair) {
        return Pair.second.ConnectionState == ETransportConnectionState::Connected;
      }
  ));
}

NetworkManager::CallbackHandle NetworkManager::AddOnConnected(ConnectedCallback Callback) {
  if (!Callback) {
    return 0;
  }
  const CallbackHandle Handle = ImplPtr->NextCallbackHandle++;
  ImplPtr->OnConnectedCallbacks.push_back({Handle, std::move(Callback)});
  return Handle;
}

NetworkManager::CallbackHandle NetworkManager::AddOnDisconnected(DisconnectedCallback Callback) {
  if (!Callback) {
    return 0;
  }
  const CallbackHandle Handle = ImplPtr->NextCallbackHandle++;
  ImplPtr->OnDisconnectedCallbacks.push_back({Handle, std::move(Callback)});
  return Handle;
}

NetworkManager::CallbackHandle NetworkManager::AddOnPacketReceived(
    PacketReceivedCallback Callback
) {
  if (!Callback) {
    return 0;
  }
  const CallbackHandle Handle = ImplPtr->NextCallbackHandle++;
  ImplPtr->OnPacketReceivedCallbacks.push_back({Handle, std::move(Callback)});
  return Handle;
}

void NetworkManager::RemoveOnConnected(CallbackHandle Handle) {
  if (Handle == 0) {
    return;
  }
  ImplPtr->OnConnectedCallbacks.erase(
      std::remove_if(
          ImplPtr->OnConnectedCallbacks.begin(),
          ImplPtr->OnConnectedCallbacks.end(),
          [Handle](const auto& CallbackEntry) { return CallbackEntry.first == Handle; }
      ),
      ImplPtr->OnConnectedCallbacks.end()
  );
}

void NetworkManager::RemoveOnDisconnected(CallbackHandle Handle) {
  if (Handle == 0) {
    return;
  }
  ImplPtr->OnDisconnectedCallbacks.erase(
      std::remove_if(
          ImplPtr->OnDisconnectedCallbacks.begin(),
          ImplPtr->OnDisconnectedCallbacks.end(),
          [Handle](const auto& CallbackEntry) { return CallbackEntry.first == Handle; }
      ),
      ImplPtr->OnDisconnectedCallbacks.end()
  );
}

void NetworkManager::RemoveOnPacketReceived(CallbackHandle Handle) {
  if (Handle == 0) {
    return;
  }
  ImplPtr->OnPacketReceivedCallbacks.erase(
      std::remove_if(
          ImplPtr->OnPacketReceivedCallbacks.begin(),
          ImplPtr->OnPacketReceivedCallbacks.end(),
          [Handle](const auto& CallbackEntry) { return CallbackEntry.first == Handle; }
      ),
      ImplPtr->OnPacketReceivedCallbacks.end()
  );
}

bool NetworkManager::SendToPeer(
    FNetworkPeerId PeerId,
    const FNetBuffer& Buffer,
    ENetPacketReliability Reliability,
    uint8_t ChannelId
) {
  if (!IsRunning() || PeerId == 0 || Buffer.Size() == 0) {
    return false;
  }
  return ImplPtr->Transport->Send(PeerId, ChannelId, Reliability, Buffer.Data(), Buffer.Size());
}

bool NetworkManager::CreateSelectedTransport() {
  ImplPtr->Transport = CreateNetworkTransport(ImplPtr->TransportType);
  if (!ImplPtr->Transport) {
    M_LOG(
        "[NetworkManager] Transport creation failed: type={}",
        GetTransportTypeName(ImplPtr->TransportType)
    );
    return false;
  }

  BindTransportCallbacks();
  ImplPtr->Transport->SetPeerAuthorizationCallback(ImplPtr->PeerAuthorization);
  M_LOG(
      "[NetworkManager] Transport created for start: type={}",
      GetTransportTypeName(ImplPtr->TransportType)
  );
  return true;
}

void NetworkManager::BindTransportCallbacks() {
  if (!ImplPtr->Transport) {
    return;
  }
  ImplPtr->Transport->SetConnectedCallback([this](FNetworkPeerId PeerId) {
    HandleTransportConnected(PeerId);
  });
  ImplPtr->Transport->SetDisconnectedCallback(
      [this](FNetworkPeerId PeerId, ESessionDisconnectReason Reason) {
        HandleTransportDisconnected(PeerId, Reason);
      }
  );
  ImplPtr->Transport->SetConnectionStateChangedCallback(
      [this](FNetworkPeerId PeerId, ETransportConnectionState State) {
        HandleTransportConnectionStateChanged(PeerId, State);
      }
  );
  ImplPtr->Transport->SetPacketReceivedCallback([this](FReceivedPacket&& Packet) {
    HandleTransportPacket(std::move(Packet));
  });
}

FNetworkConnectionId NetworkManager::RegisterPeer(FNetworkPeerId PeerId) {
  if (PeerId == 0) {
    return 0;
  }
  const FNetworkConnectionId ExistingId = FindConnectionId(PeerId);
  if (ExistingId != 0) {
    return ExistingId;
  }

  const FNetworkConnectionId ConnectionId = bIsClient ? ServerConnectionId : NextConnectionId++;
  Impl::FManagedPeer Peer;
  Peer.TransportPeerId = PeerId;
  Peer.ConnectionId = ConnectionId;
  Peer.TransportType = ImplPtr->TransportType;
  Peer.RemoteProductUserId =
      ImplPtr->Transport ? ImplPtr->Transport->GetRemoteProductUserId(PeerId) : std::string{};
  Peer.ConnectionState = ETransportConnectionState::Connected;
  ImplPtr->PeersByConnectionId[ConnectionId] = std::move(Peer);
  ImplPtr->ConnectionIdsByPeer[PeerId] = ConnectionId;
  if (bIsClient) {
    ImplPtr->ServerPeerId = PeerId;
  }
  return ConnectionId;
}

FNetworkConnectionId NetworkManager::FindConnectionId(FNetworkPeerId PeerId) const {
  const auto Iterator = ImplPtr->ConnectionIdsByPeer.find(PeerId);
  return Iterator == ImplPtr->ConnectionIdsByPeer.end() ? 0 : Iterator->second;
}

void NetworkManager::RemovePeer(FNetworkPeerId PeerId) {
  const FNetworkConnectionId ConnectionId = FindConnectionId(PeerId);
  if (ConnectionId != 0) {
    ImplPtr->PeersByConnectionId.erase(ConnectionId);
  }
  ImplPtr->ConnectionIdsByPeer.erase(PeerId);
  if (PeerId == ImplPtr->ServerPeerId) {
    ImplPtr->ServerPeerId = 0;
  }
}

void NetworkManager::ClearPeers() {
  if (!ImplPtr) {
    return;
  }
  ImplPtr->PeersByConnectionId.clear();
  ImplPtr->ConnectionIdsByPeer.clear();
  ImplPtr->ServerPeerId = 0;
}

void NetworkManager::HandleTransportConnected(FNetworkPeerId PeerId) {
  const FNetworkConnectionId ConnectionId = RegisterPeer(PeerId);
  if (ConnectionId == 0) {
    return;
  }
  const auto Iterator = ImplPtr->PeersByConnectionId.find(ConnectionId);
  M_LOG(
      "[NetworkTransportTest] Connection verified: peer={} connection={} transport={} "
      "remoteUser={}",
      PeerId,
      ConnectionId,
      GetTransportTypeName(ImplPtr->TransportType),
      Iterator != ImplPtr->PeersByConnectionId.end() &&
              !Iterator->second.RemoteProductUserId.empty()
          ? Iterator->second.RemoteProductUserId
          : "<none>"
  );
  BroadcastConnected(ConnectionId);
}

void NetworkManager::HandleTransportDisconnected(
    FNetworkPeerId PeerId, ESessionDisconnectReason Reason
) {
  const FNetworkConnectionId ConnectionId = FindConnectionId(PeerId);
  RemovePeer(PeerId);
  if (ConnectionId != 0) {
    M_LOG(
        "[NetworkTransportTest] Disconnect verified: peer={} connection={} reason={}",
        PeerId,
        ConnectionId,
        static_cast<int>(Reason)
    );
    BroadcastDisconnected(ConnectionId, Reason);
  }
}

void NetworkManager::HandleTransportConnectionStateChanged(
    FNetworkPeerId PeerId, ETransportConnectionState State
) {
  const FNetworkConnectionId ConnectionId = FindConnectionId(PeerId);
  const auto Iterator = ImplPtr->PeersByConnectionId.find(ConnectionId);
  if (ConnectionId == 0 || Iterator == ImplPtr->PeersByConnectionId.end() ||
      Iterator->second.ConnectionState == State) {
    return;
  }
  Iterator->second.ConnectionState = State;
  M_LOG(
      "[NetworkManager] Peer state changed: peer={} connection={} state={}",
      PeerId,
      ConnectionId,
      State == ETransportConnectionState::Connected ? "Connected" : "Interrupted"
  );
}
void NetworkManager::HandleTransportPacket(FReceivedPacket&& Packet) {
  const FNetworkConnectionId ConnectionId = FindConnectionId(Packet.Sender);
  const auto PeerIterator = ImplPtr->PeersByConnectionId.find(ConnectionId);
  if (ConnectionId == 0 || PeerIterator == ImplPtr->PeersByConnectionId.end() ||
      PeerIterator->second.ConnectionState != ETransportConnectionState::Connected ||
      Packet.Data.empty()) {
    return;
  }

  FNetBuffer Buffer(std::move(Packet.Data));
  ENetPacketType PacketType = ENetPacketType::None;
  if (!Buffer.Read(PacketType)) {
    return;
  }
  Buffer.ResetRead();

  const uint8_t PacketTypeValue = static_cast<uint8_t>(PacketType);
  if (ImplPtr->LoggedReceivedPacketTypes.insert(PacketTypeValue).second) {
    M_LOG(
        "[NetworkTransportTest] Receive verified: packetType={} peer={} connection={} channel={} "
        "bytes={}",
        GetPacketTypeName(PacketType),
        Packet.Sender,
        ConnectionId,
        Packet.Channel,
        Buffer.Size()
    );
  }
  BroadcastPacketReceived(ConnectionId, Buffer);
}

void NetworkManager::BroadcastConnected(FNetworkConnectionId ConnectionId) {
  const auto Callbacks = ImplPtr->OnConnectedCallbacks;
  for (const auto& CallbackEntry : Callbacks) {
    if (CallbackEntry.second) {
      CallbackEntry.second(ConnectionId);
    }
  }
}

void NetworkManager::BroadcastDisconnected(
    FNetworkConnectionId ConnectionId, ESessionDisconnectReason Reason
) {
  const auto Callbacks = ImplPtr->OnDisconnectedCallbacks;
  for (const auto& CallbackEntry : Callbacks) {
    if (CallbackEntry.second) {
      CallbackEntry.second(ConnectionId, Reason);
    }
  }
}

void NetworkManager::BroadcastPacketReceived(
    FNetworkConnectionId ConnectionId, FNetBuffer& Buffer
) {
  const auto Callbacks = ImplPtr->OnPacketReceivedCallbacks;
  for (const auto& CallbackEntry : Callbacks) {
    Buffer.ResetRead();
    if (CallbackEntry.second) {
      CallbackEntry.second(ConnectionId, Buffer);
    }
  }
}
