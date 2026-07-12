#include "Actor.h"

#include <algorithm>
#include <cmath>
#include <string>
#include <typeinfo>
#include <vector>

#include "ActorComponent.h"
#include "EngineDefine.h"
#include "ReplicationSystem.h"
#include "SceneComponent.h"
#include "TimerManager.h"
#include "World.h"

#ifdef _EDITOR
#include <DxLib.h>

#include "EditorSelectPointComponent.h"
#endif
#include "HttpManager.h"
#include "Log.h"
namespace {
bool IsNearlyEqual(float A, float B, float Tolerance) { return std::abs(A - B) <= Tolerance; }

bool IsReplicationSequenceNewer(uint32_t IncomingSequence, uint32_t CurrentSequence) {
  return static_cast<int32_t>(IncomingSequence - CurrentSequence) > 0;
}
}  // namespace

struct AActor::Impl {
  std::vector<std::string> Tags;
  bool PendingDestroy = false;
  bool HasBegunPlay = false;
  std::vector<std::unique_ptr<MActorComponent>> Components;
  World* OwnerWorld = nullptr;
  FVector2D LastReplicatedLocation = FVector2D::ZeroVector();
  FRotator LastReplicatedRotation;
  FScale LastReplicatedScale;
  bool HasReplicatedStateCache = false;
  bool ReplicatedStateDirty = false;
  std::vector<std::unique_ptr<IReplicatedProperty>> ReplicatedProperties;
  uint32_t ReplicationSequence = 0;
  uint32_t LastReceivedReplicationSequence = 0;
  FNetworkComponentId NextNetworkComponentId = 1;
  std::unordered_map<FNetworkComponentId, MActorComponent*> ComponentsByNetworkId;
  std::unordered_map<FNetworkRPCId, FRPCEntry> RPCHandlers;
};
AActor::AActor() : ImplPtr(new Impl()) {
  RootComponent = NewObject<MSceneComponent>(this);
  if (RootComponent) {
    RootComponent->RegisterComponent();
  }
}

AActor::~AActor() {
  if (ImplPtr) {
    if (ImplPtr->OwnerWorld && ImplPtr->OwnerWorld->GetTimerManager()) {
      ImplPtr->OwnerWorld->GetTimerManager()->ClearAllTimersForObject(this);
    }
    HttpManager::GetInstance().CancelAllRequestsForObject(this);
    delete ImplPtr;
    ImplPtr = nullptr;
  }
}

const std::vector<std::string>& AActor::GetTags() const { return ImplPtr->Tags; }

void AActor::AddTag(const std::string& Tag) { ImplPtr->Tags.push_back(Tag); }

void AActor::RemoveTag(const std::string& Tag) {
  std::erase(ImplPtr->Tags, Tag);
}

uint32_t AActor::GetReplicationSequence() const { return ImplPtr->ReplicationSequence; }

uint32_t AActor::GetLastReceivedReplicationSequence() const {
  return ImplPtr->LastReceivedReplicationSequence;
}

World* AActor::GetWorld() { return ImplPtr->OwnerWorld; }

bool AActor::HasBegunPlay() const { return ImplPtr->HasBegunPlay; }

void AActor::AddReplicatedProperty(std::unique_ptr<IReplicatedProperty> Property) {
  ImplPtr->ReplicatedProperties.push_back(std::move(Property));
  MarkReplicatedStateDirty();
}
void AActor::Spawned() {
  ImplPtr->HasBegunPlay = true;
  BeginPlay();

  for (size_t i = 0; i < ImplPtr->Components.size(); ++i) {
    if (ImplPtr->Components[i] && ImplPtr->Components[i]->IsRegistered() && !ImplPtr->Components[i]->IsPendingDestroy()) {
      ImplPtr->Components[i]->BeginPlay();
    }
  }
}

void AActor::SetWorld(World* world) {
  if (ImplPtr->OwnerWorld == world) return;
  ImplPtr->OwnerWorld = world;

  CompletePendingComponentRegistrations();
#ifdef _EDITOR
  auto* SelectPoint = NewObject<EditorSelectPointComponent>(this);
  if (SelectPoint) {
    SelectPoint->RegisterComponent();
  }
#endif
}

