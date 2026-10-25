#pragma once
#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>
#include <string_view>

#include "BroccoliEngineAPI.h"
#include "NetBuffer.h"
#include "NetworkTransport.h"
#include "NetworkTypes.h"

/// @brief サーバーの開始、接続、パケットの送受信を管理
/// 通信処理はINetworkTransportが行う
class BROCCOLI_ENGINE_API NetworkManager {
 public:
  using ConnectedCallback = std::function<void(FNetworkConnectionId)>;
  using DisconnectedCallback = std::function<void(FNetworkConnectionId, ESessionDisconnectReason)>;
  using PacketReceivedCallback = std::function<void(FNetworkConnectionId, FNetBuffer&)>;
  using CallbackHandle = size_t;

  static NetworkManager& GetInstance();

  /// @brief 使用するトランスポートを名前で選択する。
  /// @param Name レジストリに登録済みのトランスポート名
  /// @return
  bool SetTransportName(std::string_view Name);
  const std::string& GetTransportName() const;
  /// @brief サーバーとして通信を開始し、クライアントの接続を受け付ける。
  /// @param Port 通信するポート
  /// @param MaxConnections 最大接続クライアント数
  /// @param ChannelCount 通信チャンネル数 デフォルト2 (0=重要なRPC,1=移動同期など)
  /// @return
  bool StartServer(uint16_t Port, size_t MaxConnections = 32, size_t ChannelCount = 2);

  /// @brief クライアントとしてサーバーとの接続を開始する。
  /// @param HostName 接続対象
  /// @param Port 接続ポート
  /// @param ChannelCount 通信チャンネル数 デフォルト2 (0=重要なRPC,1=移動同期など)
  /// @return
  bool ConnectToServer(const std::string& HostName, uint16_t Port, size_t ChannelCount = 2);

  /// @brief メインUpdateで呼ばれる関数
  ///接続状態確認、トランスポートクラスの更新を行う。
  void Service();

  /// @brief 新たな送信を禁止し、接続相手との切断を要求する。
  void Disconnect();
  /// @brief トランスポートを停止、破棄、接続情報をクリアする。
  void Stop();

  /// @brief INetworkTransportを使用しサーバーに送信する。
  /// @param Buffer 送信するバッファー
  /// @param Reliability 到達順序保障
  /// @param ChannelId 送信するチャンネル デフォルト0
  /// @return
  bool SendToServer(
      const FNetBuffer& Buffer,
      ENetPacketReliability Reliability = ENetPacketReliability::Reliable,
      uint8_t ChannelId = 0
  );
  /// @brief INetworkTransportを使用し特定のクライアントに対してバッファーを送信する
  /// @param ConnectionId 送信するクライアントの接続Id
  /// @param Buffer 送信するバッファー
  /// @param Reliability 到達順序保障
  /// @param ChannelId 送信するチャンネル デフォルト0
  /// @return
  bool SendToClient(
      FNetworkConnectionId ConnectionId,
      const FNetBuffer& Buffer,
      ENetPacketReliability Reliability = ENetPacketReliability::Reliable,
      uint8_t ChannelId = 0
  );
  /// @brief INetworkTransportを使用し接続中のクライアント全員にバッファーを送信する
  /// @param Buffer 送信するバッファー
  /// @param Reliability 到達順序保障
  /// @param ChannelId 送信するチャンネル デフォルト0
  /// @return
  bool Broadcast(
      const FNetBuffer& Buffer,
      ENetPacketReliability Reliability = ENetPacketReliability::Reliable,
      uint8_t ChannelId = 0
  );

  bool IsRunning() const;
  bool IsServer() const { return bIsServer; }
  bool IsClient() const { return bIsClient; }
  /// @brief 接続中のクライアント数を取得（サーバーでのみ有効）
  /// @return
  size_t GetConnectedClientCount() const;
  FNetworkConnectionId GetLocalConnectionId() const { return LocalConnectionId; }
  /// @brief 想定される現在のサーバーの時刻を取得
  /// @return
  double GetEstimatedServerTime() const;
  /// @brief サーバーとの時刻の差を取得
  /// @return
  double GetServerTimeOffset() const { return ServerTimeOffset; }

  void SetLocalConnectionId(FNetworkConnectionId ConnectionId) { LocalConnectionId = ConnectionId; }

