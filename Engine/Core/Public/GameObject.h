#pragma once
#include "UpdateableObject.h"
#include <string>
#include <vector>
#include "Components/Public/SceneComponent.h"
#include "Utils/Umath.h"
class MSceneComponent;
class AGameObject :
	public MUpdateableObject
{
public:
	AGameObject();
	virtual void OnUpdate(float DeltaTime) override;
	virtual void Draw()final;
	MSceneComponent* GetRootComponent() const { return m_rootComponent; };


	FVector2D GetActorLocation() const;
	bool SetActorLocation(const FVector2D& NewLocation);

	FRotator GetActorRotation() const;
	bool SetActorRotation(const FRotator& NewRotation);

	FScale GetActorScale() const;
	bool SetActorScale(float NewScale);


	void AddActorWorldOffset(const FVector2D& Offset);
	void AddActorLocalOffset(const FVector2D& Offset);
	void AddActorRotation(const FRotator& DeltaRotation);

	void Destroy();
	bool IsPendingDestroy() const;

	template<class T>
	T* GetAllGameObjectsOfClass() const {

	}
protected:
	virtual void OnDraw() {}

	MSceneComponent* m_rootComponent = nullptr;
	void SetRootComponent(MSceneComponent* Component);
private:
	std::vector<AGameObject*> m_childObjects;
	bool m_PendingDestroy = false;
};

