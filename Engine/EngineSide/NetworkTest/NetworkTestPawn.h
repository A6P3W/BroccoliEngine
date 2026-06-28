#pragma once

#include "ActorComponent.h"
#include "Pawn.h"
#include "UMath.h"

class MEnhancedInputComponent;
class MSpriteComponent;
struct FInputActionValue;
class MMovementComponent;

class MNetworkTestRepComponent : public MActorComponent
{
public:
	MNetworkTestRepComponent();

	void RequestTest(int PlayerId);
	int GetReplicatedCounter() const { return ReplicatedCounter; }
	bool IsOnRepFlashActive() const { return OnRepFlashTimer > 0.0f; }
	bool IsRPCFlashActive() const { return RPCFlashTimer > 0.0f; }

protected:
	void OnUpdate(float DeltaTime) override;

private:
	void Server_ComponentTest(int PlayerId);
	void Multicast_ComponentTest(int PlayerId, int CounterValue);
	void OnRepReplicatedCounter(int OldValue);

	int ReplicatedCounter = 0;
	float OnRepFlashTimer = 0.0f;
	float RPCFlashTimer = 0.0f;
	float PendingOnRepFlashTimer = 0.0f;
};

class ANetworkTestPawn : public APawn
{
public:
	DEFINE_ACTOR_CLASS(ANetworkTestPawn)
	ANetworkTestPawn();

	void OnPossessedBy(APlayerController* NewController) override;
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
	MNetworkTestRepComponent* ReplicationTest = nullptr;
	float MoveSpeed = 320.0f;
	float FlashTimer = 0.0f;
};