  /// @brief 接続確立時に呼ばれるコールバックを追加登録する。
  /// @param Callback
  /// @return
  CallbackHandle AddOnConnected(ConnectedCallback Callback);
  /// @brief 接続切断時に呼ばれるコールバックを追加登録する。
  /// @param Callback
  /// @return
  CallbackHandle AddOnDisconnected(DisconnectedCallback Callback);
  /// @brief パケット受信時に呼ばれるコールバックを追加登録する。
  /// @param Callback
  /// @return
  CallbackHandle AddOnPacketReceived(PacketReceivedCallback Callback);
  void RemoveOnConnected(CallbackHandle Handle);
  void RemoveOnDisconnected(CallbackHandle Handle);
  void RemoveOnPacketReceived(CallbackHandle Handle);

 private:
  NetworkManager();
  ~NetworkManager();
  NetworkManager(const NetworkManager&) = delete;
  NetworkManager& operator=(const NetworkManager&) = delete;

  /// @brief id指定でバッファーを送信する。
  /// @param PeerId 接続id
  /// @param Buffer 送信するバッファー
  /// @param Reliability 到達順序保障
  /// @param ChannelId 送信するチャンネル デフォルト0
  /// @return
  bool SendToPeer(
      FNetworkPeerId PeerId,
      const FNetBuffer& Buffer,
      ENetPacketReliability Reliability,
      uint8_t ChannelId
  );
  /// @brief 通信を担当するトランスポートクラスの生成、初期化を行う。
  /// @return
  bool CreateSelectedTransport();
  /// @brief トランスポートクラスの生成後、トランスポートクラスのコールバックを設定する。
  void BindTransportCallbacks();
  /// @brief 新しい接続相手をPeersByConnectionIdなどに登録し、トランスポート側のPeerIdとエンジン側の接続IDを対応付ける。
  /// @param PeerId
  /// @return
  FNetworkConnectionId RegisterPeer(FNetworkPeerId PeerId);
  /// @brief peeridからFNetworkConnectionIdを取得する。
  /// @param PeerId
  /// @return
  FNetworkConnectionId FindConnectionId(FNetworkPeerId PeerId) const;
  void RemovePeer(FNetworkPeerId PeerId);
  /// @brief マネージャー停止時に呼ばれる。登録された接続相手リストをクリアする。
  void ClearPeers();
  /// @brief トランスポートクラスで相手との接続開始時に呼ばれる。
  /// @param PeerId
  void HandleTransportConnected(FNetworkPeerId PeerId);
  /// @brief トランスポートクラスで相手との接続切断時に呼ばれる。
  /// @param PeerId
  /// @param Reason
  void HandleTransportDisconnected(FNetworkPeerId PeerId, ESessionDisconnectReason Reason);
  /// @brief トランスポートクラスで相手との接続状態変更時に呼ばれる。
  /// @param PeerId
  /// @param State
  void HandleTransportConnectionStateChanged(
      FNetworkPeerId PeerId, ETransportConnectionState State
  );
  /// @brief トランスポートクラスでパケット受信時に呼ばれる。
  /// @param Packet
  void HandleTransportPacket(FReceivedPacket&& Packet);
  /// @brief マネージャーに登録された接続開始時コールバックを実行。
  /// @param ConnectionId
  void BroadcastConnected(FNetworkConnectionId ConnectionId);
  /// @brief マネージャーに登録された接続切断時コールバックを実行。
  /// @param ConnectionId
  /// @param Reason
  void BroadcastDisconnected(FNetworkConnectionId ConnectionId, ESessionDisconnectReason Reason);
  /// @brief マネージャーに登録されたパケット受信時コールバックを実行。
  /// @param ConnectionId
  /// @param Buffer
  void BroadcastPacketReceived(FNetworkConnectionId ConnectionId, FNetBuffer& Buffer);

  bool bIsServer = false;
  bool bIsClient = false;
  bool bIsServicing = false;
  bool bStopRequested = false;
  FNetworkConnectionId NextConnectionId = 1;
  FNetworkConnectionId ServerConnectionId = 1;
  FNetworkConnectionId LocalConnectionId = 0;
  double ServerTimeOffset = 0.0;

  struct Impl;
  Impl* ImplPtr = nullptr;
};
