#include "UpdateableObject.h"

void MUpdateableObject::AddComponent(std::unique_ptr<MSceneComponent> comp)
{
    comp->SetOwner(this);
    m_components.push_back(std::move(comp));
}

bool MUpdateableObject::Update(float DeltaTime)
{
    for (auto& comp : m_components) comp->Update(DeltaTime);
    this->OnUpdate(DeltaTime);
    return true;
}

const std::vector<std::unique_ptr<MSceneComponent>>& MUpdateableObject::GetComponents() const
{
    return m_components;
}
