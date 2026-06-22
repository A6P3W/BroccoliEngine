#pragma once
#include "Pawn.h"
#include "UMath.h"

class MEnhancedInputComponent;
struct FInputActionValue;
class MMovementComponent;


class ASamplePawn01 : public APawn
{
public:
	DEFINE_ACTOR_CLASS(ASamplePawn01)
	ASamplePawn01();
	void BeginPlay() override;
	void OnPossesed() override;
    void OnUpdate(float DeltaTime) override;
	void SetupPlayerInputComponent(MEnhancedInputComponent* PlayerInputComponent) override;

private:
	void OnInteractPressed();
	void OnMove(const FInputActionValue& Value);
	MMovementComponent* Movement = nullptr;
};

