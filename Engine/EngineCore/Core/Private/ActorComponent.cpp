#include <algorithm>
#include <utility>

#include "Actor.h"
#include "HttpManager.h"
#include "Log.h"
#include "SceneComponent.h"
#include "TimerManager.h"
#include "World.h"

struct MActorComponent::Impl {
  bool PendingDestroy = false;
  bool HasBegunPlay = false;
  EActorComponentRegistrationState RegistrationState = EActorComponentRegistrationState::Created;
  bool ReplicatedStateDirty = false;
  std::string NetComponentName;
  std::vector<std::unique_ptr<IReplicatedProperty>> ReplicatedProperties;
  uint32_t ReplicationSequence = 0;
  uint32_t LastReceivedReplicationSequence = 0;
  std::unordered_map<FNetworkRPCId, FRPCEntry> RPCHandlers;
};

MActorComponent::MActorComponent() : ImplPtr(new Impl()) {}
MActorComponent::~MActorComponent() {
  if (GetOwner() && GetOwner()->GetWorld() && GetOwner()->GetWorld()->GetTimerManager()) {
    GetOwner()->GetWorld()->GetTimerManager()->ClearAllTimersForObject(this);
  }
  HttpManager::GetInstance().CancelAllRequestsForObject(this);
  delete ImplPtr;
  ImplPtr = nullptr;
}


bool MActorComponent::IsPendingDestroy() const { return ImplPtr->PendingDestroy; }

bool MActorComponent::IsRegistered() const {
  return ImplPtr->RegistrationState == EActorComponentRegistrationState::Registered;
}

bool MActorComponent::IsRegistrationPending() const {
  return ImplPtr->RegistrationState == EActorComponentRegistrationState::RegistrationPending;
}

const std::string& MActorComponent::GetNetComponentName() const {
  return ImplPtr->NetComponentName;
}

uint32_t MActorComponent::GetReplicationSequence() const { return ImplPtr->ReplicationSequence; }

uint32_t MActorComponent::GetLastReceivedReplicationSequence() const {
  return ImplPtr->LastReceivedReplicationSequence;
}

void MActorComponent::AddReplicatedProperty(std::unique_ptr<IReplicatedProperty> Property) {
  ImplPtr->ReplicatedProperties.push_back(std::move(Property));
  MarkReplicatedStateDirty();
}
void MActorComponent::BeginPlay() {
  if (ImplPtr->HasBegunPlay || !IsRegistered() || IsPendingDestroy()) {
    return;
  }

  ImplPtr->HasBegunPlay = true;
  OnBeginPlay();
}
bool MActorComponent::Update(float DeltaTime) {
  if (IsPendingDestroy() || !IsRegistered()) {
    return false;
  }
  OnUpdate(DeltaTime);
  return true;
}
void MActorComponent::RegisterComponent() {
  if (ImplPtr->PendingDestroy ||
      ImplPtr->RegistrationState == EActorComponentRegistrationState::Registered ||
      ImplPtr->RegistrationState == EActorComponentRegistrationState::Registering) {
    return;
  }

  if (Owner == nullptr) {
    M_LOG("RegisterComponent ignored: Owner is null");
    return;
  }

  if (Owner->GetWorld() == nullptr) {
    ImplPtr->RegistrationState = EActorComponentRegistrationState::RegistrationPending;
    return;
  }

  CompleteRegistration();
}

void MActorComponent::UnRegisterComponent() {
  if (ImplPtr->RegistrationState != EActorComponentRegistrationState::Registered &&
      ImplPtr->RegistrationState != EActorComponentRegistrationState::Registering &&
      ImplPtr->RegistrationState != EActorComponentRegistrationState::RegistrationPending) {
    return;
  }

  OnUnregister();
  ImplPtr->RegistrationState = ImplPtr->PendingDestroy ? EActorComponentRegistrationState::PendingDestroy
                                      : EActorComponentRegistrationState::Created;
}

