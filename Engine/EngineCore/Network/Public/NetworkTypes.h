#pragma once

#include <cstdint>

#include "UMath.h"

using FNetworkActorId = uint32_t;
using FNetworkComponentId = uint32_t;
using FNetworkConnectionId = uint32_t;
using FNetworkRPCId = uint32_t;
using FNetworkSceneId = uint32_t;

enum class ENetRPCType : uint8_t { Server, Client, Multicast };

enum EMoveFlags : uint8_t {
  FLAG_None = 0,
  FLAG_Jump = 1 << 0,
  FLAG_Dash = 1 << 1,
};

struct FSavedMove {
  uint32_t Sequence = 0;
  float DeltaTime = 0.0f;
  FVector2D InputAxis = {0.0f, 0.0f};
  uint8_t SavedFlags = 0;
};
