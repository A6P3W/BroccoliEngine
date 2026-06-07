#pragma once
#include "Actor.h"
#include "Pawn.h"
#include <InputMapper.h>
#include <memory>

class MEnhancedInputComponent;
class APlayerController:public AActor
{
public :
	DEFINE_ACTOR_CLASS(APlayerController)
	APlayerController();
	virtual void Possess(APawn* NewPawn);

	void OnUpdate(float DeltaTime) override;
	void SetPlayerId(int id);
	virtual void SetupPlayerInputComponent(MEnhancedInputComponent* PlayerInputComponent) {}
	InputMapper* GetInputMapper() { return m_InputMapper.get(); }
	MEnhancedInputComponent* GetInputComponent() { return m_InputCompPtr; }
private:
	APawn* m_TargetPawn = nullptr;
	MEnhancedInputComponent* m_InputCompPtr;
	std::unique_ptr<InputMapper> m_InputMapper;
	int m_PlayerId = 0;
};

