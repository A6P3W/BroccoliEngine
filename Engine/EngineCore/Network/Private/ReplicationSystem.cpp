#include "ReplicationSystem.h"

#include <cstdint>
#include <string>
#include <unordered_map>

#include "Actor.h"
#include "ActorRegistry.h"
#include "GameModeBase.h"
#include "NetBuffer.h"
#include "NetPacketType.h"
#include "NetworkManager.h"
#include "ActorManager.h"
#include "Pawn.h"
#include "PlayerController.h"
#include "SceneManager.h"
#include "World.h"

namespace {
bool IsReplicationSequenceNewer(uint32_t IncomingSequence, uint32_t CurrentSequence) {
  return static_cast<int32_t>(IncomingSequence - CurrentSequence) > 0;
}

std::string MakeSceneIdTravelKey(FNetworkSceneId SceneId) {
  return "id:" + std::to_string(SceneId);
}

std::string MakeLevelPathTravelKey(const std::string& LevelPath) {
  return "path:" + LevelPath;
}

void WriteServerTravelPayload(FNetBuffer& Buffer, FNetworkSceneId SceneId) {
  const bool bIsPathBased = false;
  Buffer.Write(bIsPathBased);
  Buffer.Write(SceneId);
}

void WriteServerTravelPayload(FNetBuffer& Buffer, const std::string& LevelPath) {
  const bool bIsPathBased = true;
  Buffer.Write(bIsPathBased);
  Buffer.WriteString(LevelPath);
}
}  // namespace


struct FReplicationSystem::Impl {
  World* OwnerWorld = nullptr;
  NetworkManager::CallbackHandle ConnectedCallbackHandle = 0;
  NetworkManager::CallbackHandle DisconnectedCallbackHandle = 0;
  NetworkManager::CallbackHandle PacketReceivedCallbackHandle = 0;
  std::unordered_map<FNetworkActorId, AActor*> ActorsByNetworkId;
  std::unordered_map<FNetworkConnectionId, std::string> LastReadyTravelByConnection;
};
FReplicationSystem::FReplicationSystem(World* InWorld) : ImplPtr(new Impl()) {
  ImplPtr->OwnerWorld = InWorld;
  NetworkManager& network = NetworkManager::GetInstance();
  ImplPtr->ConnectedCallbackHandle = network.AddOnConnected([this](FNetworkConnectionId ConnectionId) {
    HandleConnected(ConnectionId);
  });
  ImplPtr->DisconnectedCallbackHandle = network.AddOnDisconnected([this](FNetworkConnectionId ConnectionId) {
    HandleDisconnected(ConnectionId);
  });
  ImplPtr->PacketReceivedCallbackHandle =
      network.AddOnPacketReceived([this](FNetworkConnectionId ConnectionId, FNetBuffer& Buffer) {
        HandlePacketReceived(ConnectionId, Buffer);
      });
}

FReplicationSystem::~FReplicationSystem() {
  NetworkManager& network = NetworkManager::GetInstance();
  network.RemoveOnConnected(ImplPtr->ConnectedCallbackHandle);
  network.RemoveOnDisconnected(ImplPtr->DisconnectedCallbackHandle);
  network.RemoveOnPacketReceived(ImplPtr->PacketReceivedCallbackHandle);
  delete ImplPtr;
}