bool MActorComponent::CompleteRegistration() {
  if (ImplPtr->PendingDestroy ||
      ImplPtr->RegistrationState == EActorComponentRegistrationState::Registered ||
      ImplPtr->RegistrationState == EActorComponentRegistrationState::Registering) {
    return false;
  }

  if (Owner == nullptr || Owner->GetWorld() == nullptr) {
    ImplPtr->RegistrationState = EActorComponentRegistrationState::RegistrationPending;
    return false;
  }

  ImplPtr->RegistrationState = EActorComponentRegistrationState::Registering;
  OnRegister();
  ImplPtr->RegistrationState = EActorComponentRegistrationState::Registered;

  if (Owner->HasBegunPlay()) {
    BeginPlay();
  }

  return true;
}
void MActorComponent::DestroyComponent() {
  if (ImplPtr->PendingDestroy) {
    return;
  }

  if (Owner != nullptr) {
    const auto* rootComponent = Owner->GetRootComponent();
    if (rootComponent != nullptr && static_cast<const MActorComponent*>(rootComponent) == this) {
      M_LOG("DestroyComponent ignored: attempted to destroy RootComponent");
      return;
    }
  }

  ImplPtr->PendingDestroy = true;

  if (GetOwner() && GetOwner()->GetWorld() && GetOwner()->GetWorld()->GetTimerManager()) {
    GetOwner()->GetWorld()->GetTimerManager()->ClearAllTimersForObject(this);
  }

  HttpManager::GetInstance().CancelAllRequestsForObject(this);
  UnRegisterComponent();

  OnComponentDestroy();
  ImplPtr->RegistrationState = EActorComponentRegistrationState::PendingDestroy;
}

void MActorComponent::SetNetComponentName(std::string Name) { ImplPtr->NetComponentName = std::move(Name); }

void MActorComponent::SerializeNetworkState(FNetBuffer& OutBuffer) {
  for (const auto& replicatedProperty : ImplPtr->ReplicatedProperties) {
    if (replicatedProperty) {
      replicatedProperty->Serialize(OutBuffer);
    }
  }
}

bool MActorComponent::DeserializeNetworkState(FNetBuffer& InBuffer) {
  for (const auto& replicatedProperty : ImplPtr->ReplicatedProperties) {
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
  if (ImplPtr->ReplicatedStateDirty) {
    return true;
  }

  return std::any_of(
      ImplPtr->ReplicatedProperties.begin(),
      ImplPtr->ReplicatedProperties.end(),
      [](const std::unique_ptr<IReplicatedProperty>& replicatedProperty) {
        return replicatedProperty && replicatedProperty->HasChanged();
      }
  );
}

void MActorComponent::UpdateReplicatedStateCache() {
  for (const auto& replicatedProperty : ImplPtr->ReplicatedProperties) {
    if (replicatedProperty) {
      replicatedProperty->UpdateCache();
    }
  }
  ImplPtr->ReplicatedStateDirty = false;
}

void MActorComponent::MarkReplicatedStateDirty() { ImplPtr->ReplicatedStateDirty = true; }

uint32_t MActorComponent::IncrementReplicationSequence() {
  ++ImplPtr->ReplicationSequence;
  return ImplPtr->ReplicationSequence;
}

void MActorComponent::SetLastReceivedReplicationSequence(uint32_t Sequence) {
  ImplPtr->LastReceivedReplicationSequence = Sequence;
}

void MActorComponent::RegisterRPC(FNetworkRPCId RPCId, ENetRPCType RPCType, FRPCHandler Handler) {
  if (RPCId == 0 || !Handler) {
    return;
  }

  ImplPtr->RPCHandlers[RPCId] = {RPCType, std::move(Handler)};
}

bool MActorComponent::DispatchRPC(FNetworkRPCId RPCId, ENetRPCType RPCType, FNetBuffer& Payload) {
  auto it = ImplPtr->RPCHandlers.find(RPCId);
  if (it == ImplPtr->RPCHandlers.end() || it->second.Type != RPCType || !it->second.Handler) {
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
