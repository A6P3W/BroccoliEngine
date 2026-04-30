#include "Components/SceneComponent.h"

MSceneComponent::MSceneComponent() = default;
MSceneComponent::~MSceneComponent() = default;

void MSceneComponent::OnUpdate(float DeltaTime)
{
}

void MSceneComponent::Draw()
{
}

void MSceneComponent::OnMessage(const std::string& message)
{
}

void MSceneComponent::SetOwner(MUpdateableObject* owner)
{
    m_owner = owner;
}
