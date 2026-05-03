#include "Core/GameObject.h"

AGameObject::AGameObject()
{
	auto root = std::make_unique<MSceneComponent>();
	m_rootComponent = root.get();
    AddComponent(std::move(root));
}

void AGameObject::OnUpdate(float DeltaTime)
{
}

void AGameObject::Draw()
{
    for (auto& comp : GetComponents()) comp->Draw();
    this->OnDraw();
}

FVector2D AGameObject::GetActorLocation() const
{
    return m_rootComponent->GetWorldLocation();
}

bool AGameObject::SetActorLocation(const FVector2D& NewLocation)
{
	m_rootComponent->SetWorldLocation(NewLocation);
    return true;
}

void AGameObject::AddActorWorldOffset(const FVector2D& Offset)
{
    m_rootComponent->AddWorldOffset(Offset);
}

void AGameObject::AddActorLocalOffset(const FVector2D& Offset)
{
    m_rootComponent->AddLocalOffset(Offset);
}

FRotator AGameObject::GetActorRotation() const
{
    return FRotator(m_rootComponent->GetWorldRotation());
}

bool AGameObject::SetActorRotation(const FRotator& NewRotation)
{
	m_rootComponent->SetWorldRotation(NewRotation.Rotation);
    return true;
}

void AGameObject::AddActorRotation(const FRotator& DeltaRotation)
{
    m_rootComponent->AddWorldRotation(DeltaRotation.Rotation);
}

FScale AGameObject::GetActorScale() const
{
    return m_rootComponent->GetScale();
}

bool AGameObject::SetActorScale(float NewScale)
{
	m_rootComponent->SetScale(NewScale);
   return true;
}