void AActor::OnUpdate(float DeltaTime) {}

const std::vector<std::unique_ptr<MActorComponent>>& AActor::GetComponents() const {
  return ImplPtr->Components;
}

MActorComponent* AActor::AcceptNewObjectComponent(std::unique_ptr<MActorComponent> NewComponent) {
  if (!NewComponent) {
    return nullptr;
  }

  MActorComponent* NewComponentPtr = NewComponent.get();

  NewComponentPtr->SetOwner(this);
  EnsureNetComponentName(NewComponentPtr);
  if (auto sceneComp = dynamic_cast<MSceneComponent*>(NewComponentPtr)) {
    if (auto gameObject = dynamic_cast<AActor*>(this)) {
      if (gameObject->GetRootComponent() && gameObject->GetRootComponent() != sceneComp &&
          sceneComp->GetParentComponent() == nullptr) {
        sceneComp->AttachToComponent(GetRootComponent());
      }
    }
  }

  ImplPtr->Components.push_back(std::move(NewComponent));
  return NewComponentPtr;
}

void AActor::CompletePendingComponentRegistrations() {
  for (size_t i = 0; i < ImplPtr->Components.size(); ++i) {
    if (ImplPtr->Components[i] && ImplPtr->Components[i]->IsRegistrationPending()) {
      ImplPtr->Components[i]->CompleteRegistration();
    }
  }
}

MActorComponent* AActor::FindReplicatedComponent(FNetworkComponentId ComponentNetworkId) const {
  if (ComponentNetworkId == 0) {
    return nullptr;
  }

  auto it = ImplPtr->ComponentsByNetworkId.find(ComponentNetworkId);
  if (it != ImplPtr->ComponentsByNetworkId.end()) {
    return it->second;
  }

  for (const auto& comp : ImplPtr->Components) {
    if (comp && comp->ComponentNetworkId == ComponentNetworkId) {
      return comp.get();
    }
  }

  return nullptr;
}

MActorComponent* AActor::FindReplicatedComponentByName(std::string_view NetComponentName) const {
  if (NetComponentName.empty()) {
    return nullptr;
  }

  for (const auto& comp : ImplPtr->Components) {
    if (comp && comp->GetNetComponentName() == NetComponentName) {
      return comp.get();
    }
  }

  return nullptr;
}

void AActor::AssignNetworkComponentIds() {
  ImplPtr->ComponentsByNetworkId.clear();
  if (NetworkId == 0) {
    return;
  }

  for (const auto& comp : ImplPtr->Components) {
    if (!comp || !comp->bReplicates || comp->IsPendingDestroy() || !comp->IsRegistered()) {
      continue;
    }

    EnsureNetComponentName(comp.get());
    if (comp->ComponentNetworkId == 0) {
      comp->ComponentNetworkId = ImplPtr->NextNetworkComponentId++;
    }

    ImplPtr->ComponentsByNetworkId[comp->ComponentNetworkId] = comp.get();
  }
}

void AActor::Update(float DeltaTime) {

  this->OnUpdate(DeltaTime);

  for (size_t i = 0; i < ImplPtr->Components.size(); ++i) {
    if (ImplPtr->Components[i] && ImplPtr->Components[i]->IsRegistered() && !ImplPtr->Components[i]->IsPendingDestroy()) {
      ImplPtr->Components[i]->Update(DeltaTime);
    }
  }

  
  std::erase_if(ImplPtr->Components, [](const std::unique_ptr<MActorComponent>& comp) {
    return !comp || comp->IsPendingDestroy();
  });
}

void AActor::Draw() {
  for (size_t i = 0; i < ImplPtr->Components.size(); ++i) {
    if (ImplPtr->Components[i] && ImplPtr->Components[i]->IsRegistered() && !ImplPtr->Components[i]->IsPendingDestroy()) {
      ImplPtr->Components[i]->Draw();
    }
  }
}

