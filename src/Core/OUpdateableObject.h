#pragma once
#include "OBaseObject.h"
#include <string>
#include <vector>
#include <memory>
#include "Components/Component.h"
class OUpdateableObject :
    public OBaseObject
{
public:
    void AddComponent(std::unique_ptr<Component> comp);
    virtual bool Update() final;
    const std::vector<std::unique_ptr<Component>>& GetComponents() const;

    // GetComponent テンプレートもここに置いておくと便利
    template <class T>
    T* GetComponent() const {
        for (const auto& comp : m_components) {
            if (auto casted = dynamic_cast<T*>(comp.get())) return casted;
        }
        return nullptr;
    }
private:
	std::vector<std::unique_ptr<Component>> m_components;
protected:
    virtual void OnUpdate() {}
};
 
