#include "Core/GameObject.h"

AGameObject::AGameObject()
{
    auto transform = std::make_unique<MTransformComponent>();
    m_transform = transform.get();
    AddComponent(std::move(transform));
}

void AGameObject::OnUpdate(float DeltaTime)
{
}

void AGameObject::Draw()
{
    for (auto& comp : GetComponents()) comp->Draw();
    this->OnDraw();
}

MTransformComponent* AGameObject::GetTransform() const
{

    return m_transform;
}
