#include "Actor.h"
#include "SceneComponent.h"
#include "ActorComponent.h"
#include "TimerManager.h"
#include <algorithm>
#include <cmath>
#include <string>
#include <typeinfo>
#include <vector>
#include "World.h"
#include "EngineDefine.h"
#include "ReplicationSystem.h"

#ifdef _EDITOR
#include "EditorSelectPointComponent.h"
#include <DxLib.h>
#endif
#include "Log.h"

#include "HttpManager.h"
namespace
{
	bool IsNearlyEqual(float A, float B, float Tolerance)
	{
		return std::abs(A - B) <= Tolerance;
	}

	bool IsReplicationSequenceNewer(uint32_t IncomingSequence, uint32_t CurrentSequence)
	{
		return static_cast<int32_t>(IncomingSequence - CurrentSequence) > 0;
	}
}

AActor::AActor()
{
	auto root = std::make_unique<MSceneComponent>();
	RootComponent = root.get();
	AddComponent(std::move(root));

}

AActor::~AActor()
{
	if (OwnerWorld && OwnerWorld->GetTimerManager()) {
		OwnerWorld->GetTimerManager()->ClearAllTimersForObject(this);
	}
	HttpManager::GetInstance().CancelAllRequestsForObject(this);
}

void AActor::Spawned()
{
	M_LOG("Actor spawned: {}", GetActorClassName());
	BeginPlay();

	for (size_t i = 0; i < Components.size(); ++i) {
		if (Components[i] && !Components[i]->IsPendingDestroy()) {
			Components[i]->BeginPlay();
		}
	}
}

void AActor::SetWorld(World* world)
{
	if (OwnerWorld == world) return;
	OwnerWorld = world;

	for (size_t i = 0; i < Components.size(); ++i) {
		if (Components[i]) {
			Components[i]->RegisterComponent();
		}
	}
#ifdef _EDITOR
	auto selectPoint = std::make_unique<EditorSelectPointComponent>();
	AddComponent(std::move(selectPoint));
#endif
}

void AActor::OnUpdate(float DeltaTime)
{
}

const std::vector<std::unique_ptr<MActorComponent>>& AActor::GetComponents() const
{
	return Components;
}

void AActor::AddComponent(std::unique_ptr<MActorComponent> NewComponent)
{
	if (!NewComponent) {
		return;
	}

	MActorComponent* NewComponentPtr = NewComponent.get();

	NewComponentPtr->SetOwner(this);
	EnsureNetComponentName(NewComponentPtr);
	if (auto sceneComp = dynamic_cast<MSceneComponent*>(NewComponentPtr)) {
		if (auto gameObject = dynamic_cast<AActor*>(this)) {
			if (gameObject->GetRootComponent() && gameObject->GetRootComponent() != sceneComp && sceneComp->GetParentComponent() == nullptr) {
				sceneComp->SetParentComponent(GetRootComponent());
			}
		}
	}

	Components.push_back(std::move(NewComponent));

	if (OwnerWorld) {
		NewComponentPtr->RegisterComponent();
	}
	
}

MActorComponent* AActor::FindReplicatedComponent(FNetworkComponentId ComponentNetworkId) const
{
	if (ComponentNetworkId == 0) {
		return nullptr;
	}

	auto it = ComponentsByNetworkId.find(ComponentNetworkId);
	if (it != ComponentsByNetworkId.end()) {
		return it->second;
	}

	for (const auto& comp : Components) {
		if (comp && comp->ComponentNetworkId == ComponentNetworkId) {
			return comp.get();
		}
	}

	return nullptr;
}

MActorComponent* AActor::FindReplicatedComponentByName(std::string_view NetComponentName) const
{
	if (NetComponentName.empty()) {
		return nullptr;
	}

	for (const auto& comp : Components) {
		if (comp && comp->GetNetComponentName() == NetComponentName) {
			return comp.get();
		}
	}

	return nullptr;
}

void AActor::AssignNetworkComponentIds()
{
	ComponentsByNetworkId.clear();
	if (NetworkId == 0) {
		return;
	}

	for (const auto& comp : Components) {
		if (!comp || !comp->bReplicates || comp->IsPendingDestroy()) {
			continue;
		}

		EnsureNetComponentName(comp.get());
		if (comp->ComponentNetworkId == 0) {
			comp->ComponentNetworkId = NextNetworkComponentId++;
		}

		ComponentsByNetworkId[comp->ComponentNetworkId] = comp.get();
	}
}

