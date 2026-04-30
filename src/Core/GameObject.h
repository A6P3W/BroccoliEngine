#pragma once
#include "UpdateableObject.h"
#include <string>
#include <vector>
#include "Components/SceneComponent.h"
class MSceneComponent;
class AGameObject :
    public MUpdateableObject
{
public:
    AGameObject();
    virtual void OnUpdate(float DeltaTime) override;
    virtual void Draw()final;
    MSceneComponent* GetTransform() const;

    template<class T>
    T* GetAllGameObjectsOfClass() const {

    }
protected:
    virtual void OnDraw() {}
    MSceneComponent* m_transform = nullptr;
private:
	std::vector<AGameObject*> m_childObjects;
};

