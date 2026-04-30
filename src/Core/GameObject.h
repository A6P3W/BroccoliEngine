#pragma once
#include "UpdateableObject.h"
#include <string>
#include <vector>
#include "Components/TransformComponent.h"
class MTransformComponent;
class AGameObject :
    public MUpdateableObject
{
public:
    AGameObject();
    virtual void OnUpdate(float DeltaTime) override;
    virtual void Draw()final;
    MTransformComponent* GetTransform() const;

    template<class T>
    T* GetAllGameObjectsOfClass() const {

    }
protected:
    virtual void OnDraw() {}
    MTransformComponent* m_transform = nullptr;
private:
	std::vector<AGameObject*> m_childObjects;
};