void AActor::Update(float DeltaTime)
{
	for (size_t i = 0; i < Components.size(); ++i) {
		if (Components[i] && !Components[i]->IsPendingDestroy()) {
			Components[i]->Update(DeltaTime);
		}
	}

	this->OnUpdate(DeltaTime);
	std::erase_if(Components, [](const std::unique_ptr<MActorComponent>& comp) {
		return !comp || comp->IsPendingDestroy();
		});
}

void AActor::Draw()
{
	for (size_t i = 0; i < Components.size(); ++i) {
		if (Components[i] && !Components[i]->IsPendingDestroy()) {
			Components[i]->Draw();
		}
	}
}

FVector2D AActor::GetActorLocation() const
{
	return RootComponent->GetWorldLocation();
}

bool AActor::SetActorLocation(const FVector2D& NewLocation)
{
	RootComponent->SetWorldLocation(NewLocation);
	return true;
}

void AActor::AddActorWorldOffset(const FVector2D& Offset)
{
	RootComponent->AddWorldOffset(Offset);
}

void AActor::AddActorLocalOffset(const FVector2D& Offset)
{
	RootComponent->AddLocalOffset(Offset);
}

FRotator AActor::GetActorRotation() const
{
	return FRotator(RootComponent->GetWorldRotation());
}

bool AActor::SetActorRotation(const FRotator& NewRotation)
{
	RootComponent->SetWorldRotation(NewRotation);
	return true;
}

void AActor::AddActorRotation(const FRotator& DeltaRotation)
{
	RootComponent->AddWorldRotation(DeltaRotation);
}

void AActor::Destroy()
{
	bPendingDestroy = true;
	if (OwnerWorld && OwnerWorld->GetTimerManager()) {
		OwnerWorld->GetTimerManager()->ClearAllTimersForObject(this);
	}
	HttpManager::GetInstance().CancelAllRequestsForObject(this);
}

bool AActor::IsPendingDestroy() const
{
	return bPendingDestroy;
}

FScale AActor::GetActorScale() const
{
	return RootComponent->GetWorldScale();
}

bool AActor::SetActorScale(FScale NewScale)
{
	RootComponent->SetWorldScale(NewScale);
	return true;
}

bool AActor::SerializeNetworkState(FNetBuffer& OutBuffer)
{
	SerializeActorNetworkState(OutBuffer);
	return SerializeReplicatedComponentStates(OutBuffer);
}

bool AActor::DeserializeNetworkState(FNetBuffer& InBuffer)
{
	if (!DeserializeActorNetworkState(InBuffer)) {
		return false;
	}

	return DeserializeReplicatedComponentStates(InBuffer);
}

bool AActor::SerializeNetworkSpawn(FNetBuffer& OutBuffer)
{
	OutBuffer.WriteString(GetActorClassName());
	SerializeActorNetworkState(OutBuffer);
	return SerializeReplicatedComponentSpawns(OutBuffer);
}

bool AActor::DeserializeNetworkSpawn(FNetBuffer& InBuffer)
{
	if (!DeserializeActorNetworkState(InBuffer)) {
		return false;
	}

	return DeserializeReplicatedComponentSpawns(InBuffer);
}

bool AActor::HasReplicatedStateChanged(float Tolerance) const
{
	if (!bHasReplicatedStateCache) {
		return true;
	}

	const FVector2D location = GetActorLocation();
	const FRotator rotation = GetActorRotation();
	const FScale scale = GetActorScale();

	if (bReplicatedStateDirty ||
		!location.Equals(LastReplicatedLocation, Tolerance) ||
		!IsNearlyEqual(rotation.Rotation, LastReplicatedRotation.Rotation, Tolerance) ||
		!IsNearlyEqual(scale.Scale, LastReplicatedScale.Scale, Tolerance)) {
		return true;
	}

	if (std::any_of(
		ReplicatedProperties.begin(),
		ReplicatedProperties.end(),
		[](const std::unique_ptr<IReplicatedProperty>& replicatedProperty) {
			return replicatedProperty && replicatedProperty->HasChanged();
		})) {
		return true;
	}

	return std::any_of(
		Components.begin(),
		Components.end(),
		[](const std::unique_ptr<MActorComponent>& comp) {
			return comp && comp->bReplicates && !comp->IsPendingDestroy() && comp->HasReplicatedStateChanged();
		});
}

