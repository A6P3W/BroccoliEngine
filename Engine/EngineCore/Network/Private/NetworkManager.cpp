#include "NetworkManager.h"

#include <enet/enet.h>

#include <algorithm>
#include <unordered_map>
#include <vector>

#include "NetPacketType.h"

struct NetworkManager::Impl {
  std::unordered_map<FNetworkConnectionId, ENetPeer*> PeersByConnectionId;
  std::unordered_map<ENetPeer*, FNetworkConnectionId> ConnectionIdsByPeer;
};

NetworkManager& NetworkManager::GetInstance() {
  static NetworkManager Instance;
  return Instance;
}

NetworkManager::NetworkManager() : ImplPtr(new Impl()) {
  bEnetInitialized = (enet_initialize() == 0);
}

NetworkManager::~NetworkManager() {
  Stop();
  delete ImplPtr;
  ImplPtr = nullptr;
  if (bEnetInitialized) {
    enet_deinitialize();
  }
}

bool NetworkManager::StartServer(uint16_t Port, size_t MaxConnections, size_t ChannelCount) {
  Stop();
  if (!bEnetInitialized) {
    return false;
  }

  ENetAddress address{};
  address.host = ENET_HOST_ANY;
  address.port = Port;

  Host = enet_host_create(&address, MaxConnections, ChannelCount, 0, 0);

  if (!Host) {
    return false;
  }

  bIsServer = true;
  bIsClient = false;
  NextConnectionId = 1;
  LocalConnectionId = 0;
  return true;
}

bool NetworkManager::ConnectToServer(
    const std::string& HostName, uint16_t Port, size_t ChannelCount
) {
  Stop();
  if (!bEnetInitialized) {
    return false;
  }

  Host = enet_host_create(nullptr, 1, ChannelCount, 0, 0);

  if (!Host) {
    return false;
  }

  ENetAddress address{};
  if (enet_address_set_host(&address, HostName.c_str()) != 0) {
    Stop();
    return false;
  }
  address.port = Port;

  ServerPeer = enet_host_connect(Host, &address, ChannelCount, 0);
  if (!ServerPeer) {
    Stop();
    return false;
  }

  bIsServer = false;
  bIsClient = true;
  LocalConnectionId = 0;
  return true;
}

void NetworkManager::Service() {
  if (!Host) {
    return;
  }

  ENetEvent event{};
  bIsServicing = true;
  while (!bStopRequested && Host && enet_host_service(Host, &event, 0) > 0) {
    switch (event.type) {
      case ENET_EVENT_TYPE_CONNECT: {
        const FNetworkConnectionId connectionId = RegisterPeer(event.peer);
        BroadcastConnected(connectionId);
        break;
      }
      case ENET_EVENT_TYPE_DISCONNECT: {
        const FNetworkConnectionId connectionId = FindConnectionId(event.peer);
        RemovePeer(event.peer);
        if (connectionId != 0) {
          BroadcastDisconnected(connectionId);
        }
        break;
      }
      case ENET_EVENT_TYPE_RECEIVE: {
        const FNetworkConnectionId connectionId = FindConnectionId(event.peer);
        if (connectionId != 0 && event.packet && event.packet->data &&
            event.packet->dataLength > 0) {
          std::vector<uint8_t> bytes(
              event.packet->data, event.packet->data + event.packet->dataLength
          );
          FNetBuffer buffer(std::move(bytes));

          ENetPacketType packetType = ENetPacketType::None;
          if (buffer.Read(packetType)) {
            buffer.ResetRead();
            BroadcastPacketReceived(connectionId, buffer);
          }
        }

        if (event.packet) {
          enet_packet_destroy(event.packet);
        }
        break;
      }
      default:
        break;
    }
  }
  bIsServicing = false;

  if (bStopRequested) {
    bStopRequested = false;
    Stop();
  }
}

void NetworkManager::Disconnect() {
  if (!Host) {
    return;
  }

  if (bIsClient && ServerPeer) {
    enet_peer_disconnect(ServerPeer, 0);
  } else {
    for (const auto& pair : ImplPtr->PeersByConnectionId) {
      if (pair.second) {
        enet_peer_disconnect(pair.second, 0);
      }
    }
  }

  enet_host_flush(Host);
}

