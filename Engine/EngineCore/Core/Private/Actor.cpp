#include "Actor.h"
#include "SceneComponent.h"
#include "ActorComponent.h"
#include "TimerManager.h"
#include <algorithm>
#include <cmath>
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
	MActorComponent* NewComponentPtr = NewComponent.get();

	NewComponentPtr->SetOwner(this);
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

void AActor::SerializeNetworkState(FNetBuffer& OutBuffer)
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

bool AActor::DeserializeNetworkState(FNetBuffer& InBuffer)
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

void AActor::SerializeNetworkSpawn(FNetBuffer& OutBuffer)
{
	OutBuffer.WriteString(GetActorClassName());
	SerializeNetworkState(OutBuffer);
}

bool AActor::DeserializeNetworkSpawn(FNetBuffer& InBuffer)
{
	std::string className;
	if (!InBuffer.ReadString(className)) {
		return false;
	}

	return DeserializeNetworkState(InBuffer);
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

	return std::any_of(
		ReplicatedProperties.begin(),
		ReplicatedProperties.end(),
		[](const std::unique_ptr<IReplicatedProperty>& replicatedProperty) {
			return replicatedProperty && replicatedProperty->HasChanged();
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

bool AActor::InvokeRPCWithPayload(FNetworkRPCId RPCId, ENetRPCType RPCType, ENetPacketReliability Reliability, const FNetBuffer& Payload)
{
	if (RPCId == 0 || NetworkId == 0) {
		return false;
	}

	if (!OwnerWorld) {
		return false;
	}

	if (OwnerWorld->IsStandalone()) {
		FNetBuffer localPayload(Payload.GetData());
		return DispatchRPC(RPCId, RPCType, localPayload);
	}

	MReplicationSystem* replication = OwnerWorld->GetReplicationSystem();
	if (!replication) {
		return false;
	}

	return replication->SendActorRPC(this, RPCId, RPCType, Reliability, Payload);
}

void AActor::SetRootComponent(MSceneComponent* Component)
{
	if (!Component) return;

	Component->SetOwner(this);

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
