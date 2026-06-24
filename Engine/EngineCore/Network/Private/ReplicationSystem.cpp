#include "ReplicationSystem.h"

#include "Actor.h"
#include "ActorRegistry.h"
#include "GameModeBase.h"
#include "NetBuffer.h"
#include "NetPacketType.h"
#include "NetworkManager.h"
#include "ObjectManager.h"
#include "Pawn.h"
#include "World.h"

#include <string>
#include <cstdint>

namespace
{
	bool IsReplicationSequenceNewer(uint32_t IncomingSequence, uint32_t CurrentSequence)
	{
		return static_cast<int32_t>(IncomingSequence - CurrentSequence) > 0;
	}
}

MReplicationSystem::MReplicationSystem(World* InWorld)
	: OwnerWorld(InWorld)
{
	NetworkManager& network = NetworkManager::GetInstance();
	network.SetOnConnected([this](FNetworkConnectionId ConnectionId) {
		HandleConnected(ConnectionId);
	});
	network.SetOnDisconnected([this](FNetworkConnectionId ConnectionId) {
		HandleDisconnected(ConnectionId);
	});
	network.SetOnPacketReceived([this](FNetworkConnectionId ConnectionId, FNetBuffer& Buffer) {
		HandlePacketReceived(ConnectionId, Buffer);
	});
}

MReplicationSystem::~MReplicationSystem()
{
	NetworkManager& network = NetworkManager::GetInstance();
	network.SetOnConnected({});
	network.SetOnDisconnected({});
	network.SetOnPacketReceived({});
}

void MReplicationSystem::Update()
{
	if (!OwnerWorld || !OwnerWorld->IsServer() || !OwnerWorld->GetObjectManager()) {
		return;
	}

	const auto& actors = OwnerWorld->GetObjectManager()->GetAllActors();
	for (const auto& actorPtr : actors) {
		AActor* actor = actorPtr.get();
		if (!actor || !actor->bReplicates) {
			continue;
		}

		if (actor->IsPendingDestroy()) {
			if (actor->NetworkId != 0) {
				const FNetworkActorId networkId = actor->NetworkId;
				UnregisterActor(networkId);
				SendActorDestroy(networkId);
				actor->NetworkId = 0;
			}
			continue;
		}

		if (actor->NetworkId == 0) {
			if (EnsureServerActorRegistered(actor)) {
				SendActorSpawn(actor);
				actor->UpdateReplicatedStateCache();
			}
			continue;
		}

		RegisterActor(actor);
		if (actor->HasReplicatedStateChanged()) {
			SendActorState(actor);
		}
	}
}

AActor* MReplicationSystem::FindActor(FNetworkActorId NetworkId) const
{
	auto it = ActorsByNetworkId.find(NetworkId);
	if (it == ActorsByNetworkId.end()) {
		return nullptr;
	}

	return it->second;
}

void MReplicationSystem::RegisterActor(AActor* Actor)
{
	if (!Actor || Actor->NetworkId == 0) {
		return;
	}

	ActorsByNetworkId[Actor->NetworkId] = Actor;
}

void MReplicationSystem::UnregisterActor(FNetworkActorId NetworkId)
{
	if (NetworkId == 0) {
		return;
	}

	ActorsByNetworkId.erase(NetworkId);
}

void MReplicationSystem::Clear()
{
	ActorsByNetworkId.clear();
}

void MReplicationSystem::HandleConnected(FNetworkConnectionId ConnectionId)
{
	if (!OwnerWorld || !OwnerWorld->IsServer()) {
		return;
	}

	if (AGameModeBase* gameMode = OwnerWorld->GetGameMode()) {
		gameMode->OnClientConnected(ConnectionId);
	}

	SendInitialStateToClient(ConnectionId);
}

void MReplicationSystem::HandleDisconnected(FNetworkConnectionId ConnectionId)
{
	(void)ConnectionId;
}

void MReplicationSystem::HandlePacketReceived(FNetworkConnectionId ConnectionId, FNetBuffer& Buffer)
{
	(void)ConnectionId;

	if (!OwnerWorld || !OwnerWorld->IsClient()) {
		return;
	}

	ENetPacketType packetType = ENetPacketType::None;
	if (!Buffer.Read(packetType)) {
		return;
	}

	switch (packetType) {
	case ENetPacketType::ActorSpawn:
		HandleActorSpawn(Buffer);
		break;
	case ENetPacketType::ActorState:
		HandleActorState(Buffer);
		break;
	case ENetPacketType::ActorDestroy:
		HandleActorDestroy(Buffer);
		break;
	default:
		break;
	}
}