FVector2D AActor::GetActorLocation() const { return RootComponent->GetWorldLocation(); }

bool AActor::SetActorLocation(const FVector2D& NewLocation) {
  RootComponent->SetWorldLocation(NewLocation);
  return true;
}

void AActor::AddActorWorldOffset(const FVector2D& Offset) { RootComponent->AddWorldOffset(Offset); }

void AActor::AddActorLocalOffset(const FVector2D& Offset) { RootComponent->AddLocalOffset(Offset); }

FRotator AActor::GetActorRotation() const { return FRotator(RootComponent->GetWorldRotation()); }

bool AActor::SetActorRotation(const FRotator& NewRotation) {
  RootComponent->SetWorldRotation(NewRotation);
  return true;
}

void AActor::AddActorRotation(const FRotator& DeltaRotation) {
  RootComponent->AddWorldRotation(DeltaRotation);
}

void AActor::Destroy() {
  if (ImplPtr) {
    ImplPtr->PendingDestroy = true;
    if (ImplPtr->OwnerWorld && ImplPtr->OwnerWorld->GetTimerManager()) {
      ImplPtr->OwnerWorld->GetTimerManager()->ClearAllTimersForObject(this);
    }
    HttpManager::GetInstance().CancelAllRequestsForObject(this);
  }
}

bool AActor::IsPendingDestroy() const { return ImplPtr->PendingDestroy; }

FScale AActor::GetActorScale() const { return RootComponent->GetWorldScale(); }

bool AActor::SetActorScale(FScale NewScale) {
  RootComponent->SetWorldScale(NewScale);
  return true;
}

bool AActor::SerializeNetworkState(FNetBuffer& OutBuffer) {
  SerializeActorNetworkState(OutBuffer);
  return SerializeReplicatedComponentStates(OutBuffer);
}

bool AActor::DeserializeNetworkState(FNetBuffer& InBuffer) {
  if (!DeserializeActorNetworkState(InBuffer, !bIsLocallyControlled)) {
    return false;
  }

  return DeserializeReplicatedComponentStates(InBuffer);
}

bool AActor::SerializeNetworkSpawn(FNetBuffer& OutBuffer) {
  OutBuffer.WriteString(GetActorClassName());
  SerializeActorNetworkState(OutBuffer);
  return SerializeReplicatedComponentSpawns(OutBuffer);
}

bool AActor::DeserializeNetworkSpawn(FNetBuffer& InBuffer) {
  if (!DeserializeActorNetworkState(InBuffer)) {
    return false;
  }

  return DeserializeReplicatedComponentSpawns(InBuffer);
}

bool AActor::HasReplicatedStateChanged(float Tolerance) const {
  if (!ImplPtr->HasReplicatedStateCache) {
    return true;
  }

  const FVector2D location = GetActorLocation();
  const FRotator rotation = GetActorRotation();
  const FScale scale = GetActorScale();

  if (ImplPtr->ReplicatedStateDirty || !location.Equals(ImplPtr->LastReplicatedLocation, Tolerance) ||
      !IsNearlyEqual(rotation.Rotation, ImplPtr->LastReplicatedRotation.Rotation, Tolerance) ||
      !IsNearlyEqual(scale.Scale, ImplPtr->LastReplicatedScale.Scale, Tolerance)) {
    return true;
  }

  if (std::any_of(
          ImplPtr->ReplicatedProperties.begin(),
          ImplPtr->ReplicatedProperties.end(),
          [](const std::unique_ptr<IReplicatedProperty>& replicatedProperty) {
            return replicatedProperty && replicatedProperty->HasChanged();
          }
      )) {
    return true;
  }

  return std::any_of(
      ImplPtr->Components.begin(), ImplPtr->Components.end(), [](const std::unique_ptr<MActorComponent>& comp) {
        return comp && comp->bReplicates && comp->IsRegistered() && !comp->IsPendingDestroy() &&
               comp->HasReplicatedStateChanged();
      }
  );
}

