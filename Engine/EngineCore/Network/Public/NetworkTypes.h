#pragma once

#include <cstdint>

#include "UMath.h"

using FNetworkActorId = uint32_t;
using FNetworkComponentId = uint32_t;
using FNetworkConnectionId = uint32_t;
using FNetworkRPCId = uint32_t;
using FNetworkSceneId = uint32_t;

enum class ENetRPCType : uint8_t { Server, Client, Multicast };

using FMoveActionState = uint32_t;

namespace EMoveAction {
enum : FMoveActionState {
  None = 0,
};
}

struct FMovePredictionData {
  uint32_t Sequence = 0;
  float DeltaTime = 0.0f;
  FVector2D InputAxis = {0.0f, 0.0f};
  FRotator StartRotation = FRotator(0.0f);
  FMoveActionState Actions = EMoveAction::None;
};
