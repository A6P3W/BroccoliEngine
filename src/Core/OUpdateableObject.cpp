#include "OUpdateableObject.h"

void OUpdateableObject::AddComponent(std::unique_ptr<Component> comp)
{
    comp->SetOwner(this);
    m_components.push_back(std::move(comp));
}

bool OUpdateableObject::Update(float DeltaTime)
{
    for (auto& comp : m_components) comp->Update(DeltaTime);
    this->OnUpdate(DeltaTime);
    return true;
}

const std::vector<std::unique_ptr<Component>>& OUpdateableObject::GetComponents() const
{
    return m_components;
}