void NetworkManager::Stop() {
  if (bIsServicing) {
    bStopRequested = true;
    return;
  }

  if (!Host) {
    ClearPeers();
    return;
  }

  if (bIsClient && ServerPeer) {
    enet_peer_disconnect_now(ServerPeer, 0);
  } else {
    for (const auto& pair : ImplPtr->PeersByConnectionId) {
      if (pair.second) {
        enet_peer_disconnect_now(pair.second, 0);
      }
    }
  }

  enet_host_destroy(Host);
  Host = nullptr;
  ServerPeer = nullptr;
  bIsServer = false;
  bIsClient = false;
  NextConnectionId = 1;
  LocalConnectionId = 0;
  ClearPeers();
}

bool NetworkManager::SendToServer(
    const FNetBuffer& Buffer, ENetPacketReliability Reliability, uint8_t ChannelId
) {
  if (!bIsClient || !ServerPeer) {
    return false;
  }

  return SendToPeer(ServerPeer, Buffer, Reliability, ChannelId);
}

bool NetworkManager::SendToClient(
    FNetworkConnectionId ConnectionId,
    const FNetBuffer& Buffer,
    ENetPacketReliability Reliability,
    uint8_t ChannelId
) {
  if (!bIsServer) {
    return false;
  }

  auto it = ImplPtr->PeersByConnectionId.find(ConnectionId);
  if (it == ImplPtr->PeersByConnectionId.end()) {
    return false;
  }

  return SendToPeer(it->second, Buffer, Reliability, ChannelId);
}

bool NetworkManager::Broadcast(
    const FNetBuffer& Buffer, ENetPacketReliability Reliability, uint8_t ChannelId
) {
  if (!Host || !bIsServer || Buffer.Size() == 0) {
    return false;
  }

  const enet_uint32 flags =
      (Reliability == ENetPacketReliability::Reliable) ? ENET_PACKET_FLAG_RELIABLE : 0;
  ENetPacket* packet = enet_packet_create(Buffer.Data(), Buffer.Size(), flags);
  if (!packet) {
    return false;
  }

  enet_host_broadcast(Host, ChannelId, packet);
  enet_host_flush(Host);
  return true;
}

NetworkManager::CallbackHandle NetworkManager::AddOnConnected(ConnectedCallback Callback) {
  if (!Callback) {
    return 0;
  }

  const CallbackHandle handle = NextCallbackHandle++;
  OnConnectedCallbacks.push_back({handle, std::move(Callback)});
  return handle;
}

NetworkManager::CallbackHandle NetworkManager::AddOnDisconnected(DisconnectedCallback Callback) {
  if (!Callback) {
    return 0;
  }

  const CallbackHandle handle = NextCallbackHandle++;
  OnDisconnectedCallbacks.push_back({handle, std::move(Callback)});
  return handle;
}

NetworkManager::CallbackHandle NetworkManager::AddOnPacketReceived(
    PacketReceivedCallback Callback
) {
  if (!Callback) {
    return 0;
  }

  const CallbackHandle handle = NextCallbackHandle++;
  OnPacketReceivedCallbacks.push_back({handle, std::move(Callback)});
  return handle;
}

void NetworkManager::RemoveOnConnected(CallbackHandle Handle) {
  if (Handle == 0) {
    return;
  }

  OnConnectedCallbacks.erase(
      std::remove_if(
          OnConnectedCallbacks.begin(),
          OnConnectedCallbacks.end(),
          [Handle](const auto& CallbackEntry) { return CallbackEntry.first == Handle; }
      ),
      OnConnectedCallbacks.end()
  );
}

void NetworkManager::RemoveOnDisconnected(CallbackHandle Handle) {
  if (Handle == 0) {
    return;
  }

  OnDisconnectedCallbacks.erase(
      std::remove_if(
          OnDisconnectedCallbacks.begin(),
          OnDisconnectedCallbacks.end(),
          [Handle](const auto& CallbackEntry) { return CallbackEntry.first == Handle; }
      ),
      OnDisconnectedCallbacks.end()
  );
}

