#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "BroccoliEngineAPI.h"

using FNetworkPeerId = uint64_t;

enum class ENetworkReliability : uint8_t { Unreliable, Reliable };

enum class ENetworkTransportType : uint8_t { ENet };

struct BROCCOLI_ENGINE_API FNetworkEndpoint {
  std::string HostName;
  uint16_t Port = 0;
};

struct BROCCOLI_ENGINE_API FReceivedPacket {
  FNetworkPeerId Sender = 0;
  uint8_t Channel = 0;
  std::vector<uint8_t> Data;
};

class BROCCOLI_ENGINE_API INetworkTransport {
 public:
  using ConnectedCallback = std::function<void(FNetworkPeerId)>;
  using DisconnectedCallback = std::function<void(FNetworkPeerId)>;
  using PacketReceivedCallback = std::function<void(FReceivedPacket&&)>;

  virtual ~INetworkTransport() = default;

  virtual bool StartHost(uint16_t Port, size_t MaxConnections, size_t ChannelCount) = 0;
  virtual bool Connect(
      const FNetworkEndpoint& Endpoint, size_t ChannelCount, FNetworkPeerId& OutPeerId
  ) = 0;
  virtual bool Service() = 0;

  virtual bool Send(
      FNetworkPeerId PeerId,
      uint8_t Channel,
      ENetworkReliability Reliability,
      const uint8_t* Data,
      size_t Size
  ) = 0;
  virtual bool Broadcast(
      uint8_t Channel, ENetworkReliability Reliability, const uint8_t* Data, size_t Size
  ) = 0;

  virtual void Disconnect(FNetworkPeerId PeerId) = 0;
  virtual void Stop() = 0;
  virtual bool IsRunning() const = 0;

  virtual void SetConnectedCallback(ConnectedCallback Callback) = 0;
  virtual void SetDisconnectedCallback(DisconnectedCallback Callback) = 0;
  virtual void SetPacketReceivedCallback(PacketReceivedCallback Callback) = 0;
};

BROCCOLI_ENGINE_API std::unique_ptr<INetworkTransport> CreateNetworkTransport(
    ENetworkTransportType Type
);
