#pragma once
#include "Actor.h"
#include "Pawn.h"
#include <InputMapper.h>
#include <memory>

enum class EInputMode
{
	GameOnly,
	UIOnly,
	GameAndUI
};
class MEnhancedInputComponent;
class APlayerController :public AActor
{
public:
	DEFINE_ACTOR_CLASS(APlayerController)
		APlayerController();
	virtual void Possess(APawn* NewPawn);

	void OnUpdate(float DeltaTime) override;
	void SetPlayerId(int id);
	virtual void SetupPlayerInputComponent(MEnhancedInputComponent* PlayerInputComponent);
	virtual void SetupInputMappings();
	InputMapper* GetInputMapper() { return InputMapperPtr.get(); }
	MEnhancedInputComponent* GetInputComponent() { return InputComponent; }

	void SetInputMode(EInputMode NewInputMode) { InputMode = NewInputMode; }
	EInputMode GetInputMode() const { return InputMode; }

private:
	APawn* TargetPawn = nullptr;
	MEnhancedInputComponent* InputComponent;
	std::unique_ptr<InputMapper> InputMapperPtr;
	int PlayerId = 0;
	EInputMode InputMode = EInputMode::GameAndUI;
};

