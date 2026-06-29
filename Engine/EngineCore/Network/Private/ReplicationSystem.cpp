#include "ReplicationSystem.h"

#include <cstdint>
#include <string>

#include "Actor.h"
#include "ActorRegistry.h"
#include "GameModeBase.h"
#include "NetBuffer.h"
#include "NetPacketType.h"
#include "NetworkManager.h"
#include "ObjectManager.h"
#include "Pawn.h"
#include "SceneManager.h"
#include "World.h"

namespace {
bool IsReplicationSequenceNewer(uint32_t IncomingSequence, uint32_t CurrentSequence) {
  return static_cast<int32_t>(IncomingSequence - CurrentSequence) > 0;
}
}  // namespace

MReplicationSystem::MReplicationSystem(World* InWorld) : OwnerWorld(InWorld) {
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

MReplicationSystem::~MReplicationSystem() {
  NetworkManager& network = NetworkManager::GetInstance();
  network.SetOnConnected({});
  network.SetOnDisconnected({});
  network.SetOnPacketReceived({});
}

void MReplicationSystem::Update() {
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
        if (SendActorSpawn(actor)) {
          actor->UpdateReplicatedStateCache();
        }
      }
      continue;
    }

    RegisterActor(actor);
    if (actor->HasReplicatedStateChanged()) {
      SendActorState(actor);
    }
  }
}

AActor* MReplicationSystem::FindActor(FNetworkActorId NetworkId) const {
  auto it = ActorsByNetworkId.find(NetworkId);
  if (it == ActorsByNetworkId.end()) {
    return nullptr;
  }

  return it->second;
}

void MReplicationSystem::RegisterActor(AActor* Actor) {
  if (!Actor || Actor->NetworkId == 0) {
    return;
  }

  ActorsByNetworkId[Actor->NetworkId] = Actor;
}

void MReplicationSystem::UnregisterActor(FNetworkActorId NetworkId) {
  if (NetworkId == 0) {
    return;
  }

  ActorsByNetworkId.erase(NetworkId);
}

void MReplicationSystem::Clear() { ActorsByNetworkId.clear(); }

bool MReplicationSystem::SendActorRPC(
    AActor* Actor,
    FNetworkRPCId RPCId,
    ENetRPCType RPCType,
    ENetPacketReliability Reliability,
    const FNetBuffer& Payload,
    FNetworkComponentId ComponentNetworkId
) {
  if (!OwnerWorld || !Actor || Actor->NetworkId == 0 || RPCId == 0) {
    return false;
  }

  FNetBuffer buffer;
  buffer.Write(ENetPacketType::ActorRPC);
  buffer.Write(Actor->NetworkId);
  buffer.Write(ComponentNetworkId);
  buffer.Write(RPCId);
  buffer.Write(RPCType);
  if (!buffer.WriteBufferWithSize(Payload)) {
    return false;
  }

  NetworkManager& network = NetworkManager::GetInstance();
  switch (RPCType) {
    case ENetRPCType::Server:
      if (OwnerWorld->IsServer() && Actor->bHasAuthority) {
        FNetBuffer localPayload(Payload.GetData());
        return Actor->DispatchRPC(ComponentNetworkId, RPCId, RPCType, localPayload);
      }
      if (!OwnerWorld->IsClient() || !Actor->bIsLocallyControlled) {
        return false;
      }
      return network.SendToServer(buffer, Reliability);
    case ENetRPCType::Client:
      if (!OwnerWorld->IsServer() || !Actor->bHasAuthority) {
        return false;
      }
      if (Actor->OwnerConnectionId == 0) {
        FNetBuffer localPayload(Payload.GetData());
        return Actor->DispatchRPC(ComponentNetworkId, RPCId, RPCType, localPayload);
      }
      return network.SendToClient(Actor->OwnerConnectionId, buffer, Reliability);
    case ENetRPCType::Multicast:
      if (!OwnerWorld->IsServer() || !Actor->bHasAuthority) {
        return false;
      }
      {
        FNetBuffer localPayload(Payload.GetData());
        Actor->DispatchRPC(ComponentNetworkId, RPCId, RPCType, localPayload);
      }
      return network.Broadcast(buffer, Reliability);
    default:
      return false;
  }
}