void FReplicationSystem::Update() {
  if (!ImplPtr->OwnerWorld || !ImplPtr->OwnerWorld->IsServer() || !ImplPtr->OwnerWorld->GetObjectManager()) {
    return;
  }
  const auto& actors = ImplPtr->OwnerWorld->GetObjectManager()->GetAllActors();
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

AActor* FReplicationSystem::FindActor(FNetworkActorId NetworkId) const {
  auto it = ImplPtr->ActorsByNetworkId.find(NetworkId);
  if (it == ImplPtr->ActorsByNetworkId.end()) {
    return nullptr;
  }
  return it->second;
}

void FReplicationSystem::RegisterActor(AActor* Actor) {
  if (!Actor || Actor->NetworkId == 0) {
    return;
  }
  ImplPtr->ActorsByNetworkId[Actor->NetworkId] = Actor;
}

void FReplicationSystem::UnregisterActor(FNetworkActorId NetworkId) {
  if (NetworkId == 0) {
    return;
  }
  ImplPtr->ActorsByNetworkId.erase(NetworkId);
}

void FReplicationSystem::Clear() { ImplPtr->ActorsByNetworkId.clear(); }

bool FReplicationSystem::SendActorRPC(
    AActor* Actor,
    FNetworkRPCId RPCId,
    ENetRPCType RPCType,
    ENetPacketReliability Reliability,
    const FNetBuffer& Payload,
    FNetworkComponentId ComponentNetworkId
) {
  if (!ImplPtr->OwnerWorld || !Actor || Actor->NetworkId == 0 || RPCId == 0) {
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
      if (ImplPtr->OwnerWorld->IsServer() && Actor->bHasAuthority) {
        FNetBuffer localPayload(Payload.GetData());
        return Actor->DispatchRPC(ComponentNetworkId, RPCId, RPCType, localPayload);
      }
      if (!ImplPtr->OwnerWorld->IsClient() || !Actor->bIsLocallyControlled) {
        return false;
      }
      return network.SendToServer(buffer, Reliability);
    case ENetRPCType::Client:
      if (!ImplPtr->OwnerWorld->IsServer() || !Actor->bHasAuthority) {
        return false;
      }
      if (Actor->OwnerConnectionId == 0) {
        FNetBuffer localPayload(Payload.GetData());
        return Actor->DispatchRPC(ComponentNetworkId, RPCId, RPCType, localPayload);
      }
      return network.SendToClient(Actor->OwnerConnectionId, buffer, Reliability);
    case ENetRPCType::Multicast:
      if (!ImplPtr->OwnerWorld->IsServer() || !Actor->bHasAuthority) {
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

bool FReplicationSystem::BroadcastServerTravel(FNetworkSceneId SceneId) {
  if (!ImplPtr->OwnerWorld || !ImplPtr->OwnerWorld->IsListenServer() || SceneId == 0) {
    return false;
  }
  FNetBuffer buffer;
  buffer.Write(ENetPacketType::ServerTravel);
  WriteServerTravelPayload(buffer, SceneId);
  return NetworkManager::GetInstance().Broadcast(buffer, ENetPacketReliability::Reliable);
}

bool FReplicationSystem::BroadcastServerTravel(const std::string& LevelPath) {
  if (!ImplPtr->OwnerWorld || !ImplPtr->OwnerWorld->IsListenServer() || LevelPath.empty()) {
    return false;
  }
  FNetBuffer buffer;
  buffer.Write(ENetPacketType::ServerTravel);
  WriteServerTravelPayload(buffer, LevelPath);
  return NetworkManager::GetInstance().Broadcast(buffer, ENetPacketReliability::Reliable);
}

void FReplicationSystem::NotifySceneLoaded() {
  if (!ImplPtr->OwnerWorld || !ImplPtr->OwnerWorld->IsClient()) {
    return;
  }

  SceneManager& sceneManager = SceneManager::GetInstance();
  const FNetworkSceneId currentSceneId = sceneManager.GetCurrentSceneId();
  if (currentSceneId != 0) {
    SendClientTravelReady(currentSceneId);
    return;
  }

  SendClientTravelReady(sceneManager.GetCurrentLevelPath());
}

void FReplicationSystem::HandleConnected(FNetworkConnectionId ConnectionId) {
  if (!ImplPtr->OwnerWorld) {
    return;
  }
  if (NetworkManager::GetInstance().IsClient()) {
    ImplPtr->OwnerWorld->SetNetMode(ENetMode::Client);
    return;
  }
  if (!ImplPtr->OwnerWorld->IsServer()) {
    return;
  }
  const FNetworkSceneId currentSceneId = SceneManager::GetInstance().GetCurrentSceneId();
  if (currentSceneId != 0 && SendServerTravelToClient(ConnectionId, currentSceneId)) {
    return;
  }

  const std::string& currentLevelPath = SceneManager::GetInstance().GetCurrentLevelPath();
  if (!currentLevelPath.empty() && SendServerTravelToClient(ConnectionId, currentLevelPath)) {
    return;
  }

  InitializeClientForCurrentScene(ConnectionId);
}

void FReplicationSystem::HandleDisconnected(FNetworkConnectionId ConnectionId) {
  ImplPtr->LastReadyTravelByConnection.erase(ConnectionId);
  if (ImplPtr->OwnerWorld && ImplPtr->OwnerWorld->IsServer()) {
    if (AGameModeBase* gameMode = ImplPtr->OwnerWorld->GetGameMode()) {
      gameMode->OnClientDisconnected(ConnectionId);
    }
  }
}

void FReplicationSystem::HandlePacketReceived(
    FNetworkConnectionId ConnectionId, FNetBuffer& Buffer
) {
  if (!ImplPtr->OwnerWorld) {
    return;
  }
  ENetPacketType packetType = ENetPacketType::None;
  if (!Buffer.Read(packetType)) {
    return;
  }
  switch (packetType) {
    case ENetPacketType::AssignNetId:
      if (!ImplPtr->OwnerWorld->IsClient()) {
        break;
      }
      HandleAssignNetId(Buffer);
      break;
    case ENetPacketType::ActorSpawn:
      if (!ImplPtr->OwnerWorld->IsClient()) {
        break;
      }
      HandleActorSpawn(Buffer);
      break;
    case ENetPacketType::ActorState:
      if (!ImplPtr->OwnerWorld->IsClient()) {
        break;
      }
      HandleActorState(Buffer);
      break;
    case ENetPacketType::ActorDestroy:
      if (!ImplPtr->OwnerWorld->IsClient()) {
        break;
      }
      HandleActorDestroy(Buffer);
      break;
    case ENetPacketType::ActorRPC:
      HandleActorRPC(ConnectionId, Buffer);
      break;
    case ENetPacketType::ServerTravel:
      if (!ImplPtr->OwnerWorld->IsClient()) {
        break;
      }
      HandleServerTravel(Buffer);
      break;
    case ENetPacketType::ClientTravelReady:
      if (!ImplPtr->OwnerWorld->IsServer()) {
        break;
      }
      HandleClientTravelReady(ConnectionId, Buffer);
      break;
    default:
      break;
  }
}

void FReplicationSystem::HandleActorSpawn(FNetBuffer& Buffer) {
  FNetworkActorId networkId = 0;
  FNetworkConnectionId ownerConnectionId = 0;
  std::string className;
  if (!Buffer.Read(networkId)) return;
  if (!Buffer.Read(ownerConnectionId)) return;
  if (!Buffer.ReadString(className)) return;

  AActor* actor = FindActor(networkId);
  const bool bAlreadyExists = (actor != nullptr);
  if (!actor) {
    actor = ActorRegistry::GetInstance().Spawn(ImplPtr->OwnerWorld, className);
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
    if (APlayerController* pc = dynamic_cast<APlayerController*>(actor)) {
      // サーバーからレプリケートされてきたのが PlayerController
      // の場合、自分のコントローラーとして登録する
      ImplPtr->OwnerWorld->SetLocalPlayerController(pc);
    } else if (APawn* pawn = dynamic_cast<APawn*>(actor)) {
      // Pawnの場合はPossessする
      ImplPtr->OwnerWorld->PossessLocalPawn(pawn);
    }
  }

  if (!bAlreadyExists) {
    actor->Spawned();
  }
}

void FReplicationSystem::HandleActorState(FNetBuffer& Buffer) {
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

void FReplicationSystem::HandleActorDestroy(FNetBuffer& Buffer) {
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

void FReplicationSystem::HandleActorRPC(FNetworkConnectionId ConnectionId, FNetBuffer& Buffer) {
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
  if (ImplPtr->OwnerWorld->IsServer()) {
    if (rpcType != ENetRPCType::Server || !actor->bHasAuthority || actor->OwnerConnectionId == 0 ||
        actor->OwnerConnectionId != ConnectionId) {
      return;
    }
  } else if (!ImplPtr->OwnerWorld->IsClient() || rpcType == ENetRPCType::Server) {
    return;
  }
  actor->DispatchRPC(componentNetworkId, rpcId, rpcType, payload);
}

void FReplicationSystem::HandleAssignNetId(FNetBuffer& Buffer) {
  FNetworkConnectionId assignedConnectionId = 0;
  if (!Buffer.Read(assignedConnectionId)) {
    return;
  }
  NetworkManager::GetInstance().SetLocalConnectionId(assignedConnectionId);
  RefreshLocalControl();
}

void FReplicationSystem::HandleServerTravel(FNetBuffer& Buffer) {
  bool bIsPathBased = false;
  if (!Buffer.Read(bIsPathBased)) {
    return;
  }

  if (bIsPathBased) {
    std::string levelPath;
    if (!Buffer.ReadString(levelPath) || levelPath.empty()) {
      return;
    }

    SceneManager::GetInstance().OpenLevelByPath(levelPath, ENetMode::Client);
    return;
  }

  FNetworkSceneId sceneId = 0;
  if (!Buffer.Read(sceneId)) {
    return;
  }
  SceneManager::GetInstance().OpenLevelById(sceneId, ENetMode::Client);
}

void FReplicationSystem::HandleClientTravelReady(
    FNetworkConnectionId ConnectionId, FNetBuffer& Buffer
) {
  bool bIsPathBased = false;
  if (!Buffer.Read(bIsPathBased)) {
    return;
  }

  std::string travelKey;
  if (bIsPathBased) {
    std::string levelPath;
    if (!Buffer.ReadString(levelPath) || levelPath.empty()) {
      return;
    }
    if (levelPath != SceneManager::GetInstance().GetCurrentLevelPath()) {
      return;
    }
    travelKey = MakeLevelPathTravelKey(levelPath);
  } else {
    FNetworkSceneId sceneId = 0;
    if (!Buffer.Read(sceneId)) {
      return;
    }
    if (sceneId == 0 || sceneId != SceneManager::GetInstance().GetCurrentSceneId()) {
      return;
    }
    travelKey = MakeSceneIdTravelKey(sceneId);
  }

  auto readyIt = ImplPtr->LastReadyTravelByConnection.find(ConnectionId);
  if (readyIt != ImplPtr->LastReadyTravelByConnection.end() && readyIt->second == travelKey) {
    return;
  }
  ImplPtr->LastReadyTravelByConnection[ConnectionId] = travelKey;
  InitializeClientForCurrentScene(ConnectionId);
}

void FReplicationSystem::SendAssignedConnectionId(FNetworkConnectionId ConnectionId) {
  if (!ImplPtr->OwnerWorld || !ImplPtr->OwnerWorld->IsServer() || ConnectionId == 0) {
    return;
  }
  FNetBuffer buffer;
  buffer.Write(ENetPacketType::AssignNetId);
  buffer.Write(ConnectionId);
  NetworkManager::GetInstance().SendToClient(ConnectionId, buffer, ENetPacketReliability::Reliable);
}

bool FReplicationSystem::SendServerTravelToClient(
    FNetworkConnectionId ConnectionId, FNetworkSceneId SceneId
) {
  if (!ImplPtr->OwnerWorld || !ImplPtr->OwnerWorld->IsServer() || ConnectionId == 0 || SceneId == 0) {
    return false;
  }
  FNetBuffer buffer;
  buffer.Write(ENetPacketType::ServerTravel);
  WriteServerTravelPayload(buffer, SceneId);
  return NetworkManager::GetInstance().SendToClient(
      ConnectionId, buffer, ENetPacketReliability::Reliable
  );
}

bool FReplicationSystem::SendServerTravelToClient(
    FNetworkConnectionId ConnectionId, const std::string& LevelPath
) {
  if (!ImplPtr->OwnerWorld || !ImplPtr->OwnerWorld->IsServer() || ConnectionId == 0 || LevelPath.empty()) {
    return false;
  }
  FNetBuffer buffer;
  buffer.Write(ENetPacketType::ServerTravel);
  WriteServerTravelPayload(buffer, LevelPath);
  return NetworkManager::GetInstance().SendToClient(
      ConnectionId, buffer, ENetPacketReliability::Reliable
  );
}

bool FReplicationSystem::SendClientTravelReady(FNetworkSceneId SceneId) {
  if (!ImplPtr->OwnerWorld || !ImplPtr->OwnerWorld->IsClient() || SceneId == 0) {
    return false;
  }
  FNetBuffer buffer;
  buffer.Write(ENetPacketType::ClientTravelReady);
  WriteServerTravelPayload(buffer, SceneId);
  return NetworkManager::GetInstance().SendToServer(buffer, ENetPacketReliability::Reliable);
}

bool FReplicationSystem::SendClientTravelReady(const std::string& LevelPath) {
  if (!ImplPtr->OwnerWorld || !ImplPtr->OwnerWorld->IsClient() || LevelPath.empty()) {
    return false;
  }
  FNetBuffer buffer;
  buffer.Write(ENetPacketType::ClientTravelReady);
  WriteServerTravelPayload(buffer, LevelPath);
  return NetworkManager::GetInstance().SendToServer(buffer, ENetPacketReliability::Reliable);
}

void FReplicationSystem::SendInitialStateToClient(FNetworkConnectionId ConnectionId) {
  if (!ImplPtr->OwnerWorld || !ImplPtr->OwnerWorld->GetObjectManager()) {
    return;
  }
  const auto& actors = ImplPtr->OwnerWorld->GetObjectManager()->GetAllActors();
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

bool FReplicationSystem::SendActorSpawn(AActor* Actor, FNetworkConnectionId TargetConnectionId) {
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

bool FReplicationSystem::SendActorState(AActor* Actor) {
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

void FReplicationSystem::SendActorDestroy(FNetworkActorId NetworkId) {
  if (NetworkId == 0) {
    return;
  }
  FNetBuffer buffer;
  buffer.Write(ENetPacketType::ActorDestroy);
  buffer.Write(NetworkId);
  NetworkManager::GetInstance().Broadcast(buffer, ENetPacketReliability::Reliable);
}

bool FReplicationSystem::EnsureServerActorRegistered(AActor* Actor) {
  if (!ImplPtr->OwnerWorld || !Actor || !ImplPtr->OwnerWorld->IsServer()) {
    return false;
  }
  if (Actor->NetworkId == 0) {
    Actor->NetworkId = ImplPtr->OwnerWorld->AllocateNetworkActorId();
    Actor->bHasAuthority = true;
  }
  if (Actor->NetworkId == 0) {
    return false;
  }
  Actor->AssignNetworkComponentIds();
  RegisterActor(Actor);
  return true;
}

void FReplicationSystem::RefreshLocalControl() {
  if (!ImplPtr->OwnerWorld || !ImplPtr->OwnerWorld->IsClient()) {
    return;
  }
  const FNetworkConnectionId localConnectionId =
      NetworkManager::GetInstance().GetLocalConnectionId();
  if (localConnectionId == 0) {
    return;
  }
  for (const auto& pair : ImplPtr->ActorsByNetworkId) {
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
      if (APlayerController* pc = dynamic_cast<APlayerController*>(actor)) {
        ImplPtr->OwnerWorld->SetLocalPlayerController(pc);
      } else if (APawn* pawn = dynamic_cast<APawn*>(actor)) {
        ImplPtr->OwnerWorld->PossessLocalPawn(pawn);
      }
    }
  }
}

void FReplicationSystem::InitializeClientForCurrentScene(FNetworkConnectionId ConnectionId) {
  SendAssignedConnectionId(ConnectionId);
  if (AGameModeBase* gameMode = ImplPtr->OwnerWorld->GetGameMode()) {
    gameMode->OnClientConnected(ConnectionId);
  }
  SendInitialStateToClient(ConnectionId);
}