void AActor::UpdateReplicatedStateCache()
{
	LastReplicatedLocation = GetActorLocation();
	LastReplicatedRotation = GetActorRotation();
	LastReplicatedScale = GetActorScale();
	for (const auto& replicatedProperty : ReplicatedProperties) {
		if (replicatedProperty) {
			replicatedProperty->UpdateCache();
		}
	}
	for (const auto& comp : Components) {
		if (comp && comp->bReplicates && !comp->IsPendingDestroy()) {
			comp->UpdateReplicatedStateCache();
		}
	}
	bHasReplicatedStateCache = true;
	bReplicatedStateDirty = false;
}

void AActor::MarkReplicatedStateDirty()
{
	bReplicatedStateDirty = true;
}

uint32_t AActor::IncrementReplicationSequence()
{
	++ReplicationSequence;
	return ReplicationSequence;
}

void AActor::SetLastReceivedReplicationSequence(uint32_t Sequence)
{
	LastReceivedReplicationSequence = Sequence;
}

void AActor::RegisterRPC(FNetworkRPCId RPCId, ENetRPCType RPCType, FRPCHandler Handler)
{
	if (RPCId == 0 || !Handler) {
		return;
	}

	RPCHandlers[RPCId] = { RPCType, std::move(Handler) };
}

bool AActor::DispatchRPC(FNetworkRPCId RPCId, ENetRPCType RPCType, FNetBuffer& Payload)
{
	auto it = RPCHandlers.find(RPCId);
	if (it == RPCHandlers.end() || it->second.Type != RPCType || !it->second.Handler) {
		return false;
	}

	it->second.Handler(Payload);
	return true;
}

bool AActor::DispatchRPC(FNetworkComponentId ComponentNetworkId, FNetworkRPCId RPCId, ENetRPCType RPCType, FNetBuffer& Payload)
{
	if (ComponentNetworkId == 0) {
		return DispatchRPC(RPCId, RPCType, Payload);
	}

	MActorComponent* component = FindReplicatedComponent(ComponentNetworkId);
	if (!component) {
		return false;
	}

	return component->DispatchRPC(RPCId, RPCType, Payload);
}

bool AActor::InvokeRPCWithPayload(FNetworkRPCId RPCId, ENetRPCType RPCType, ENetPacketReliability Reliability, const FNetBuffer& Payload)
{
	if (RPCId == 0 || !OwnerWorld) {
		return false;
	}

	if (OwnerWorld->IsStandalone()) {
		FNetBuffer localPayload(Payload.GetData());
		return DispatchRPC(RPCId, RPCType, localPayload);
	}

	if (NetworkId == 0) {
		return false;
	}

	MReplicationSystem* replication = OwnerWorld->GetReplicationSystem();
	if (!replication) {
		return false;
	}

	return replication->SendActorRPC(this, RPCId, RPCType, Reliability, Payload);
}

bool AActor::InvokeComponentRPCWithPayload(MActorComponent* Component, FNetworkRPCId RPCId, ENetRPCType RPCType, ENetPacketReliability Reliability, const FNetBuffer& Payload)
{
	if (!Component || Component->GetOwner() != this || RPCId == 0 || !OwnerWorld) {
		return false;
	}

	if (OwnerWorld->IsStandalone()) {
		FNetBuffer localPayload(Payload.GetData());
		return Component->DispatchRPC(RPCId, RPCType, localPayload);
	}

	if (NetworkId == 0) {
		return false;
	}

	if (OwnerWorld->IsServer()) {
		AssignNetworkComponentIds();
	}

	if (!Component->bReplicates || Component->ComponentNetworkId == 0) {
		return false;
	}

	MReplicationSystem* replication = OwnerWorld->GetReplicationSystem();
	if (!replication) {
		return false;
	}

	return replication->SendActorRPC(this, RPCId, RPCType, Reliability, Payload, Component->ComponentNetworkId);
}

