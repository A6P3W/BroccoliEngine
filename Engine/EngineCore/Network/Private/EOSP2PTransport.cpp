#include <eos_p2p.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cstring>
#include <limits>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "EOSAuthManager.h"
#include "EOSCoreManager.h"
#include "Log.h"
#include "NetworkTransport.h"

namespace {
constexpr const char* SocketName = "BroccoliGame";
constexpr auto InterruptedConnectionTimeout = std::chrono::seconds(10);

const char* SafeEOSResult(EOS_EResult Result) {
  const char* Text = EOS_EResult_ToString(Result);
  return Text ? Text : "<unknown>";
}

std::string ProductUserIdToString(EOS_ProductUserId UserId) {
  if (!UserId) {
    return {};
  }

  std::array<char, EOS_PRODUCTUSERID_MAX_LENGTH + 1> Buffer = {};
  int32_t BufferLength = static_cast<int32_t>(Buffer.size());
  if (EOS_ProductUserId_ToString(UserId, Buffer.data(), &BufferLength) !=
      EOS_EResult::EOS_Success) {
    return {};
  }
  return Buffer.data();
}

bool IsExpectedSocket(const EOS_P2P_SocketId* Value) {
  return Value && std::strncmp(Value->SocketName, SocketName, sizeof(Value->SocketName)) == 0;
}

EOS_EPacketReliability ConvertReliability(ENetworkReliability Value) {
  return Value == ENetworkReliability::Reliable
             ? EOS_EPacketReliability::EOS_PR_ReliableOrdered
             : EOS_EPacketReliability::EOS_PR_UnreliableUnordered;
}

class FEOSP2PTransport final : public INetworkTransport {
 public:
  FEOSP2PTransport() {
    SocketId.ApiVersion = EOS_P2P_SOCKETID_API_LATEST;
    strcpy_s(SocketId.SocketName, SocketName);
    M_LOG("[NetworkTransport] Created transport=EOSP2P socket={}", SocketName);
  }

  ~FEOSP2PTransport() override { Stop(); }

  bool StartHost(uint16_t Port, size_t MaxConnections, size_t ChannelCount) override {
    Stop();
    if (!Initialize(MaxConnections, ChannelCount)) {
      M_LOG("[EOSP2PTransport] StartHost failed: port={} (unused by EOS P2P).", Port);
      return false;
    }

    Hosting = true;
    M_LOG(
        "[EOSP2PTransportTest] Foundation ready: mode=host socket={} maxPeers={} channels={}",
        SocketName,
        MaximumConnections,
        MaximumChannels
    );
    return true;
  }

  bool Connect(
      const FNetworkEndpoint& Endpoint, size_t ChannelCount, FNetworkPeerId& OutPeerId
  ) override {
    Stop();
    OutPeerId = 0;
    if (!Initialize(1, ChannelCount)) {
      M_LOG("[EOSP2PTransport] Connect failed: EOS P2P initialization failed.");
      return false;
    }

    EOS_ProductUserId RemoteUserId = EOS_ProductUserId_FromString(Endpoint.HostName.c_str());
    if (!RemoteUserId) {
      M_LOG(
          "[EOSP2PTransport] Connect failed: host must be a Product User ID. host={}",
          Endpoint.HostName
      );
      Stop();
      return false;
    }

    OutPeerId = RegisterPeer(RemoteUserId);
    if (OutPeerId == 0) {
      M_LOG("[EOSP2PTransport] Connect failed: remote user registration failed.");
      Stop();
      return false;
    }

    constexpr uint8_t ConnectionProbe = 0;
    if (!Send(
            OutPeerId, 0, ENetworkReliability::Reliable, &ConnectionProbe, sizeof(ConnectionProbe)
        )) {
      M_LOG("[EOSP2PTransport] Connect failed: initial connection probe send failed.");
      Stop();
      OutPeerId = 0;
      return false;
    }
    M_LOG("[EOSP2PTransportTest] Initial connection probe sent: peer={}", OutPeerId);

    M_LOG(
        "[EOSP2PTransportTest] Connection target ready: remoteUser={} peer={} socket={} "
        "portUnused={}",
        Endpoint.HostName,
        OutPeerId,
        SocketName,
        Endpoint.Port
    );
    return true;
  }

