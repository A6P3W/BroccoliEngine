#pragma once
#include "UpdateableObject.h"
#include "GameObject.h"
class APlayerController:public MUpdateableObject
{
public :
	void Possess(AGameObject* NewPawn);

	void OnUpdate(float DeltaTime) override;
private:
	AGameObject* m_TargetPawn = nullptr;
};

