#pragma once
#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>

#include "BroccoliEngineAPI.h"
#include "NetBuffer.h"
#include "NetworkTransport.h"
#include "NetworkTypes.h"

class BROCCOLI_ENGINE_API NetworkManager {
 public:
  using ConnectedCallback = std::function<void(FNetworkConnectionId)>;
  using DisconnectedCallback = std::function<void(FNetworkConnectionId, ESessionDisconnectReason)>;
  using PacketReceivedCallback = std::function<void(FNetworkConnectionId, FNetBuffer&)>;
  using CallbackHandle = size_t;

  static NetworkManager& GetInstance();

  bool SetTransportType(ENetworkTransportType Type);
  ENetworkTransportType GetTransportType() const;
  void SetPeerAuthorizationCallback(INetworkTransport::PeerAuthorizationCallback Callback);
  bool StartServer(uint16_t Port, size_t MaxConnections = 32, size_t ChannelCount = 2);
  bool ConnectToServer(const std::string& HostName, uint16_t Port, size_t ChannelCount = 2);

  void Service();
  void Disconnect();
  void Stop();

  bool SendToServer(
      const FNetBuffer& Buffer,
      ENetworkReliability Reliability = ENetworkReliability::Reliable,
      uint8_t ChannelId = 0
  );
  bool SendToClient(
      FNetworkConnectionId ConnectionId,
      const FNetBuffer& Buffer,
      ENetworkReliability Reliability = ENetworkReliability::Reliable,
      uint8_t ChannelId = 0
  );
  bool Broadcast(
      const FNetBuffer& Buffer,
      ENetworkReliability Reliability = ENetworkReliability::Reliable,
      uint8_t ChannelId = 0
  );

  bool IsRunning() const;
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
      FNetworkPeerId PeerId,
      const FNetBuffer& Buffer,
      ENetworkReliability Reliability,
      uint8_t ChannelId
  );
  bool CreateSelectedTransport();
  void BindTransportCallbacks();
  FNetworkConnectionId RegisterPeer(FNetworkPeerId PeerId);
  FNetworkConnectionId FindConnectionId(FNetworkPeerId PeerId) const;
  void RemovePeer(FNetworkPeerId PeerId);
  void ClearPeers();
  void HandleTransportConnected(FNetworkPeerId PeerId);
  void HandleTransportDisconnected(FNetworkPeerId PeerId, ESessionDisconnectReason Reason);
  void HandleTransportConnectionStateChanged(
      FNetworkPeerId PeerId, ETransportConnectionState State
  );
  void HandleTransportPacket(FReceivedPacket&& Packet);
  void BroadcastConnected(FNetworkConnectionId ConnectionId);
  void BroadcastDisconnected(FNetworkConnectionId ConnectionId, ESessionDisconnectReason Reason);
  void BroadcastPacketReceived(FNetworkConnectionId ConnectionId, FNetBuffer& Buffer);

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