  bool Service() override {
    CheckInterruptedTimeouts();
    if (!Running || !P2PHandle || !LocalUserId) {
      return false;
    }

    uint32_t PacketSize = 0;
    EOS_P2P_GetNextReceivedPacketSizeOptions SizeOptions = {};
    SizeOptions.ApiVersion = EOS_P2P_GETNEXTRECEIVEDPACKETSIZE_API_LATEST;
    SizeOptions.LocalUserId = LocalUserId;
    const EOS_EResult SizeResult =
        EOS_P2P_GetNextReceivedPacketSize(P2PHandle, &SizeOptions, &PacketSize);
    if (SizeResult == EOS_EResult::EOS_NotFound) {
      return false;
    }
    if (SizeResult != EOS_EResult::EOS_Success) {
      M_LOG("[EOSP2PTransport] GetNextReceivedPacketSize failed: {}", SafeEOSResult(SizeResult));
      return false;
    }
    if (PacketSize == 0 || PacketSize > EOS_P2P_MAX_PACKET_SIZE) {
      M_LOG(
          "[EOSP2PTransport] Invalid received size: bytes={} limit={}",
          PacketSize,
          EOS_P2P_MAX_PACKET_SIZE
      );
      return false;
    }

    std::vector<uint8_t> Data(PacketSize);
    EOS_ProductUserId RemoteUserId = nullptr;
    EOS_P2P_SocketId ReceivedSocket = {};
    uint8_t Channel = 0;
    uint32_t BytesWritten = PacketSize;
    EOS_P2P_ReceivePacketOptions Options = {};
    Options.ApiVersion = EOS_P2P_RECEIVEPACKET_API_LATEST;
    Options.LocalUserId = LocalUserId;
    Options.MaxDataSizeBytes = PacketSize;
    const EOS_EResult Result = EOS_P2P_ReceivePacket(
        P2PHandle, &Options, &RemoteUserId, &ReceivedSocket, &Channel, Data.data(), &BytesWritten
    );
    if (Result != EOS_EResult::EOS_Success) {
      M_LOG("[EOSP2PTransport] ReceivePacket failed: {}", SafeEOSResult(Result));
      return false;
    }
    if (!IsExpectedSocket(&ReceivedSocket)) {
      M_LOG("[EOSP2PTransport] Packet dropped: unexpected socket={}", ReceivedSocket.SocketName);
      return true;
    }
    if (Channel >= MaximumChannels) {
      M_LOG(
          "[EOSP2PTransport] Packet dropped: unknown channel={} configuredChannels={}",
          Channel,
          MaximumChannels
      );
      return true;
    }
    if (!RemoteUserId || BytesWritten == 0 || BytesWritten > PacketSize) {
      M_LOG(
          "[EOSP2PTransport] Packet dropped: invalid metadata bytes={} expectedMax={}",
          BytesWritten,
          PacketSize
      );
      return true;
    }

    const FNetworkPeerId PeerId = RegisterPeer(RemoteUserId);
    if (PeerId == 0) {
      M_LOG("[EOSP2PTransport] Packet dropped: sender registration failed.");
      return true;
    }

    Data.resize(BytesWritten);
    if (!LoggedReceive) {
      LoggedReceive = true;
      M_LOG(
          "[EOSP2PTransportTest] Receive verified: peer={} channel={} bytes={} socket={}",
          PeerId,
          Channel,
          BytesWritten,
          ReceivedSocket.SocketName
      );
    }
    if (OnPacketReceived) {
      FReceivedPacket Packet;
      Packet.Sender = PeerId;
      Packet.Channel = Channel;
      Packet.Data = std::move(Data);
      OnPacketReceived(std::move(Packet));
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
    if (!Running || !P2PHandle || !LocalUserId || !Data || Size == 0 ||
        InterruptedPeers.find(PeerId) != InterruptedPeers.end()) {
      M_LOG("[EOSP2PTransport] Send rejected: invalid transport or data. bytes={}", Size);
      return false;
    }
    if (Channel >= MaximumChannels) {
      M_LOG(
          "[EOSP2PTransport] Send rejected: channel={} configuredChannels={}",
          Channel,
          MaximumChannels
      );
      return false;
    }
    if (Size > EOS_P2P_MAX_PACKET_SIZE || Size > (std::numeric_limits<uint32_t>::max)()) {
      M_LOG(
          "[EOSP2PTransportTest] Oversize send rejected: bytes={} limit={}",
          Size,
          EOS_P2P_MAX_PACKET_SIZE
      );
      return false;
    }

    const auto Iterator = PeersById.find(PeerId);
    if (Iterator == PeersById.end() || !Iterator->second) {
      M_LOG("[EOSP2PTransport] Send rejected: unknown peer={}", PeerId);
      return false;
    }

    EOS_P2P_SendPacketOptions Options = {};
    Options.ApiVersion = EOS_P2P_SENDPACKET_API_LATEST;
    Options.LocalUserId = LocalUserId;
    Options.RemoteUserId = Iterator->second;
    Options.SocketId = &SocketId;
    Options.Channel = Channel;
    Options.DataLengthBytes = static_cast<uint32_t>(Size);
    Options.Data = Data;
    Options.bAllowDelayedDelivery = EOS_TRUE;
    Options.Reliability = ConvertReliability(Reliability);
    Options.bDisableAutoAcceptConnection = EOS_FALSE;
    const EOS_EResult Result = EOS_P2P_SendPacket(P2PHandle, &Options);
    if (Result != EOS_EResult::EOS_Success) {
      M_LOG(
          "[EOSP2PTransport] SendPacket failed: peer={} channel={} bytes={} result={}",
          PeerId,
          Channel,
          Size,
          SafeEOSResult(Result)
      );
      return false;
    }

    bool& ReliabilityLogged =
        Reliability == ENetworkReliability::Reliable ? LoggedReliableSend : LoggedUnreliableSend;
    if (!ReliabilityLogged) {
      ReliabilityLogged = true;
      M_LOG(
          "[EOSP2PTransportTest] Send verified: reliability={} peer={} channel={} bytes={}",
          Reliability == ENetworkReliability::Reliable ? "ReliableOrdered" : "UnreliableUnordered",
          PeerId,
          Channel,
          Size
      );
    }
    return true;
  }

  bool Broadcast(
      uint8_t Channel, ENetworkReliability Reliability, const uint8_t* Data, size_t Size
  ) override {
    bool SentAny = false;
    bool AllSucceeded = true;
    std::vector<FNetworkPeerId> PeerIds;
    PeerIds.reserve(PeersById.size());
    for (const auto& Pair : PeersById) {
      PeerIds.push_back(Pair.first);
    }
    for (FNetworkPeerId PeerId : PeerIds) {
      SentAny = true;
      AllSucceeded = Send(PeerId, Channel, Reliability, Data, Size) && AllSucceeded;
    }
    return SentAny && AllSucceeded;
  }

  void Disconnect(FNetworkPeerId PeerId) override {
    const auto Iterator = PeersById.find(PeerId);
    if (!P2PHandle || !LocalUserId || Iterator == PeersById.end() || !Iterator->second) {
      return;
    }

    EOS_P2P_CloseConnectionOptions Options = {};
    Options.ApiVersion = EOS_P2P_CLOSECONNECTION_API_LATEST;
    Options.LocalUserId = LocalUserId;
    Options.RemoteUserId = Iterator->second;
    Options.SocketId = &SocketId;
    const EOS_EResult Result = EOS_P2P_CloseConnection(P2PHandle, &Options);
    M_LOG("[EOSP2PTransport] CloseConnection: peer={} result={}", PeerId, SafeEOSResult(Result));
  }

  void Stop() override {
    if (!Running) {
      ClearState();
      return;
    }

    UnregisterNotifications();
    if (P2PHandle && LocalUserId) {
      EOS_P2P_CloseConnectionsOptions Options = {};
      Options.ApiVersion = EOS_P2P_CLOSECONNECTIONS_API_LATEST;
      Options.LocalUserId = LocalUserId;
      Options.SocketId = &SocketId;
      const EOS_EResult Result = EOS_P2P_CloseConnections(P2PHandle, &Options);
      M_LOG("[EOSP2PTransport] CloseConnections result={}", SafeEOSResult(Result));
    }
    ClearState();
    M_LOG("[EOSP2PTransportTest] Shutdown verified: notifications removed and peers cleared.");
  }

  bool IsRunning() const override { return Running; }

  std::string GetRemoteProductUserId(FNetworkPeerId PeerId) const override {
    const auto Iterator = PeersById.find(PeerId);
    return Iterator == PeersById.end() ? std::string{} : ProductUserIdToString(Iterator->second);
  }

  void SetConnectedCallback(ConnectedCallback Callback) override {
    OnConnected = std::move(Callback);
  }
  void SetDisconnectedCallback(DisconnectedCallback Callback) override {
    OnDisconnected = std::move(Callback);
  }
  void SetConnectionStateChangedCallback(ConnectionStateChangedCallback Callback) override {
    OnConnectionStateChanged = std::move(Callback);
  }
  void SetPacketReceivedCallback(PacketReceivedCallback Callback) override {
    OnPacketReceived = std::move(Callback);
  }
  void SetPeerAuthorizationCallback(PeerAuthorizationCallback Callback) override {
    IsPeerAuthorized = std::move(Callback);
  }

 private:
  bool Initialize(size_t MaxConnections, size_t ChannelCount) {
    P2PHandle = EOSCoreManager::GetInstance().GetP2PHandle();
    LocalUserId = EOSAuthManager::GetInstance().GetLocalUserId();
    if (!EOSCoreManager::GetInstance().IsInitialized() || !P2PHandle) {
      M_LOG("[EOSP2PTransport] Initialize failed: EOS P2P handle is unavailable.");
      ClearState();
      return false;
    }
    if (!LocalUserId || !EOSAuthManager::GetInstance().IsLoggedIn()) {
      M_LOG("[EOSP2PTransport] Initialize failed: local Product User ID is unavailable.");
      ClearState();
      return false;
    }
    if (ChannelCount == 0 || ChannelCount > 256) {
      M_LOG("[EOSP2PTransport] Initialize failed: invalid channel count={}", ChannelCount);
      ClearState();
      return false;
    }

    MaximumConnections = (std::min)(MaxConnections, static_cast<size_t>(EOS_P2P_MAX_CONNECTIONS));
    MaximumChannels = ChannelCount;
    if (MaximumConnections == 0 || !RegisterNotifications()) {
      M_LOG("[EOSP2PTransport] Initialize failed: notification registration failed.");
      UnregisterNotifications();
      ClearState();
      return false;
    }

    Running = true;
    M_LOG(
        "[EOSP2PTransportTest] Initialized: localUser={} p2pHandle={} socket={} notifications=4",
        ProductUserIdToString(LocalUserId),
        static_cast<const void*>(P2PHandle),
        SocketName
    );
    return true;
  }

  bool RegisterNotifications() {
    UnregisterNotifications();

    EOS_P2P_AddNotifyPeerConnectionRequestOptions RequestOptions = {};
    RequestOptions.ApiVersion = EOS_P2P_ADDNOTIFYPEERCONNECTIONREQUEST_API_LATEST;
    RequestOptions.LocalUserId = LocalUserId;
    RequestOptions.SocketId = &SocketId;
    RequestNotificationId = EOS_P2P_AddNotifyPeerConnectionRequest(
        P2PHandle, &RequestOptions, this, &FEOSP2PTransport::HandleConnectionRequest
    );

    EOS_P2P_AddNotifyPeerConnectionEstablishedOptions EstablishedOptions = {};
    EstablishedOptions.ApiVersion = EOS_P2P_ADDNOTIFYPEERCONNECTIONESTABLISHED_API_LATEST;
    EstablishedOptions.LocalUserId = LocalUserId;
    EstablishedOptions.SocketId = &SocketId;
    EstablishedNotificationId = EOS_P2P_AddNotifyPeerConnectionEstablished(
        P2PHandle, &EstablishedOptions, this, &FEOSP2PTransport::HandleConnectionEstablished
    );

    EOS_P2P_AddNotifyPeerConnectionInterruptedOptions InterruptedOptions = {};
    InterruptedOptions.ApiVersion = EOS_P2P_ADDNOTIFYPEERCONNECTIONINTERRUPTED_API_LATEST;
    InterruptedOptions.LocalUserId = LocalUserId;
    InterruptedOptions.SocketId = &SocketId;
    InterruptedNotificationId = EOS_P2P_AddNotifyPeerConnectionInterrupted(
        P2PHandle, &InterruptedOptions, this, &FEOSP2PTransport::HandleConnectionInterrupted
    );

    EOS_P2P_AddNotifyPeerConnectionClosedOptions ClosedOptions = {};
    ClosedOptions.ApiVersion = EOS_P2P_ADDNOTIFYPEERCONNECTIONCLOSED_API_LATEST;
    ClosedOptions.LocalUserId = LocalUserId;
    ClosedOptions.SocketId = &SocketId;
    ClosedNotificationId = EOS_P2P_AddNotifyPeerConnectionClosed(
        P2PHandle, &ClosedOptions, this, &FEOSP2PTransport::HandleConnectionClosed
    );

    return RequestNotificationId != EOS_INVALID_NOTIFICATIONID &&
           EstablishedNotificationId != EOS_INVALID_NOTIFICATIONID &&
           InterruptedNotificationId != EOS_INVALID_NOTIFICATIONID &&
           ClosedNotificationId != EOS_INVALID_NOTIFICATIONID;
  }

  void UnregisterNotifications() {
    if (P2PHandle) {
      if (RequestNotificationId != EOS_INVALID_NOTIFICATIONID) {
        EOS_P2P_RemoveNotifyPeerConnectionRequest(P2PHandle, RequestNotificationId);
      }
      if (EstablishedNotificationId != EOS_INVALID_NOTIFICATIONID) {
        EOS_P2P_RemoveNotifyPeerConnectionEstablished(P2PHandle, EstablishedNotificationId);
      }
      if (InterruptedNotificationId != EOS_INVALID_NOTIFICATIONID) {
        EOS_P2P_RemoveNotifyPeerConnectionInterrupted(P2PHandle, InterruptedNotificationId);
      }
      if (ClosedNotificationId != EOS_INVALID_NOTIFICATIONID) {
        EOS_P2P_RemoveNotifyPeerConnectionClosed(P2PHandle, ClosedNotificationId);
      }
    }
    RequestNotificationId = EOS_INVALID_NOTIFICATIONID;
    EstablishedNotificationId = EOS_INVALID_NOTIFICATIONID;
    InterruptedNotificationId = EOS_INVALID_NOTIFICATIONID;
    ClosedNotificationId = EOS_INVALID_NOTIFICATIONID;
  }

  FNetworkPeerId RegisterPeer(EOS_ProductUserId UserId) {
    const std::string UserIdString = ProductUserIdToString(UserId);
    if (UserIdString.empty()) {
      return 0;
    }
    const auto Existing = PeerIdsByUserId.find(UserIdString);
    if (Existing != PeerIdsByUserId.end()) {
      return Existing->second;
    }
    const FNetworkPeerId PeerId = NextPeerId++;
    PeersById[PeerId] = UserId;
    PeerIdsByUserId[UserIdString] = PeerId;
    return PeerId;
  }

  FNetworkPeerId FindPeerId(EOS_ProductUserId UserId) const {
    const auto Iterator = PeerIdsByUserId.find(ProductUserIdToString(UserId));
    return Iterator == PeerIdsByUserId.end() ? 0 : Iterator->second;
  }

  void RemovePeer(FNetworkPeerId PeerId) {
    const auto Iterator = PeersById.find(PeerId);
    if (Iterator != PeersById.end()) {
      PeerIdsByUserId.erase(ProductUserIdToString(Iterator->second));
      PeersById.erase(Iterator);
    }
    ConnectedPeers.erase(PeerId);
    InterruptedPeers.erase(PeerId);
  }

  void ClearState() {
    Running = false;
    Hosting = false;
    P2PHandle = nullptr;
    LocalUserId = nullptr;
    MaximumConnections = 0;
    MaximumChannels = 0;
    NextPeerId = 1;
    LoggedReliableSend = false;
    LoggedUnreliableSend = false;
    LoggedReceive = false;
    PeersById.clear();
    PeerIdsByUserId.clear();
    ConnectedPeers.clear();
    InterruptedPeers.clear();
  }

  void OnConnectionRequest(const EOS_P2P_OnIncomingConnectionRequestInfo* Data) {
    if (!Data || !Running || !Hosting || !IsExpectedSocket(Data->SocketId)) {
      M_LOG("[EOSP2PTransport] Incoming connection rejected: invalid state or socket.");
      return;
    }
    const std::string RemoteUserId = ProductUserIdToString(Data->RemoteUserId);
    if (RemoteUserId.empty() || !IsPeerAuthorized || !IsPeerAuthorized(RemoteUserId)) {
      M_LOG(
          "[EOSP2PTransportTest] Incoming connection rejected: unauthorized remoteUser={}",
          RemoteUserId.empty() ? "<invalid>" : RemoteUserId
      );
      return;
    }
    if (PeersById.size() >= MaximumConnections && FindPeerId(Data->RemoteUserId) == 0) {
      M_LOG(
          "[EOSP2PTransportTest] Incoming connection rejected: maximum peers reached "
          "remoteUser={} maxPeers={}",
          RemoteUserId,
          MaximumConnections
      );
      return;
    }

    EOS_P2P_AcceptConnectionOptions Options = {};
    Options.ApiVersion = EOS_P2P_ACCEPTCONNECTION_API_LATEST;
    Options.LocalUserId = LocalUserId;
    Options.RemoteUserId = Data->RemoteUserId;
    Options.SocketId = &SocketId;
    const EOS_EResult Result = EOS_P2P_AcceptConnection(P2PHandle, &Options);
    const FNetworkPeerId PeerId =
        Result == EOS_EResult::EOS_Success ? RegisterPeer(Data->RemoteUserId) : 0;
    M_LOG(
        "[EOSP2PTransportTest] Incoming request: remoteUser={} peer={} result={}",
        ProductUserIdToString(Data->RemoteUserId),
        PeerId,
        SafeEOSResult(Result)
    );
  }

  void OnConnectionEstablished(const EOS_P2P_OnPeerConnectionEstablishedInfo* Data) {
    if (!Data || !Running || !IsExpectedSocket(Data->SocketId)) {
      return;
    }
    const FNetworkPeerId PeerId = RegisterPeer(Data->RemoteUserId);
    if (PeerId == 0) {
      return;
    }
    const bool bWasInterrupted = InterruptedPeers.erase(PeerId) != 0;
    const bool bIsNewConnection = ConnectedPeers.insert(PeerId).second;
    if (!bIsNewConnection && !bWasInterrupted) {
      return;
    }
    if (bWasInterrupted && OnConnectionStateChanged) {
      OnConnectionStateChanged(PeerId, ETransportConnectionState::Connected);
    }
    M_LOG(
        "[EOSP2PTransportTest] Connection established: remoteUser={} peer={} connectionType={} "
        "networkType={}",
        ProductUserIdToString(Data->RemoteUserId),
        PeerId,
        static_cast<int>(Data->ConnectionType),
        static_cast<int>(Data->NetworkType)
    );
    if (bIsNewConnection && OnConnected) {
      OnConnected(PeerId);
    }
  }

  void OnConnectionInterrupted(const EOS_P2P_OnPeerConnectionInterruptedInfo* Data) {
    if (!Data || !Running || !IsExpectedSocket(Data->SocketId)) {
      return;
    }
    const FNetworkPeerId PeerId = FindPeerId(Data->RemoteUserId);
    if (PeerId == 0 || ConnectedPeers.find(PeerId) == ConnectedPeers.end() ||
        InterruptedPeers.find(PeerId) != InterruptedPeers.end()) {
      return;
    }
    InterruptedPeers[PeerId] = std::chrono::steady_clock::now();
    if (OnConnectionStateChanged) {
      OnConnectionStateChanged(PeerId, ETransportConnectionState::Interrupted);
    }
    M_LOG(
        "[EOSP2PTransportTest] Connection interrupted: remoteUser={} peer={}",
        ProductUserIdToString(Data->RemoteUserId),
        PeerId
    );
  }

  void OnConnectionClosed(const EOS_P2P_OnRemoteConnectionClosedInfo* Data) {
    if (!Data || !IsExpectedSocket(Data->SocketId)) {
      return;
    }
    const FNetworkPeerId PeerId = FindPeerId(Data->RemoteUserId);
    const bool WasConnected = ConnectedPeers.find(PeerId) != ConnectedPeers.end();
    M_LOG(
        "[EOSP2PTransportTest] Connection closed: remoteUser={} peer={} reason={}",
        ProductUserIdToString(Data->RemoteUserId),
        PeerId,
        static_cast<int>(Data->Reason)
    );
    RemovePeer(PeerId);
    if (WasConnected && OnDisconnected) {
      OnDisconnected(PeerId, GetDisconnectReason(Data->Reason));
    }
  }

  static ESessionDisconnectReason GetDisconnectReason(EOS_EConnectionClosedReason Reason) {
    switch (Reason) {
      case EOS_EConnectionClosedReason::EOS_CCR_ClosedByLocalUser:
        return ESessionDisconnectReason::LocalLeave;
      case EOS_EConnectionClosedReason::EOS_CCR_ClosedByPeer:
      case EOS_EConnectionClosedReason::EOS_CCR_ConnectionClosed:
        return ESessionDisconnectReason::RemoteLeave;
      case EOS_EConnectionClosedReason::EOS_CCR_TimedOut:
        return ESessionDisconnectReason::ConnectionTimeout;
      case EOS_EConnectionClosedReason::EOS_CCR_TooManyConnections:
      case EOS_EConnectionClosedReason::EOS_CCR_ConnectionIgnored:
        return ESessionDisconnectReason::UnauthorizedPeer;
      case EOS_EConnectionClosedReason::EOS_CCR_Unknown:
      case EOS_EConnectionClosedReason::EOS_CCR_InvalidMessage:
      case EOS_EConnectionClosedReason::EOS_CCR_InvalidData:
      case EOS_EConnectionClosedReason::EOS_CCR_ConnectionFailed:
      case EOS_EConnectionClosedReason::EOS_CCR_NegotiationFailed:
      case EOS_EConnectionClosedReason::EOS_CCR_UnexpectedError:
      default:
        return ESessionDisconnectReason::TransportError;
    }
  }

  void CheckInterruptedTimeouts() {
    if (InterruptedPeers.empty()) {
      return;
    }
    const auto Now = std::chrono::steady_clock::now();
    std::vector<FNetworkPeerId> TimedOutPeers;
    for (const auto& Pair : InterruptedPeers) {
      if (Now - Pair.second >= InterruptedConnectionTimeout) {
        TimedOutPeers.push_back(Pair.first);
      }
    }
    for (FNetworkPeerId PeerId : TimedOutPeers) {
      M_LOG(
          "[EOSP2PTransportTest] Interrupted connection timed out: peer={} timeoutSeconds={}",
          PeerId,
          InterruptedConnectionTimeout.count()
      );
      Disconnect(PeerId);
      const bool bWasConnected = ConnectedPeers.find(PeerId) != ConnectedPeers.end();
      RemovePeer(PeerId);
      if (bWasConnected && OnDisconnected) {
        OnDisconnected(PeerId, ESessionDisconnectReason::ConnectionTimeout);
      }
    }
  }

  static void EOS_CALL
  HandleConnectionRequest(const EOS_P2P_OnIncomingConnectionRequestInfo* Data) {
    auto* Transport = static_cast<FEOSP2PTransport*>(Data ? Data->ClientData : nullptr);
    if (Transport) {
      Transport->OnConnectionRequest(Data);
    }
  }

  static void EOS_CALL
  HandleConnectionEstablished(const EOS_P2P_OnPeerConnectionEstablishedInfo* Data) {
    auto* Transport = static_cast<FEOSP2PTransport*>(Data ? Data->ClientData : nullptr);
    if (Transport) {
      Transport->OnConnectionEstablished(Data);
    }
  }

  static void EOS_CALL
  HandleConnectionInterrupted(const EOS_P2P_OnPeerConnectionInterruptedInfo* Data) {
    auto* Transport = static_cast<FEOSP2PTransport*>(Data ? Data->ClientData : nullptr);
    if (Transport) {
      Transport->OnConnectionInterrupted(Data);
    }
  }

  static void EOS_CALL HandleConnectionClosed(const EOS_P2P_OnRemoteConnectionClosedInfo* Data) {
    auto* Transport = static_cast<FEOSP2PTransport*>(Data ? Data->ClientData : nullptr);
    if (Transport) {
      Transport->OnConnectionClosed(Data);
    }
  }

  EOS_HP2P P2PHandle = nullptr;
  EOS_ProductUserId LocalUserId = nullptr;
  EOS_P2P_SocketId SocketId = {};
  bool Running = false;
  bool Hosting = false;
  size_t MaximumConnections = 0;
  size_t MaximumChannels = 0;
  FNetworkPeerId NextPeerId = 1;
  bool LoggedReliableSend = false;
  bool LoggedUnreliableSend = false;
  bool LoggedReceive = false;
  EOS_NotificationId RequestNotificationId = EOS_INVALID_NOTIFICATIONID;
  EOS_NotificationId EstablishedNotificationId = EOS_INVALID_NOTIFICATIONID;
  EOS_NotificationId InterruptedNotificationId = EOS_INVALID_NOTIFICATIONID;
  EOS_NotificationId ClosedNotificationId = EOS_INVALID_NOTIFICATIONID;
  std::unordered_map<FNetworkPeerId, EOS_ProductUserId> PeersById;
  std::unordered_map<std::string, FNetworkPeerId> PeerIdsByUserId;
  std::unordered_set<FNetworkPeerId> ConnectedPeers;
  ConnectedCallback OnConnected;
  std::unordered_map<FNetworkPeerId, std::chrono::steady_clock::time_point> InterruptedPeers;
  DisconnectedCallback OnDisconnected;
  PacketReceivedCallback OnPacketReceived;
  ConnectionStateChangedCallback OnConnectionStateChanged;
  PeerAuthorizationCallback IsPeerAuthorized;
};
}  // namespace

std::unique_ptr<INetworkTransport> CreateEOSP2PTransport() {
  return std::make_unique<FEOSP2PTransport>();
}
