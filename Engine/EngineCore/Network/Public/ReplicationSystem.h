#pragma once

#include "NetworkTypes.h"

#include <unordered_map>

class AActor;
class FNetBuffer;
class World;

class MReplicationSystem
{
public:
	explicit MReplicationSystem(World* InWorld);
	~MReplicationSystem();

	void Update();

	AActor* FindActor(FNetworkActorId NetworkId) const;
	void RegisterActor(AActor* Actor);
	void UnregisterActor(FNetworkActorId NetworkId);
	void Clear();

private:
	void HandleConnected(FNetworkConnectionId ConnectionId);
	void HandleDisconnected(FNetworkConnectionId ConnectionId);
	void HandlePacketReceived(FNetworkConnectionId ConnectionId, FNetBuffer& Buffer);

	void HandleActorSpawn(FNetBuffer& Buffer);
	void HandleActorState(FNetBuffer& Buffer);
	void HandleActorDestroy(FNetBuffer& Buffer);

	void SendInitialStateToClient(FNetworkConnectionId ConnectionId);
	void SendActorSpawn(AActor* Actor, FNetworkConnectionId TargetConnectionId = 0);
	void SendActorState(AActor* Actor);
	void SendActorDestroy(FNetworkActorId NetworkId);

	bool EnsureServerActorRegistered(AActor* Actor);

	World* OwnerWorld = nullptr;
	std::unordered_map<FNetworkActorId, AActor*> ActorsByNetworkId;
};
