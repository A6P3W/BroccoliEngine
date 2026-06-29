#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>
#include <utility>

#include "NetBuffer.h"
#include "NetworkTypes.h"

struct _ENetHost;
struct _ENetPeer;
using ENetHost = _ENetHost;
using ENetPeer = _ENetPeer;

enum class ENetPacketReliability { Reliable, Unreliable };

class NetworkManager {
 public:
  using ConnectedCallback = std::function<void(FNetworkConnectionId)>;
  using DisconnectedCallback = std::function<void(FNetworkConnectionId)>;
  using PacketReceivedCallback = std::function<void(FNetworkConnectionId, FNetBuffer&)>;

  static NetworkManager& GetInstance();

  bool StartServer(uint16_t Port, size_t MaxConnections = 32, size_t ChannelCount = 2);
  bool ConnectToServer(const std::string& HostName, uint16_t Port, size_t ChannelCount = 2);

  void Service();
  void Disconnect();
  void Stop();

  bool SendToServer(
      const FNetBuffer& Buffer,
      ENetPacketReliability Reliability = ENetPacketReliability::Reliable,
      uint8_t ChannelId = 0
  );
  bool SendToClient(
      FNetworkConnectionId ConnectionId,
      const FNetBuffer& Buffer,
      ENetPacketReliability Reliability = ENetPacketReliability::Reliable,
      uint8_t ChannelId = 0
  );
  bool Broadcast(
      const FNetBuffer& Buffer,
      ENetPacketReliability Reliability = ENetPacketReliability::Reliable,
      uint8_t ChannelId = 0
  );

  bool IsRunning() const { return Host != nullptr; }
  bool IsServer() const { return bIsServer; }
  bool IsClient() const { return bIsClient; }
  FNetworkConnectionId GetLocalConnectionId() const { return LocalConnectionId; }

  void SetLocalConnectionId(FNetworkConnectionId ConnectionId) { LocalConnectionId = ConnectionId; }

  void SetOnConnected(ConnectedCallback Callback) { OnConnected = std::move(Callback); }
  void SetOnDisconnected(DisconnectedCallback Callback) { OnDisconnected = std::move(Callback); }
  void SetOnPacketReceived(PacketReceivedCallback Callback) {
    OnPacketReceived = std::move(Callback);
  }

 private:
  NetworkManager();
  ~NetworkManager();
  NetworkManager(const NetworkManager&) = delete;
  NetworkManager& operator=(const NetworkManager&) = delete;

  bool SendToPeer(
      ENetPeer* Peer, const FNetBuffer& Buffer, ENetPacketReliability Reliability, uint8_t ChannelId
  );
  FNetworkConnectionId RegisterPeer(ENetPeer* Peer);
  FNetworkConnectionId FindConnectionId(ENetPeer* Peer) const;
  void RemovePeer(ENetPeer* Peer);
  void ClearPeers();

  ENetHost* Host = nullptr;
  ENetPeer* ServerPeer = nullptr;
  bool bEnetInitialized = false;
  bool bIsServer = false;
  bool bIsClient = false;
  FNetworkConnectionId NextConnectionId = 1;
  FNetworkConnectionId ServerConnectionId = 1;
  FNetworkConnectionId LocalConnectionId = 0;

  struct Impl;
  Impl* ImplPtr = nullptr;

  ConnectedCallback OnConnected;
  DisconnectedCallback OnDisconnected;
  PacketReceivedCallback OnPacketReceived;
};
