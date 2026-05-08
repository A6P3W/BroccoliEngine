#pragma once
#include "Actor.h"
#include "Actor.h"
class APlayerController:public AActor
{
public :
	void Possess(AActor* NewPawn);

	void OnUpdate(float DeltaTime) override;
private:
	AActor* m_TargetPawn = nullptr;
};

