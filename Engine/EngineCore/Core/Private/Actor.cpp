#include "Actor.h"
#include "SceneComponent.h"
#include "ActorComponent.h"
AActor::AActor()
{
	auto root = std::make_unique<MSceneComponent>();
	m_rootComponent = root.get();
	AddComponent(std::move(root));
}

void AActor::OnUpdate(float DeltaTime)
{
}

const std::vector<std::unique_ptr<MActorComponent>>& AActor::GetComponents() const
{
	return m_components;
}

void AActor::AddComponent(std::unique_ptr<MActorComponent> comp)
{
	comp->SetOwner(this);
	if (auto sceneComp = dynamic_cast<MSceneComponent*>(comp.get())) {

		if (auto gameObject = dynamic_cast<AActor*>(this)) {
			if (gameObject->GetRootComponent() && gameObject->GetRootComponent() != sceneComp && sceneComp->GetParentComponent() == nullptr) {
				sceneComp->SetParentComponent(gameObject->GetRootComponent());
			}
		}
	}
	m_components.push_back(std::move(comp));
}

void AActor::Update(float DeltaTime)
{
	for (auto& comp : m_components) comp->Update(DeltaTime);
	this->OnUpdate(DeltaTime);
}

void AActor::Draw()
{
	for (auto& comp : GetComponents()) comp->Draw();
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
	m_rootComponent->SetWorldRotation(NewRotation.Rotation);
	return true;
}

void AActor::AddActorRotation(const FRotator& DeltaRotation)
{
	m_rootComponent->AddWorldRotation(DeltaRotation.Rotation);
}

void AActor::Destroy()
{
	m_PendingDestroy = true;
}

bool AActor::IsPendingDestroy() const
{
	return m_PendingDestroy;
}

FScale AActor::GetActorScale() const
{
	return m_rootComponent->GetScale();
}

bool AActor::SetActorScale(float NewScale)
{
	m_rootComponent->SetScale(NewScale);
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
