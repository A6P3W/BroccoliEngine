#include "UpdateableObject.h"
#include "Components/SceneComponent.h"

void MUpdateableObject::AddComponent(std::unique_ptr<UpdateableComponent> comp)
{
    if (auto sceneComp = dynamic_cast<MSceneComponent*>(comp.get())) {
        sceneComp->SetOwner(this);
    }
    m_components.push_back(std::move(comp));
}

bool MUpdateableObject::Update(float DeltaTime)
{
    for (auto& comp : m_components) comp->Update(DeltaTime);
    this->OnUpdate(DeltaTime);
    return true;
}

const std::vector<std::unique_ptr<UpdateableComponent>>& MUpdateableObject::GetComponents() const
{
    return m_components;
}
