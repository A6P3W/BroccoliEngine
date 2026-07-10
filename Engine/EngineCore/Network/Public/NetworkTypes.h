#pragma once

#include <cstdint>

#include "UMath.h"

using FNetworkActorId = uint32_t;
using FNetworkComponentId = uint32_t;
using FNetworkConnectionId = uint32_t;
using FNetworkRPCId = uint32_t;
using FNetworkSceneId = uint32_t;

enum class ENetRPCType : uint8_t { Server, Client, Multicast };

struct FMovePredictionData {
  uint32_t Sequence = 0;
  float DeltaTime = 0.0f;
  FVector2D CurrentForce = {0.0f, 0.0f};
  FVector2D Velocity = {0.0f, 0.0f};
  bool bUseVelocityOverride = false;
  FRotator StartRotation = FRotator(0.0f);
  FRotator VelocityRotation = FRotator(0.0f);
};