void NetworkManager::RemoveOnPacketReceived(CallbackHandle Handle) {
  if (Handle == 0) {
    return;
  }

  OnPacketReceivedCallbacks.erase(
      std::remove_if(
          OnPacketReceivedCallbacks.begin(),
          OnPacketReceivedCallbacks.end(),
          [Handle](const auto& CallbackEntry) { return CallbackEntry.first == Handle; }
      ),
      OnPacketReceivedCallbacks.end()
  );
}

bool NetworkManager::SendToPeer(
    ENetPeer* Peer, const FNetBuffer& Buffer, ENetPacketReliability Reliability, uint8_t ChannelId
) {
  if (!Host || !Peer || Buffer.Size() == 0) {
    return false;
  }

  const enet_uint32 flags =
      (Reliability == ENetPacketReliability::Reliable) ? ENET_PACKET_FLAG_RELIABLE : 0;
  ENetPacket* packet = enet_packet_create(Buffer.Data(), Buffer.Size(), flags);
  if (!packet) {
    return false;
  }

  if (enet_peer_send(Peer, ChannelId, packet) != 0) {
    enet_packet_destroy(packet);
    return false;
  }

  enet_host_flush(Host);
  return true;
}

FNetworkConnectionId NetworkManager::RegisterPeer(ENetPeer* Peer) {
  if (!Peer) {
    return 0;
  }

  const FNetworkConnectionId existingId = FindConnectionId(Peer);
  if (existingId != 0) {
    return existingId;
  }

  const FNetworkConnectionId connectionId = bIsClient ? ServerConnectionId : NextConnectionId++;
  ImplPtr->PeersByConnectionId[connectionId] = Peer;
  ImplPtr->ConnectionIdsByPeer[Peer] = connectionId;
  if (bIsClient) {
    ServerPeer = Peer;
  }

  return connectionId;
}

FNetworkConnectionId NetworkManager::FindConnectionId(ENetPeer* Peer) const {
  if (!Peer) {
    return 0;
  }

  auto it = ImplPtr->ConnectionIdsByPeer.find(Peer);
  if (it == ImplPtr->ConnectionIdsByPeer.end()) {
    return 0;
  }

  return it->second;
}

void NetworkManager::RemovePeer(ENetPeer* Peer) {
  const FNetworkConnectionId connectionId = FindConnectionId(Peer);
  if (connectionId != 0) {
    ImplPtr->PeersByConnectionId.erase(connectionId);
  }
  ImplPtr->ConnectionIdsByPeer.erase(Peer);

  if (Peer == ServerPeer) {
    ServerPeer = nullptr;
  }
}

void NetworkManager::ClearPeers() {
  if (!ImplPtr) {
    return;
  }

  ImplPtr->PeersByConnectionId.clear();
  ImplPtr->ConnectionIdsByPeer.clear();
}

void NetworkManager::BroadcastConnected(FNetworkConnectionId ConnectionId) {
  const auto callbacks = OnConnectedCallbacks;
  for (const auto& callbackEntry : callbacks) {
    if (callbackEntry.second) {
      callbackEntry.second(ConnectionId);
    }
  }
}

void NetworkManager::BroadcastDisconnected(FNetworkConnectionId ConnectionId) {
  const auto callbacks = OnDisconnectedCallbacks;
  for (const auto& callbackEntry : callbacks) {
    if (callbackEntry.second) {
      callbackEntry.second(ConnectionId);
    }
  }
}

void NetworkManager::BroadcastPacketReceived(FNetworkConnectionId ConnectionId, FNetBuffer& Buffer) {
  const auto callbacks = OnPacketReceivedCallbacks;
  for (const auto& callbackEntry : callbacks) {
    Buffer.ResetRead();
    if (callbackEntry.second) {
      callbackEntry.second(ConnectionId, Buffer);
    }
  }
}
