#pragma once

#include <cstdint>

using FNetworkActorId = uint32_t;
using FNetworkComponentId = uint32_t;
using FNetworkConnectionId = uint32_t;
using FNetworkRPCId = uint32_t;
using FNetworkSceneId = uint32_t;

enum class ENetRPCType : uint8_t { Server, Client, Multicast };
