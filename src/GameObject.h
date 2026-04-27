#pragma once
#include <vector>
#include <memory>
#include <string>
#include "Component.h"

class GameObject {
public:
    GameObject() {}
    virtual ~GameObject() = default;
    // コンポーネントの追加
    void AddComponent(std::unique_ptr<Component> comp) {
        comp->SetOwner(this);
        m_components.push_back(std::move(comp));
    }

    virtual bool Update() final {
        for (auto& comp : m_components) comp->Update();
        this -> OnUpdate();
        return true;
    }

    virtual bool Draw() final {
        for (auto& comp : m_components) comp->Draw();
        this -> OnDraw();
        return true;
    }

    // メッセージング（全コンポーネントに通知）
    void SendMessage(const std::string& message) {
        for (auto& comp : m_components) comp->OnMessage(message);
    }

    template <class T>
    T* GetComponent() const {
        for (const auto& comp : m_components) {
            if (auto casted = dynamic_cast<T*>(comp.get())) {
                return casted;
            }
        }
        return nullptr;
    }



private:
    std::vector<std::unique_ptr<Component>> m_components;
protected:
    virtual void OnUpdate(){}
    virtual void OnDraw(){}
};