#include <enet/enet.h>

#include <unordered_map>
#include <utility>

#include "Log.h"
#include "NetworkTransport.h"

namespace {
enet_uint32 GetENetPacketFlags(ENetworkReliability Reliability) {
  return Reliability == ENetworkReliability::Reliable ? ENET_PACKET_FLAG_RELIABLE : 0;
}

class FENetTransport final : public INetworkTransport {
 public:
  FENetTransport() {
    Initialized = enet_initialize() == 0;
    M_LOG("[NetworkTransport] Created transport=ENet initialized={}", Initialized);
  }

  ~FENetTransport() override {
    Stop();
    if (Initialized) {
      enet_deinitialize();
    }
  }

  bool StartHost(uint16_t Port, size_t MaxConnections, size_t ChannelCount) override {
    Stop();
    if (!Initialized) {
      M_LOG("[NetworkTransport] ENet StartHost failed: ENet is not initialized.");
      return false;
    }

    ENetAddress Address{};
    Address.host = ENET_HOST_ANY;
    Address.port = Port;
    Host = enet_host_create(&Address, MaxConnections, ChannelCount, 0, 0);
    if (!Host) {
      M_LOG("[NetworkTransport] ENet StartHost failed: port={}", Port);
      return false;
    }

    M_LOG(
        "[NetworkTransport] ENet host started: port={} maxPeers={} channels={}",
        Port,
        MaxConnections,
        ChannelCount
    );
    return true;
  }

  bool Connect(
      const FNetworkEndpoint& Endpoint, size_t ChannelCount, FNetworkPeerId& OutPeerId
  ) override {
    Stop();
    OutPeerId = 0;
    if (!Initialized) {
      M_LOG("[NetworkTransport] ENet Connect failed: ENet is not initialized.");
      return false;
    }

    Host = enet_host_create(nullptr, 1, ChannelCount, 0, 0);
    if (!Host) {
      M_LOG("[NetworkTransport] ENet Connect failed: client host creation failed.");
      return false;
    }

    ENetAddress Address{};
    if (enet_address_set_host(&Address, Endpoint.HostName.c_str()) != 0) {
      M_LOG("[NetworkTransport] ENet Connect failed: host={}", Endpoint.HostName);
      Stop();
      return false;
    }
    Address.port = Endpoint.Port;

    ENetPeer* Peer = enet_host_connect(Host, &Address, ChannelCount, 0);
    if (!Peer) {
      M_LOG("[NetworkTransport] ENet Connect failed: peer creation failed.");
      Stop();
      return false;
    }

    OutPeerId = RegisterPeer(Peer);
    M_LOG(
        "[NetworkTransport] ENet connection requested: host={} port={} peer={}",
        Endpoint.HostName,
        Endpoint.Port,
        OutPeerId
    );
    return true;
  }

  bool Service() override {
    if (!Host) {
      return false;
    }

    ENetEvent Event{};
    if (enet_host_service(Host, &Event, 0) <= 0) {
      return false;
    }

    switch (Event.type) {
      case ENET_EVENT_TYPE_CONNECT: {
        const FNetworkPeerId PeerId = RegisterPeer(Event.peer);
        M_LOG("[NetworkTransport] ENet peer connected: peer={}", PeerId);
        if (OnConnected) {
          OnConnected(PeerId);
        }
        break;
      }
      case ENET_EVENT_TYPE_DISCONNECT: {
        const FNetworkPeerId PeerId = FindPeerId(Event.peer);
        RemovePeer(Event.peer);
        M_LOG("[NetworkTransport] ENet peer disconnected: peer={}", PeerId);
        if (PeerId != 0 && OnDisconnected) {
          OnDisconnected(PeerId);
        }
        break;
      }
      case ENET_EVENT_TYPE_RECEIVE: {
        const FNetworkPeerId PeerId = FindPeerId(Event.peer);
        if (PeerId != 0 && Event.packet && Event.packet->data && Event.packet->dataLength > 0 &&
            OnPacketReceived) {
          FReceivedPacket Packet;
          Packet.Sender = PeerId;
          Packet.Channel = Event.channelID;
          Packet.Data.assign(Event.packet->data, Event.packet->data + Event.packet->dataLength);
          OnPacketReceived(std::move(Packet));
        }
        if (Event.packet) {
          enet_packet_destroy(Event.packet);
        }
        break;
      }
      default:
        break;
    }
    return true;
  }

  bool Send(
      FNetworkPeerId PeerId,
      uint8_t Channel,
      ENetworkReliability Reliability,
      const uint8_t* Data,
      size_t Size
  ) override {
    if (!Host || !Data || Size == 0) {
      return false;
    }

    const auto Iterator = PeersById.find(PeerId);
    if (Iterator == PeersById.end() || !Iterator->second) {
      return false;
    }

    ENetPacket* Packet = enet_packet_create(Data, Size, GetENetPacketFlags(Reliability));
    if (!Packet) {
      return false;
    }
    if (enet_peer_send(Iterator->second, Channel, Packet) != 0) {
      enet_packet_destroy(Packet);
      return false;
    }

    enet_host_flush(Host);
    LogSuccessfulSend(Reliability, Channel, Size, "peer");
    return true;
  }

