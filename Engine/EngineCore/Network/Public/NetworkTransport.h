#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "BroccoliEngineAPI.h"
#include "NetworkTypes.h"

/// @brief 通信相手を指すためのトランスポート用番号。
using FNetworkPeerId = uint64_t;

/// @brief パケットの到達保障 保証なし/順序,到達保証
enum class ENetPacketReliability : uint8_t { Unreliable, Reliable };

/// @brief 接続相手との接続状態
enum class ETransportConnectionState : uint8_t {
  Connected,   //正常に通信中
  Interrupted  //一時的に接続が途絶えている
};

/// @brief 接続先の宛先情報
struct BROCCOLI_ENGINE_API FNetworkEndpoint {
  std::string Address;  //接続先アドレス/ユーザーID
  uint16_t Port = 0;    //ポート番号
};

/// @brief NetworkManagerへの受け渡し時のみ使用される加工前受信データ
struct BROCCOLI_ENGINE_API FReceivedPacket {
  FNetworkPeerId Sender = 0;
  uint8_t Channel = 0;
  std::vector<uint8_t> Data;
};

/// @brief 通信方法のベースクラス
class BROCCOLI_ENGINE_API INetworkTransport {
 public:
  using ConnectedCallback = std::function<void(FNetworkPeerId)>;
  using DisconnectedCallback = std::function<void(FNetworkPeerId, ESessionDisconnectReason)>;
  using ConnectionStateChangedCallback =
      std::function<void(FNetworkPeerId, ETransportConnectionState)>;
  using PacketReceivedCallback = std::function<void(FReceivedPacket&&)>;

  virtual ~INetworkTransport() = default;

  virtual bool StartHost(uint16_t Port, size_t MaxConnections, size_t ChannelCount) = 0;
  virtual bool Connect(
      const FNetworkEndpoint& Endpoint, size_t ChannelCount, FNetworkPeerId& OutPeerId
  ) = 0;
  /// @brief NetworkManagerから呼ばれるトランスポート更新関数。
  /// @return
  virtual bool Service() = 0;

  /// @brief NetworkManagerから呼ばれる接続相手指定パケット送信関数。
  /// @param PeerId
  /// @param Channel
  /// @param Reliability
  /// @param Data
  /// @param Size
  /// @return
  virtual bool Send(
      FNetworkPeerId PeerId,
      uint8_t Channel,
      ENetPacketReliability Reliability,
      const uint8_t* Data,
      size_t Size
  ) = 0;
  /// @brief NetworkManagerから呼ばれるクライアント全員へのパケット送信関数。
  /// @param Channel
  /// @param Reliability
  /// @param Data
  /// @param Size
  /// @return
  virtual bool Broadcast(
      uint8_t Channel, ENetPacketReliability Reliability, const uint8_t* Data, size_t Size
  ) = 0;

  virtual void Disconnect(FNetworkPeerId PeerId) = 0;
  virtual void Stop() = 0;
  virtual bool IsRunning() const = 0;

  virtual void SetConnectedCallback(ConnectedCallback Callback) = 0;
  virtual void SetDisconnectedCallback(DisconnectedCallback Callback) = 0;
  virtual void SetPacketReceivedCallback(PacketReceivedCallback Callback) = 0;
  virtual void SetConnectionStateChangedCallback(ConnectionStateChangedCallback Callback) = 0;
};
