#pragma once
#include "Actor.h"
#include "Pawn.h"
class APlayerController:public AActor
{
public :
	APlayerController();
	virtual void Possess(APawn* NewPawn);

	void OnUpdate(float DeltaTime) override;
private:
	APawn* m_TargetPawn = nullptr;
};

