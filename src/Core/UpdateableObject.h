#pragma once
#include "BaseObject.h"
#include <string>
#include <vector>
#include <memory>
#include "Components/SceneComponent.h"
class MUpdateableObject :
    public MBaseObject
{
public:
    void AddComponent(std::unique_ptr<MSceneComponent> comp);
    virtual bool Update(float DeltaTime) final;
    const std::vector<std::unique_ptr<MSceneComponent>>& GetComponents() const;

    template <class T>
    T* GetComponent() const {
        for (const auto& comp : m_components) {
            if (auto casted = dynamic_cast<T*>(comp.get())) return casted;
        }
        return nullptr;
    }
private:
	std::vector<std::unique_ptr<MSceneComponent>> m_components;
protected:
    virtual void OnUpdate(float DeltaTime) {}
};
 