void AActor::UpdateReplicatedStateCache() {
  ImplPtr->LastReplicatedLocation = GetActorLocation();
  ImplPtr->LastReplicatedRotation = GetActorRotation();
  ImplPtr->LastReplicatedScale = GetActorScale();
  for (const auto& replicatedProperty : ImplPtr->ReplicatedProperties) {
    if (replicatedProperty) {
      replicatedProperty->UpdateCache();
    }
  }
  for (const auto& comp : ImplPtr->Components) {
    if (comp && comp->bReplicates && comp->IsRegistered() && !comp->IsPendingDestroy()) {
      comp->UpdateReplicatedStateCache();
    }
  }
  ImplPtr->HasReplicatedStateCache = true;
  ImplPtr->ReplicatedStateDirty = false;
}

void AActor::MarkReplicatedStateDirty() { ImplPtr->ReplicatedStateDirty = true; }

uint32_t AActor::IncrementReplicationSequence() {
  ++ImplPtr->ReplicationSequence;
  return ImplPtr->ReplicationSequence;
}

void AActor::SetLastReceivedReplicationSequence(uint32_t Sequence) {
  ImplPtr->LastReceivedReplicationSequence = Sequence;
}

void AActor::RegisterRPC(FNetworkRPCId RPCId, ENetRPCType RPCType, FRPCHandler Handler) {
  if (RPCId == 0 || !Handler) {
    return;
  }

  ImplPtr->RPCHandlers[RPCId] = {RPCType, std::move(Handler)};
}

bool AActor::DispatchRPC(FNetworkRPCId RPCId, ENetRPCType RPCType, FNetBuffer& Payload) {
  auto it = ImplPtr->RPCHandlers.find(RPCId);
  if (it == ImplPtr->RPCHandlers.end() || it->second.Type != RPCType || !it->second.Handler) {
    return false;
  }

  it->second.Handler(Payload);
  return true;
}

bool AActor::DispatchRPC(
    FNetworkComponentId ComponentNetworkId,
    FNetworkRPCId RPCId,
    ENetRPCType RPCType,
    FNetBuffer& Payload
) {
  if (ComponentNetworkId == 0) {
    return DispatchRPC(RPCId, RPCType, Payload);
  }

  MActorComponent* component = FindReplicatedComponent(ComponentNetworkId);
  if (!component) {
    return false;
  }

  return component->DispatchRPC(RPCId, RPCType, Payload);
}

bool AActor::InvokeRPCWithPayload(
    FNetworkRPCId RPCId,
    ENetRPCType RPCType,
    ENetPacketReliability Reliability,
    const FNetBuffer& Payload
) {
  if (RPCId == 0 || !ImplPtr->OwnerWorld) {
    return false;
  }

  if (ImplPtr->OwnerWorld->IsStandalone()) {
    FNetBuffer localPayload(Payload.GetData());
    return DispatchRPC(RPCId, RPCType, localPayload);
  }

  if (NetworkId == 0) {
    return false;
  }

  FReplicationSystem* replication = ImplPtr->OwnerWorld->GetReplicationSystem();
  if (!replication) {
    return false;
  }

  return replication->SendActorRPC(this, RPCId, RPCType, Reliability, Payload);
}

bool AActor::InvokeComponentRPCWithPayload(
    MActorComponent* Component,
    FNetworkRPCId RPCId,
    ENetRPCType RPCType,
    ENetPacketReliability Reliability,
    const FNetBuffer& Payload
) {
  if (!Component || Component->GetOwner() != this || RPCId == 0 || !ImplPtr->OwnerWorld) {
    return false;
  }

  if (ImplPtr->OwnerWorld->IsStandalone()) {
    FNetBuffer localPayload(Payload.GetData());
    return Component->DispatchRPC(RPCId, RPCType, localPayload);
  }

  if (NetworkId == 0) {
    return false;
  }

  if (ImplPtr->OwnerWorld->IsServer()) {
    AssignNetworkComponentIds();
  }

  if (!Component->bReplicates || Component->ComponentNetworkId == 0) {
    return false;
  }

  FReplicationSystem* replication = ImplPtr->OwnerWorld->GetReplicationSystem();
  if (!replication) {
    return false;
  }

  return replication->SendActorRPC(
      this, RPCId, RPCType, Reliability, Payload, Component->ComponentNetworkId
  );
}