  bool Broadcast(
      uint8_t Channel, ENetworkReliability Reliability, const uint8_t* Data, size_t Size
  ) override {
    if (!Host || !Data || Size == 0) {
      return false;
    }

    ENetPacket* Packet = enet_packet_create(Data, Size, GetENetPacketFlags(Reliability));
    if (!Packet) {
      return false;
    }
    enet_host_broadcast(Host, Channel, Packet);
    enet_host_flush(Host);
    LogSuccessfulSend(Reliability, Channel, Size, "broadcast");
    return true;
  }

  void Disconnect(FNetworkPeerId PeerId) override {
    const auto Iterator = PeersById.find(PeerId);
    if (!Host || Iterator == PeersById.end() || !Iterator->second) {
      return;
    }
    enet_peer_disconnect(Iterator->second, 0);
    enet_host_flush(Host);
  }

  void Stop() override {
    if (!Host) {
      ClearPeers();
      return;
    }

    for (const auto& Pair : PeersById) {
      if (Pair.second) {
        enet_peer_disconnect_now(Pair.second, 0);
      }
    }
    enet_host_destroy(Host);
    Host = nullptr;
    ClearPeers();
    M_LOG("[NetworkTransport] ENet transport stopped.");
  }

  bool IsRunning() const override { return Host != nullptr; }

  void SetConnectedCallback(ConnectedCallback Callback) override {
    OnConnected = std::move(Callback);
  }

  void SetDisconnectedCallback(DisconnectedCallback Callback) override {
    OnDisconnected = std::move(Callback);
  }

  void SetPacketReceivedCallback(PacketReceivedCallback Callback) override {
    OnPacketReceived = std::move(Callback);
  }

 private:
  void LogSuccessfulSend(
      ENetworkReliability Reliability, uint8_t Channel, size_t Size, const char* Target
  ) {
    bool& LoggedSend =
        Reliability == ENetworkReliability::Reliable ? LoggedReliableSend : LoggedUnreliableSend;
    if (LoggedSend) {
      return;
    }

    LoggedSend = true;
    M_LOG(
        "[ENetTransportTest] Send verified: reliability={} target={} channel={} bytes={}",
        Reliability == ENetworkReliability::Reliable ? "Reliable" : "Unreliable",
        Target,
        Channel,
        Size
    );
  }

  FNetworkPeerId RegisterPeer(ENetPeer* Peer) {
    if (!Peer) {
      return 0;
    }

    const FNetworkPeerId ExistingPeerId = FindPeerId(Peer);
    if (ExistingPeerId != 0) {
      return ExistingPeerId;
    }

    const FNetworkPeerId PeerId = NextPeerId++;
    PeersById[PeerId] = Peer;
    PeerIdsByPeer[Peer] = PeerId;
    return PeerId;
  }

  FNetworkPeerId FindPeerId(ENetPeer* Peer) const {
    const auto Iterator = PeerIdsByPeer.find(Peer);
    return Iterator == PeerIdsByPeer.end() ? 0 : Iterator->second;
  }

  void RemovePeer(ENetPeer* Peer) {
    const FNetworkPeerId PeerId = FindPeerId(Peer);
    if (PeerId != 0) {
      PeersById.erase(PeerId);
    }
    PeerIdsByPeer.erase(Peer);
  }

  void ClearPeers() {
    PeersById.clear();
    PeerIdsByPeer.clear();
    NextPeerId = 1;
    LoggedReliableSend = false;
    LoggedUnreliableSend = false;
  }

  ENetHost* Host = nullptr;
  bool Initialized = false;
  FNetworkPeerId NextPeerId = 1;
  bool LoggedReliableSend = false;
  bool LoggedUnreliableSend = false;
  std::unordered_map<FNetworkPeerId, ENetPeer*> PeersById;
  std::unordered_map<ENetPeer*, FNetworkPeerId> PeerIdsByPeer;
  ConnectedCallback OnConnected;
  DisconnectedCallback OnDisconnected;
  PacketReceivedCallback OnPacketReceived;
};
}  // namespace
std::unique_ptr<INetworkTransport> CreateEOSP2PTransport();

std::unique_ptr<INetworkTransport> CreateNetworkTransport(ENetworkTransportType Type) {
  switch (Type) {
    case ENetworkTransportType::EOSP2P:
      return CreateEOSP2PTransport();
    case ENetworkTransportType::ENet:
    default:
      return std::make_unique<FENetTransport>();
  }
}