bool MReplicationSystem::BroadcastServerTravel(FNetworkSceneId SceneId) {
  if (!OwnerWorld || !OwnerWorld->IsListenServer() || SceneId == 0) {
    return false;
  }

  FNetBuffer buffer;
  buffer.Write(ENetPacketType::ServerTravel);
  buffer.Write(SceneId);

  return NetworkManager::GetInstance().Broadcast(buffer, ENetPacketReliability::Reliable);
}

void MReplicationSystem::NotifySceneLoaded() {
  if (!OwnerWorld || !OwnerWorld->IsClient()) {
    return;
  }

  SendClientTravelReady(SceneManager::GetInstance().GetCurrentSceneId());
}

void MReplicationSystem::HandleConnected(FNetworkConnectionId ConnectionId) {
  if (!OwnerWorld || !OwnerWorld->IsServer()) {
    return;
  }

  const FNetworkSceneId currentSceneId = SceneManager::GetInstance().GetCurrentSceneId();
  if (currentSceneId != 0 && SendServerTravelToClient(ConnectionId, currentSceneId)) {
    return;
  }

  InitializeClientForCurrentScene(ConnectionId);
}

void MReplicationSystem::HandleDisconnected(FNetworkConnectionId ConnectionId) {
  LastReadySceneByConnection.erase(ConnectionId);
  if (OwnerWorld && OwnerWorld->IsServer()) {
    if (AGameModeBase* gameMode = OwnerWorld->GetGameMode()) {
      gameMode->OnClientDisconnected(ConnectionId);
    }
  }
}
void MReplicationSystem::HandlePacketReceived(
    FNetworkConnectionId ConnectionId, FNetBuffer& Buffer
) {
  if (!OwnerWorld) {
    return;
  }

  ENetPacketType packetType = ENetPacketType::None;
  if (!Buffer.Read(packetType)) {
    return;
  }

  switch (packetType) {
    case ENetPacketType::AssignNetId:
      if (!OwnerWorld->IsClient()) {
        break;
      }
      HandleAssignNetId(Buffer);
      break;
    case ENetPacketType::ActorSpawn:
      if (!OwnerWorld->IsClient()) {
        break;
      }
      HandleActorSpawn(Buffer);
      break;
    case ENetPacketType::ActorState:
      if (!OwnerWorld->IsClient()) {
        break;
      }
      HandleActorState(Buffer);
      break;
    case ENetPacketType::ActorDestroy:
      if (!OwnerWorld->IsClient()) {
        break;
      }
      HandleActorDestroy(Buffer);
      break;
    case ENetPacketType::ActorRPC:
      HandleActorRPC(ConnectionId, Buffer);
      break;
    case ENetPacketType::ServerTravel:
      if (!OwnerWorld->IsClient()) {
        break;
      }
      HandleServerTravel(Buffer);
      break;
    case ENetPacketType::ClientTravelReady:
      if (!OwnerWorld->IsServer()) {
        break;
      }
      HandleClientTravelReady(ConnectionId, Buffer);
      break;
    default:
      break;
  }
}

void MReplicationSystem::HandleActorSpawn(FNetBuffer& Buffer) {
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
  actor->bIsLocallyControlled =
      ownerConnectionId != 0 &&
      ownerConnectionId == NetworkManager::GetInstance().GetLocalConnectionId();

  if (!actor->DeserializeNetworkSpawn(Buffer)) {
    if (!bAlreadyExists) {
      actor->Destroy();
    }
    return;
  }

  RegisterActor(actor);
  if (actor->bIsLocallyControlled) {
    if (APawn* pawn = dynamic_cast<APawn*>(actor)) {
      OwnerWorld->PossessLocalPawn(pawn);
    }
  }
  if (!bAlreadyExists) {
    actor->Spawned();
  }
}