void AActor::SetRootComponent(MSceneComponent* Component)
{
	if (!Component) return;

	Component->SetOwner(this);
	EnsureNetComponentName(Component);

	if (RootComponent && RootComponent != Component) {
		RootComponent->SetParentComponent(Component);
	}

	RootComponent = Component;
}

MTimerManager& AActor::GetWorldTimerManager()
{
	return *(OwnerWorld->GetTimerManager());
}

bool AActor::HasTag(std::string_view Tag) const
{
	return std::any_of(Tags.begin(), Tags.end(), [&](const std::string& existing) {
		return existing == Tag;
	});
}

void AActor::EnsureNetComponentName(MActorComponent* Component)
{
	if (!Component) {
		return;
	}

	std::string candidate = Component->GetNetComponentName();
	if (candidate.empty()) {
		candidate = std::string(typeid(*Component).name()) + "_" + std::to_string(Components.size());
	}

	const std::string baseName = candidate;
	uint32_t suffix = 1;
	bool bRenamed = false;
	while (FindReplicatedComponentByName(candidate) != nullptr && FindReplicatedComponentByName(candidate) != Component) {
		candidate = baseName + "_" + std::to_string(suffix++);
		bRenamed = true;
	}

	if (bRenamed) {
		M_LOG("Duplicate NetComponentName on {}. Renamed to {}", GetActorClassName(), candidate);
	}

	Component->SetNetComponentName(std::move(candidate));
}

std::vector<MActorComponent*> AActor::GetReplicatedNetworkComponents() const
{
	std::vector<MActorComponent*> replicatedComponents;
	for (const auto& comp : Components) {
		if (comp && comp->bReplicates && !comp->IsPendingDestroy() && comp->ComponentNetworkId != 0) {
			replicatedComponents.push_back(comp.get());
		}
	}

	return replicatedComponents;
}

void AActor::SerializeActorNetworkState(FNetBuffer& OutBuffer)
{
	const FVector2D location = GetActorLocation();
	const FRotator rotation = GetActorRotation();
	const FScale scale = GetActorScale();

	OutBuffer.Write(location.X);
	OutBuffer.Write(location.Y);
	OutBuffer.Write(rotation.Rotation);
	OutBuffer.Write(scale.Scale);

	for (const auto& replicatedProperty : ReplicatedProperties) {
		if (replicatedProperty) {
			replicatedProperty->Serialize(OutBuffer);
		}
	}
}

bool AActor::DeserializeActorNetworkState(FNetBuffer& InBuffer)
{
	float locationX = 0.0f;
	float locationY = 0.0f;
	float rotation = 0.0f;
	float scale = 1.0f;

	if (!InBuffer.Read(locationX)) return false;
	if (!InBuffer.Read(locationY)) return false;
	if (!InBuffer.Read(rotation)) return false;
	if (!InBuffer.Read(scale)) return false;

	SetActorLocation({ locationX, locationY });
	SetActorRotation(FRotator(rotation));
	SetActorScale(FScale(scale));

	for (const auto& replicatedProperty : ReplicatedProperties) {
		if (replicatedProperty && !replicatedProperty->DeserializeAndTriggerOnRep(InBuffer)) {
			return false;
		}
	}

	return true;
}

bool AActor::SerializeReplicatedComponentStates(FNetBuffer& OutBuffer)
{
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

bool AActor::DeserializeReplicatedComponentStates(FNetBuffer& InBuffer)
{
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

bool AActor::SerializeReplicatedComponentSpawns(FNetBuffer& OutBuffer)
{
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

bool AActor::DeserializeReplicatedComponentSpawns(FNetBuffer& InBuffer)
{
	if (InBuffer.GetRemainingSize() == 0) {
		return true;
	}

	uint32_t componentCount = 0;
	if (!InBuffer.Read(componentCount)) {
		return false;
	}

	ComponentsByNetworkId.clear();
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
		ComponentsByNetworkId[componentNetworkId] = component;

		if (!component->DeserializeNetworkSpawn(payload)) {
			return false;
		}

		component->SetLastReceivedReplicationSequence(sequence);
		component->UpdateReplicatedStateCache();
	}

	return true;
}
