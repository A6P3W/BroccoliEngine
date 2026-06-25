#pragma once

#include <cstdint>

enum class ENetPacketType : uint8_t
{
	None = 0,
	AssignNetId,
	ActorSpawn,
	ActorDestroy,
	ActorState,
	ActorRPC,
	PlayerInput,
	ServerTravel,
	ClientTravelReady
};
