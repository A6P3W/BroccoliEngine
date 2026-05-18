#pragma once
#include "Actor.h"
#include "Pawn.h"
class MEnhancedInputComponent;
class APlayerController:public AActor
{
public :
	APlayerController();
	virtual void Possess(APawn* NewPawn);

	void OnUpdate(float DeltaTime) override;

	MEnhancedInputComponent* GetInputComponent() { return m_InputCompPtr; }
private:
	APawn* m_TargetPawn = nullptr;
	MEnhancedInputComponent* m_InputCompPtr;
};

