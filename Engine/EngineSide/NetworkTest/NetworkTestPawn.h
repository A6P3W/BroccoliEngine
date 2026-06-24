#pragma once

#include "Pawn.h"
#include "UMath.h"

class MEnhancedInputComponent;
class MSpriteComponent;
struct FInputActionValue;
class MMovementComponent;

class ANetworkTestPawn : public APawn
{
public:
	DEFINE_ACTOR_CLASS(ANetworkTestPawn)
	ANetworkTestPawn();

	void OnPossesed() override;
	void OnUpdate(float DeltaTime) override;
	void SetupPlayerInputComponent(MEnhancedInputComponent* PlayerInputComponent) override;

private:
	void OnMove(const FInputActionValue& Value);
	void OnInteract(const FInputActionValue& Value);
	void Server_TestRPC(int PlayerId);
	void Multicast_TestRPC(int PlayerId);
	void Server_Move(const FVector2D& MoveInput);
	void ApplyMovementInput(const FVector2D& MoveInput);
	void BeginOverlap(AActor* OtherActor) override;
	int GetDisplayColor() const;

	MSpriteComponent* BodySprite = nullptr;
	MMovementComponent* Movement = nullptr;
	float MoveSpeed = 320.0f;
	float FlashTimer = 0.0f;
};

