#include <algorithm>
#include <utility>

#include "Actor.h"
#include "HttpManager.h"
#include "Log.h"
#include "SceneComponent.h"
#include "TimerManager.h"
#include "World.h"

MActorComponent::~MActorComponent() {
  if (GetOwner() && GetOwner()->GetWorld() && GetOwner()->GetWorld()->GetTimerManager()) {
    GetOwner()->GetWorld()->GetTimerManager()->ClearAllTimersForObject(this);
  }
  HttpManager::GetInstance().CancelAllRequestsForObject(this);
}

bool MActorComponent::Update(float DeltaTime) {
  if (IsPendingDestroy()) {
    return false;
  }
  OnUpdate(DeltaTime);
  return true;
}

void MActorComponent::DestroyComponent() {
  if (bPendingDestroy) {
    return;
  }

  if (Owner != nullptr) {
    const auto* rootComponent = Owner->GetRootComponent();
    if (rootComponent != nullptr && static_cast<const MActorComponent*>(rootComponent) == this) {
      M_LOG("DestroyComponent ignored: attempted to destroy RootComponent");
      return;
    }
  }

  bPendingDestroy = true;

  if (GetOwner() && GetOwner()->GetWorld() && GetOwner()->GetWorld()->GetTimerManager()) {
    GetOwner()->GetWorld()->GetTimerManager()->ClearAllTimersForObject(this);
  }

  HttpManager::GetInstance().CancelAllRequestsForObject(this);
  UnRegisterComponent();

  OnComponentDestroy();
}

void MActorComponent::SetNetComponentName(std::string Name) { NetComponentName = std::move(Name); }

void MActorComponent::SerializeNetworkState(FNetBuffer& OutBuffer) {
  for (const auto& replicatedProperty : ReplicatedProperties) {
    if (replicatedProperty) {
      replicatedProperty->Serialize(OutBuffer);
    }
  }
}

bool MActorComponent::DeserializeNetworkState(FNetBuffer& InBuffer) {
  for (const auto& replicatedProperty : ReplicatedProperties) {
    if (replicatedProperty && !replicatedProperty->DeserializeAndTriggerOnRep(InBuffer)) {
      return false;
    }
  }

  return true;
}

void MActorComponent::SerializeNetworkSpawn(FNetBuffer& OutBuffer) {
  SerializeNetworkState(OutBuffer);
}

bool MActorComponent::DeserializeNetworkSpawn(FNetBuffer& InBuffer) {
  return DeserializeNetworkState(InBuffer);
}

bool MActorComponent::HasReplicatedStateChanged() const {
  if (bReplicatedStateDirty) {
    return true;
  }

  return std::any_of(
      ReplicatedProperties.begin(),
      ReplicatedProperties.end(),
      [](const std::unique_ptr<IReplicatedProperty>& replicatedProperty) {
        return replicatedProperty && replicatedProperty->HasChanged();
      }
  );
}

void MActorComponent::UpdateReplicatedStateCache() {
  for (const auto& replicatedProperty : ReplicatedProperties) {
    if (replicatedProperty) {
      replicatedProperty->UpdateCache();
    }
  }
  bReplicatedStateDirty = false;
}

void MActorComponent::MarkReplicatedStateDirty() { bReplicatedStateDirty = true; }

uint32_t MActorComponent::IncrementReplicationSequence() {
  ++ReplicationSequence;
  return ReplicationSequence;
}

void MActorComponent::SetLastReceivedReplicationSequence(uint32_t Sequence) {
  LastReceivedReplicationSequence = Sequence;
}

void MActorComponent::RegisterRPC(FNetworkRPCId RPCId, ENetRPCType RPCType, FRPCHandler Handler) {
  if (RPCId == 0 || !Handler) {
    return;
  }

  RPCHandlers[RPCId] = {RPCType, std::move(Handler)};
}

bool MActorComponent::DispatchRPC(FNetworkRPCId RPCId, ENetRPCType RPCType, FNetBuffer& Payload) {
  auto it = RPCHandlers.find(RPCId);
  if (it == RPCHandlers.end() || it->second.Type != RPCType || !it->second.Handler) {
    return false;
  }

  it->second.Handler(Payload);
  return true;
}

bool MActorComponent::InvokeRPCWithPayload(
    FNetworkRPCId RPCId,
    ENetRPCType RPCType,
    ENetPacketReliability Reliability,
    const FNetBuffer& Payload
) {
  if (RPCId == 0 || !Owner) {
    return false;
  }

  return Owner->InvokeComponentRPCWithPayload(this, RPCId, RPCType, Reliability, Payload);
}
