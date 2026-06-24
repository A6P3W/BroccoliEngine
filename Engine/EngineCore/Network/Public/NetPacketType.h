#pragma once

#include <cstdint>

enum class ENetPacketType : uint8_t
{
	None = 0,
	ActorSpawn,
	ActorDestroy,
	ActorState,
	ActorRPC,
	PlayerInput
};
