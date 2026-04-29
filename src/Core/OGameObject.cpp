#include "Core/OGameObject.h"

OGameObject::OGameObject()
{
    auto transform = std::make_unique<TransformComponent>();
    m_transform = transform.get();
    AddComponent(std::move(transform));
}

void OGameObject::OnUpdate(float DeltaTime)
{
}

void OGameObject::Draw()
{
    for (auto& comp : GetComponents()) comp->Draw();
    this->OnDraw();
}

TransformComponent* OGameObject::GetTransform() const
{

    return m_transform;
}
