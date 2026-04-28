#include "OUpdateableObject.h"

void OUpdateableObject::AddComponent(std::unique_ptr<Component> comp)
{
    comp->SetOwner(this);
    m_components.push_back(std::move(comp));
}

bool OUpdateableObject::Update()
{
    for (auto& comp : m_components) comp->Update();
    this->OnUpdate();
    return true;
}

const std::vector<std::unique_ptr<Component>>& OUpdateableObject::GetComponents() const
{
    return m_components;
}