void AActor::SetRootComponent(MSceneComponent* Component) {
  if (Component == nullptr || Component == RootComponent) {
    return;
  }

  Component->SetOwner(this);
  EnsureNetComponentName(Component);

  MSceneComponent* OldRoot = RootComponent;
  Component->AttachToComponent(nullptr);

  if (OldRoot != nullptr) {
    OldRoot->AttachToComponent(Component);
  }

  RootComponent = Component;
}

FTimerManager& AActor::GetWorldTimerManager() { return *(ImplPtr->OwnerWorld->GetTimerManager()); }

bool AActor::HasTag(std::string_view Tag) const {
  return std::any_of(ImplPtr->Tags.begin(), ImplPtr->Tags.end(), [&](const std::string& existing) {
    return existing == Tag;
  });
}

void AActor::EnsureNetComponentName(MActorComponent* Component) {
  if (!Component) {
    return;
  }

  std::string candidate = Component->GetNetComponentName();
  if (candidate.empty()) {
    candidate = std::string(typeid(*Component).name()) + "_" + std::to_string(ImplPtr->Components.size());
  }

  const std::string baseName = candidate;
  uint32_t suffix = 1;
  bool bRenamed = false;
  while (FindReplicatedComponentByName(candidate) != nullptr &&
         FindReplicatedComponentByName(candidate) != Component) {
    candidate = baseName + "_" + std::to_string(suffix++);
    bRenamed = true;
  }

  if (bRenamed) {
    M_LOG("Duplicate NetComponentName on {}. Renamed to {}", GetActorClassName(), candidate);
  }

  Component->SetNetComponentName(std::move(candidate));
}

std::vector<MActorComponent*> AActor::GetReplicatedNetworkComponents() const {
  std::vector<MActorComponent*> replicatedComponents;
  for (const auto& comp : ImplPtr->Components) {
    if (comp && comp->bReplicates && comp->IsRegistered() && !comp->IsPendingDestroy() && comp->ComponentNetworkId != 0) {
      replicatedComponents.push_back(comp.get());
    }
  }

  return replicatedComponents;
}

void AActor::SerializeActorNetworkState(FNetBuffer& OutBuffer) {
  const FVector2D location = GetActorLocation();
  const FRotator rotation = GetActorRotation();
  const FScale scale = GetActorScale();

  OutBuffer.Write(location.X);
  OutBuffer.Write(location.Y);
  OutBuffer.Write(rotation.Rotation);
  OutBuffer.Write(scale.Scale);

  for (const auto& replicatedProperty : ImplPtr->ReplicatedProperties) {
    if (replicatedProperty) {
      replicatedProperty->Serialize(OutBuffer);
    }
  }
}

bool AActor::DeserializeActorNetworkState(FNetBuffer& InBuffer, bool bApplyTransform) {
  float locationX = 0.0f;
  float locationY = 0.0f;
  float rotation = 0.0f;
  float scale = 1.0f;

  if (!InBuffer.Read(locationX)) return false;
  if (!InBuffer.Read(locationY)) return false;
  if (!InBuffer.Read(rotation)) return false;
  if (!InBuffer.Read(scale)) return false;

  if (bApplyTransform) {
    SetActorLocation({locationX, locationY});
    SetActorRotation(FRotator(rotation));
    SetActorScale(FScale(scale));
  }

  for (const auto& replicatedProperty : ImplPtr->ReplicatedProperties) {
    if (replicatedProperty && !replicatedProperty->DeserializeAndTriggerOnRep(InBuffer)) {
      return false;
    }
  }

  return true;
}

