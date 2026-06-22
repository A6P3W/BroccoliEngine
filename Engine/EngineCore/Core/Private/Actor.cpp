#include "Actor.h"
#include "SceneComponent.h"
#include "ActorComponent.h"
#include "TimerManager.h"
#include <algorithm>
#include "World.h"
#include "EngineDefine.h"

#ifdef _EDITOR
#include "EditorSelectPointComponent.h"
#include <DxLib.h>
#endif
#include "Log.h"

#include "HttpManager.h"
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
