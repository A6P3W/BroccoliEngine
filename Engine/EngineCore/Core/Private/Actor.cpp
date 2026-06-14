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
AActor::AActor()
{
	auto root = std::make_unique<MSceneComponent>();
	m_rootComponent = root.get();
	AddComponent(std::move(root));

}

AActor::~AActor()
{
	if (m_world && m_world->GetTimerManager()) {
		m_world->GetTimerManager()->ClearAllTimersForObject(this);
	}
}

void AActor::Spawned()
{
	M_LOG("Actor spawned: {}", GetActorClassName());
	BeginPlay();

	for (size_t i = 0; i < m_components.size(); ++i) {
		if (m_components[i] && !m_components[i]->IsPendingDestroy()) {
			m_components[i]->BeginPlay();
		}
	}
}

void AActor::SetWorld(World* world)
{
	if (m_world == world) return;
	m_world = world;

	for (size_t i = 0; i < m_components.size(); ++i) {
		if (m_components[i]) {
			m_components[i]->RegisterComponent();
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
	return m_components;
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

	m_components.push_back(std::move(NewComponent));

	if (m_world) {
		NewComponentPtr->RegisterComponent();
	}
	
}
void AActor::Update(float DeltaTime)
{
	for (size_t i = 0; i < m_components.size(); ++i) {
		if (m_components[i] && !m_components[i]->IsPendingDestroy()) {
			m_components[i]->Update(DeltaTime);
		}
	}

	this->OnUpdate(DeltaTime);
	std::erase_if(m_components, [](const std::unique_ptr<MActorComponent>& comp) {
		return !comp || comp->IsPendingDestroy();
		});
}

void AActor::Draw()
{
	for (size_t i = 0; i < m_components.size(); ++i) {
		if (m_components[i] && !m_components[i]->IsPendingDestroy()) {
			m_components[i]->Draw();
		}
	}
}

FVector2D AActor::GetActorLocation() const
{
	return m_rootComponent->GetWorldLocation();
}

bool AActor::SetActorLocation(const FVector2D& NewLocation)
{
	m_rootComponent->SetWorldLocation(NewLocation);
	return true;
}

void AActor::AddActorWorldOffset(const FVector2D& Offset)
{
	m_rootComponent->AddWorldOffset(Offset);
}

void AActor::AddActorLocalOffset(const FVector2D& Offset)
{
	m_rootComponent->AddLocalOffset(Offset);
}

FRotator AActor::GetActorRotation() const
{
	return FRotator(m_rootComponent->GetWorldRotation());
}

bool AActor::SetActorRotation(const FRotator& NewRotation)
{
	m_rootComponent->SetWorldRotation(NewRotation);
	return true;
}

void AActor::AddActorRotation(const FRotator& DeltaRotation)
{
	m_rootComponent->AddWorldRotation(DeltaRotation);
}

void AActor::Destroy()
{
	m_PendingDestroy = true;
	if (m_world && m_world->GetTimerManager()) {
		m_world->GetTimerManager()->ClearAllTimersForObject(this);
	}
}

bool AActor::IsPendingDestroy() const
{
	return m_PendingDestroy;
}

FScale AActor::GetActorScale() const
{
	return m_rootComponent->GetWorldScale();
}

bool AActor::SetActorScale(FScale NewScale)
{
	m_rootComponent->SetWorldScale(NewScale);
	return true;
}

void AActor::SetRootComponent(MSceneComponent* Component)
{
	if (!Component) return;

	Component->SetOwner(this);

	if (m_rootComponent && m_rootComponent != Component) {
		m_rootComponent->SetParentComponent(Component);
	}

	m_rootComponent = Component;
}

TimerManager& AActor::GetWorldTimerManager()
{
	return *(m_world->GetTimerManager());
}

bool AActor::HasTag(std::string_view Tag) const
{
	return std::any_of(Tags.begin(), Tags.end(), [&](const std::string& existing) {
		return existing == Tag;
	});
}