void MReplicationSystem::HandleActorSpawn(FNetBuffer& Buffer)
{
	FNetworkActorId networkId = 0;
	FNetworkConnectionId ownerConnectionId = 0;
	std::string className;

	if (!Buffer.Read(networkId)) return;
	if (!Buffer.Read(ownerConnectionId)) return;
	if (!Buffer.ReadString(className)) return;

	AActor* actor = FindActor(networkId);
	const bool bAlreadyExists = (actor != nullptr);
	if (!actor) {
		actor = ActorRegistry::GetInstance().Spawn(OwnerWorld, className);
		if (!actor) {
			return;
		}
	}

	actor->NetworkId = networkId;
	actor->OwnerConnectionId = ownerConnectionId;
	actor->bReplicates = true;
	actor->bHasAuthority = false;
	actor->bIsLocallyControlled = ownerConnectionId != 0 && ownerConnectionId == NetworkManager::GetInstance().GetLocalConnectionId();

	if (!actor->DeserializeNetworkState(Buffer)) {
		if (!bAlreadyExists) {
			actor->Destroy();
		}
		return;
	}

	RegisterActor(actor);
	if (actor->bIsLocallyControlled) {
		if (AGameModeBase* gameMode = OwnerWorld->GetGameMode()) {
			if (APawn* pawn = dynamic_cast<APawn*>(actor)) {
				gameMode->PossessLocalPawn(pawn);
			}
		}
	}
	if (!bAlreadyExists) {
		actor->Spawned();
	}
}

void MReplicationSystem::HandleActorState(FNetBuffer& Buffer)
{
	FNetworkActorId networkId = 0;
	if (!Buffer.Read(networkId)) {
		return;
	}

	uint32_t sequence = 0;
	if (!Buffer.Read(sequence)) {
		return;
	}

	AActor* actor = FindActor(networkId);
	if (!actor) {
		return;
	}

	if (!IsReplicationSequenceNewer(sequence, actor->GetLastReceivedReplicationSequence())) {
		return;
	}

	if (actor->DeserializeNetworkState(Buffer)) {
		actor->SetLastReceivedReplicationSequence(sequence);
	}
}

void MReplicationSystem::HandleActorDestroy(FNetBuffer& Buffer)
{
	FNetworkActorId networkId = 0;
	if (!Buffer.Read(networkId)) {
		return;
	}

	AActor* actor = FindActor(networkId);
	if (actor) {
		actor->Destroy();
	}
	UnregisterActor(networkId);
}

void MReplicationSystem::SendInitialStateToClient(FNetworkConnectionId ConnectionId)
{
	if (!OwnerWorld || !OwnerWorld->GetObjectManager()) {
		return;
	}

	const auto& actors = OwnerWorld->GetObjectManager()->GetAllActors();
	for (const auto& actorPtr : actors) {
		AActor* actor = actorPtr.get();
		if (!actor || !actor->bReplicates || actor->IsPendingDestroy()) {
			continue;
		}

		if (EnsureServerActorRegistered(actor)) {
			SendActorSpawn(actor, ConnectionId);
		}
	}
}

void MReplicationSystem::SendActorSpawn(AActor* Actor, FNetworkConnectionId TargetConnectionId)
{
	if (!Actor || Actor->NetworkId == 0) {
		return;
	}

	FNetBuffer buffer;
	buffer.Write(ENetPacketType::ActorSpawn);
	buffer.Write(Actor->NetworkId);
	buffer.Write(Actor->OwnerConnectionId);
	Actor->SerializeNetworkSpawn(buffer);

	NetworkManager& network = NetworkManager::GetInstance();
	if (TargetConnectionId != 0) {
		network.SendToClient(TargetConnectionId, buffer, ENetPacketReliability::Reliable);
	}
	else {
		network.Broadcast(buffer, ENetPacketReliability::Reliable);
	}
}

void MReplicationSystem::SendActorState(AActor* Actor)
{
	if (!Actor || Actor->NetworkId == 0) {
		return;
	}

	const uint32_t sequence = Actor->IncrementReplicationSequence();

	FNetBuffer buffer;
	buffer.Write(ENetPacketType::ActorState);
	buffer.Write(Actor->NetworkId);
	buffer.Write(sequence);
	Actor->SerializeNetworkState(buffer);

	NetworkManager::GetInstance().Broadcast(buffer, ENetPacketReliability::Unreliable);
	Actor->UpdateReplicatedStateCache();
}

void MReplicationSystem::SendActorDestroy(FNetworkActorId NetworkId)
{
	if (NetworkId == 0) {
		return;
	}

	FNetBuffer buffer;
	buffer.Write(ENetPacketType::ActorDestroy);
	buffer.Write(NetworkId);

	NetworkManager::GetInstance().Broadcast(buffer, ENetPacketReliability::Reliable);
}

bool MReplicationSystem::EnsureServerActorRegistered(AActor* Actor)
{
	if (!OwnerWorld || !Actor || !OwnerWorld->IsServer()) {
		return false;
	}

	if (Actor->NetworkId == 0) {
		Actor->NetworkId = OwnerWorld->AllocateNetworkActorId();
		Actor->bHasAuthority = true;
	}

	if (Actor->NetworkId == 0) {
		return false;
	}

	RegisterActor(Actor);
	return true;
}