void MReplicationSystem::HandleActorState(FNetBuffer& Buffer) {
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

void MReplicationSystem::HandleActorDestroy(FNetBuffer& Buffer) {
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

void MReplicationSystem::HandleActorRPC(FNetworkConnectionId ConnectionId, FNetBuffer& Buffer) {
  FNetworkActorId networkId = 0;
  FNetworkComponentId componentNetworkId = 0;
  FNetworkRPCId rpcId = 0;
  ENetRPCType rpcType = ENetRPCType::Server;

  if (!Buffer.Read(networkId)) return;
  if (!Buffer.Read(componentNetworkId)) return;
  if (!Buffer.Read(rpcId)) return;
  if (!Buffer.Read(rpcType)) return;

  FNetBuffer payload;
  if (!Buffer.ReadBufferWithSize(payload)) return;

  AActor* actor = FindActor(networkId);
  if (!actor) {
    return;
  }

  if (OwnerWorld->IsServer()) {
    if (rpcType != ENetRPCType::Server || !actor->bHasAuthority || actor->OwnerConnectionId == 0 ||
        actor->OwnerConnectionId != ConnectionId) {
      return;
    }
  } else if (!OwnerWorld->IsClient() || rpcType == ENetRPCType::Server) {
    return;
  }

  actor->DispatchRPC(componentNetworkId, rpcId, rpcType, payload);
}

void MReplicationSystem::HandleAssignNetId(FNetBuffer& Buffer) {
  FNetworkConnectionId assignedConnectionId = 0;
  if (!Buffer.Read(assignedConnectionId)) {
    return;
  }

  NetworkManager::GetInstance().SetLocalConnectionId(assignedConnectionId);
  RefreshLocalControl();
}

void MReplicationSystem::HandleServerTravel(FNetBuffer& Buffer) {
  FNetworkSceneId sceneId = 0;
  if (!Buffer.Read(sceneId)) {
    return;
  }

  SceneManager::GetInstance().OpenSceneById(sceneId, ENetMode::Client);
}

void MReplicationSystem::HandleClientTravelReady(
    FNetworkConnectionId ConnectionId, FNetBuffer& Buffer
) {
  FNetworkSceneId sceneId = 0;
  if (!Buffer.Read(sceneId)) {
    return;
  }

  if (sceneId == 0 || sceneId != SceneManager::GetInstance().GetCurrentSceneId()) {
    return;
  }

  auto readyIt = LastReadySceneByConnection.find(ConnectionId);
  if (readyIt != LastReadySceneByConnection.end() && readyIt->second == sceneId) {
    return;
  }
  LastReadySceneByConnection[ConnectionId] = sceneId;

  InitializeClientForCurrentScene(ConnectionId);
}

void MReplicationSystem::SendAssignedConnectionId(FNetworkConnectionId ConnectionId) {
  if (!OwnerWorld || !OwnerWorld->IsServer() || ConnectionId == 0) {
    return;
  }

  FNetBuffer buffer;
  buffer.Write(ENetPacketType::AssignNetId);
  buffer.Write(ConnectionId);

  NetworkManager::GetInstance().SendToClient(ConnectionId, buffer, ENetPacketReliability::Reliable);
}

bool MReplicationSystem::SendServerTravelToClient(
    FNetworkConnectionId ConnectionId, FNetworkSceneId SceneId
) {
  if (!OwnerWorld || !OwnerWorld->IsServer() || ConnectionId == 0 || SceneId == 0) {
    return false;
  }

  FNetBuffer buffer;
  buffer.Write(ENetPacketType::ServerTravel);
  buffer.Write(SceneId);

  return NetworkManager::GetInstance().SendToClient(
      ConnectionId, buffer, ENetPacketReliability::Reliable
  );
}

bool MReplicationSystem::SendClientTravelReady(FNetworkSceneId SceneId) {
  if (!OwnerWorld || !OwnerWorld->IsClient() || SceneId == 0) {
    return false;
  }

  FNetBuffer buffer;
  buffer.Write(ENetPacketType::ClientTravelReady);
  buffer.Write(SceneId);

  return NetworkManager::GetInstance().SendToServer(buffer, ENetPacketReliability::Reliable);
}

void MReplicationSystem::SendInitialStateToClient(FNetworkConnectionId ConnectionId) {
  if (!OwnerWorld || !OwnerWorld->GetObjectManager()) {
    return;
  }

  const auto& actors = OwnerWorld->GetObjectManager()->GetAllActors();
  for (const auto& actorPtr : actors) {
    AActor* actor = actorPtr.get();
    if (!actor || !actor->bReplicates || actor->IsPendingDestroy()) {
      continue;
    }

    if (actor->NetworkId == 0) {
      continue;
    }

    SendActorSpawn(actor, ConnectionId);
  }
}

bool MReplicationSystem::SendActorSpawn(AActor* Actor, FNetworkConnectionId TargetConnectionId) {
  if (!Actor || Actor->NetworkId == 0) {
    return false;
  }

  FNetBuffer buffer;
  buffer.Write(ENetPacketType::ActorSpawn);
  buffer.Write(Actor->NetworkId);
  buffer.Write(Actor->OwnerConnectionId);
  if (!Actor->SerializeNetworkSpawn(buffer)) {
    return false;
  }

  NetworkManager& network = NetworkManager::GetInstance();
  if (TargetConnectionId != 0) {
    network.SendToClient(TargetConnectionId, buffer, ENetPacketReliability::Reliable);
    return true;
  }

  network.Broadcast(buffer, ENetPacketReliability::Reliable);
  return true;
}

bool MReplicationSystem::SendActorState(AActor* Actor) {
  if (!Actor || Actor->NetworkId == 0) {
    return false;
  }

  const uint32_t sequence = Actor->IncrementReplicationSequence();
  FNetBuffer buffer;
  buffer.Write(ENetPacketType::ActorState);
  buffer.Write(Actor->NetworkId);
  buffer.Write(sequence);
  if (!Actor->SerializeNetworkState(buffer)) {
    return false;
  }

  NetworkManager::GetInstance().Broadcast(buffer, ENetPacketReliability::Unreliable);
  Actor->UpdateReplicatedStateCache();
  return true;
}

void MReplicationSystem::SendActorDestroy(FNetworkActorId NetworkId) {
  if (NetworkId == 0) {
    return;
  }

  FNetBuffer buffer;
  buffer.Write(ENetPacketType::ActorDestroy);
  buffer.Write(NetworkId);

  NetworkManager::GetInstance().Broadcast(buffer, ENetPacketReliability::Reliable);
}

bool MReplicationSystem::EnsureServerActorRegistered(AActor* Actor) {
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

  Actor->AssignNetworkComponentIds();
  RegisterActor(Actor);
  return true;
}

void MReplicationSystem::RefreshLocalControl() {
  if (!OwnerWorld || !OwnerWorld->IsClient()) {
    return;
  }

  const FNetworkConnectionId localConnectionId =
      NetworkManager::GetInstance().GetLocalConnectionId();
  if (localConnectionId == 0) {
    return;
  }

  for (const auto& pair : ActorsByNetworkId) {
    AActor* actor = pair.second;
    if (!actor) {
      continue;
    }

    const bool bShouldBeLocallyControlled = actor->OwnerConnectionId == localConnectionId;
    if (actor->bIsLocallyControlled == bShouldBeLocallyControlled) {
      continue;
    }

    actor->bIsLocallyControlled = bShouldBeLocallyControlled;
    if (bShouldBeLocallyControlled) {
      if (APawn* pawn = dynamic_cast<APawn*>(actor)) {
        OwnerWorld->PossessLocalPawn(pawn);
      }
    }
  }
}

void MReplicationSystem::InitializeClientForCurrentScene(FNetworkConnectionId ConnectionId) {
  SendAssignedConnectionId(ConnectionId);
  if (AGameModeBase* gameMode = OwnerWorld->GetGameMode()) {
    gameMode->OnClientConnected(ConnectionId);
  }
  SendInitialStateToClient(ConnectionId);
}