bool AActor::SerializeReplicatedComponentStates(FNetBuffer& OutBuffer) {
  AssignNetworkComponentIds();
  const std::vector<MActorComponent*> replicatedComponents = GetReplicatedNetworkComponents();
  const uint32_t componentCount = static_cast<uint32_t>(replicatedComponents.size());
  OutBuffer.Write(componentCount);

  for (MActorComponent* component : replicatedComponents) {
    FNetBuffer componentPayload;
    component->SerializeNetworkState(componentPayload);

    OutBuffer.Write(component->ComponentNetworkId);
    OutBuffer.Write(component->IncrementReplicationSequence());
    if (!OutBuffer.WriteBufferWithSize(componentPayload)) {
      return false;
    }
  }

  return true;
}

bool AActor::DeserializeReplicatedComponentStates(FNetBuffer& InBuffer) {
  if (InBuffer.GetRemainingSize() == 0) {
    return true;
  }

  uint32_t componentCount = 0;
  if (!InBuffer.Read(componentCount)) {
    return false;
  }

  for (uint32_t i = 0; i < componentCount; ++i) {
    FNetworkComponentId componentNetworkId = 0;
    uint32_t sequence = 0;
    if (!InBuffer.Read(componentNetworkId)) return false;
    if (!InBuffer.Read(sequence)) return false;

    FNetBuffer payload;
    if (!InBuffer.ReadBufferWithSize(payload)) {
      return false;
    }

    MActorComponent* component = FindReplicatedComponent(componentNetworkId);
    if (!component) {
      continue;
    }

    if (!IsReplicationSequenceNewer(sequence, component->GetLastReceivedReplicationSequence())) {
      continue;
    }

    if (!component->DeserializeNetworkState(payload)) {
      return false;
    }

    component->SetLastReceivedReplicationSequence(sequence);
    component->UpdateReplicatedStateCache();
  }

  return true;
}

bool AActor::SerializeReplicatedComponentSpawns(FNetBuffer& OutBuffer) {
  AssignNetworkComponentIds();
  const std::vector<MActorComponent*> replicatedComponents = GetReplicatedNetworkComponents();
  const uint32_t componentCount = static_cast<uint32_t>(replicatedComponents.size());
  OutBuffer.Write(componentCount);

  for (MActorComponent* component : replicatedComponents) {
    FNetBuffer componentPayload;
    component->SerializeNetworkSpawn(componentPayload);

    OutBuffer.Write(component->ComponentNetworkId);
    OutBuffer.WriteString(component->GetNetComponentName());
    OutBuffer.Write(component->GetReplicationSequence());
    if (!OutBuffer.WriteBufferWithSize(componentPayload)) {
      return false;
    }
  }

  return true;
}

bool AActor::DeserializeReplicatedComponentSpawns(FNetBuffer& InBuffer) {
  if (InBuffer.GetRemainingSize() == 0) {
    return true;
  }

  uint32_t componentCount = 0;
  if (!InBuffer.Read(componentCount)) {
    return false;
  }

  ImplPtr->ComponentsByNetworkId.clear();
  for (uint32_t i = 0; i < componentCount; ++i) {
    FNetworkComponentId componentNetworkId = 0;
    std::string netComponentName;
    uint32_t sequence = 0;
    if (!InBuffer.Read(componentNetworkId)) return false;
    if (!InBuffer.ReadString(netComponentName)) return false;
    if (!InBuffer.Read(sequence)) return false;

    FNetBuffer payload;
    if (!InBuffer.ReadBufferWithSize(payload)) {
      return false;
    }

    MActorComponent* component = FindReplicatedComponentByName(netComponentName);
    if (!component) {
      continue;
    }

    component->bReplicates = true;
    component->ComponentNetworkId = componentNetworkId;
    ImplPtr->ComponentsByNetworkId[componentNetworkId] = component;

    if (!component->DeserializeNetworkSpawn(payload)) {
      return false;
    }

    component->SetLastReceivedReplicationSequence(sequence);
    component->UpdateReplicatedStateCache();
  }

  return true;
}
