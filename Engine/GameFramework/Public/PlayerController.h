#pragma once
#include "GameObject.h"
#include "GameObject.h"
class APlayerController:public AGameObject
{
public :
	void Possess(AGameObject* NewPawn);

	void OnUpdate(float DeltaTime) override;
private:
	AGameObject* m_TargetPawn = nullptr;
};

