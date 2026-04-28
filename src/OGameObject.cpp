#include "OGameObject.h"

OGameObject::OGameObject()
{
    auto transform = std::make_unique<TransformComponent>();
    m_transform = transform.get();
    AddComponent(std::move(transform));
}

void OGameObject::OnUpdate()
{
}

void OGameObject::Draw()
{
    for (auto& comp : GetComponents()) comp->Draw();
    this->OnDraw();
}
