#pragma once
#include "BroccoliEngineAPI.h"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>
#include <utility>
#include <vector>

#include "NetBuffer.h"
#include "NetworkTypes.h"

struct _ENetHost;
struct _ENetPeer;
using ENetHost = _ENetHost;
using ENetPeer = _ENetPeer;

enum class ENetPacketReliability { Reliable, Unreliable };

class BROCCOLI_ENGINE_API NetworkManager {
 public:
  using ConnectedCallback = std::function<void(FNetworkConnectionId)>;
  using DisconnectedCallback = std::function<void(FNetworkConnectionId)>;
  using PacketReceivedCallback = std::function<void(FNetworkConnectionId, FNetBuffer&)>;
  using CallbackHandle = size_t;

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
  size_t GetConnectedClientCount() const;
  FNetworkConnectionId GetLocalConnectionId() const { return LocalConnectionId; }

  void SetLocalConnectionId(FNetworkConnectionId ConnectionId) { LocalConnectionId = ConnectionId; }

  CallbackHandle AddOnConnected(ConnectedCallback Callback);
  CallbackHandle AddOnDisconnected(DisconnectedCallback Callback);
  CallbackHandle AddOnPacketReceived(PacketReceivedCallback Callback);
  void RemoveOnConnected(CallbackHandle Handle);
  void RemoveOnDisconnected(CallbackHandle Handle);
  void RemoveOnPacketReceived(CallbackHandle Handle);

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
  void BroadcastConnected(FNetworkConnectionId ConnectionId);
  void BroadcastDisconnected(FNetworkConnectionId ConnectionId);
  void BroadcastPacketReceived(FNetworkConnectionId ConnectionId, FNetBuffer& Buffer);

  ENetHost* Host = nullptr;
  ENetPeer* ServerPeer = nullptr;
  bool bEnetInitialized = false;
  bool bIsServer = false;
  bool bIsClient = false;
  bool bIsServicing = false;
  bool bStopRequested = false;
  FNetworkConnectionId NextConnectionId = 1;
  FNetworkConnectionId ServerConnectionId = 1;
  FNetworkConnectionId LocalConnectionId = 0;

  struct Impl;
  Impl* ImplPtr = nullptr;


};
