#include "NetworkTest/NetworkTestPawn.h"

#include "EnhancedInputComponent.h"
#include "Log.h"
#include "SpriteComponent.h"
#include <MovementComponent.h>
#include <RectangleCollisionComponent.h>

#include <DxLib.h>

namespace
{
	enum : FNetworkRPCId
	{
		RPC_ServerTest = 1,
		RPC_MulticastTest = 2,
		RPC_ServerMove = 3
	};
}

REGISTER_ACTOR(ANetworkTestPawn)

ANetworkTestPawn::ANetworkTestPawn()
{
	bReplicates = true;

	RegisterRPC(RPC_ServerTest, ENetRPCType::Server, this, &ANetworkTestPawn::Server_TestRPC);
	RegisterRPC(RPC_MulticastTest, ENetRPCType::Multicast, this, &ANetworkTestPawn::Multicast_TestRPC);
	RegisterRPC(RPC_ServerMove, ENetRPCType::Server, this, &ANetworkTestPawn::Server_Move);

	auto sprite = std::make_unique<MSpriteComponent>(10, RenderSpace::World);
	BodySprite = sprite.get();
	BodySprite->SubmitBox(48.0f, 48.0f, GetDisplayColor(), true);
	BodySprite->SetRelativeLocation({ -24.0f, -24.0f });
	AddComponent(std::move(sprite));

	auto col = std::make_unique<MRectangleCollisionComponent>(48.0f, 48.0f);
	col->SetParentComponent(GetRootComponent());
	col->SetStatic(false);
	AddComponent(std::move(col));

	auto movement = std::make_unique<MMovementComponent>();
	Movement = movement.get();
	AddComponent(std::move(movement));
}

void ANetworkTestPawn::OnPossesed()
{
	APawn::OnPossesed();
}

void ANetworkTestPawn::OnUpdate(float DeltaTime)
{
	if (FlashTimer > 0.0f) {
		FlashTimer -= DeltaTime;
		if (FlashTimer < 0.0f) {
			FlashTimer = 0.0f;
		}
	}

	if (BodySprite) {
		BodySprite->SubmitBox(48.0f, 48.0f, GetDisplayColor(), true);
	}
}

void ANetworkTestPawn::SetupPlayerInputComponent(MEnhancedInputComponent* PlayerInputComponent)
{
	if (!PlayerInputComponent) {
		return;
	}

	PlayerInputComponent->BindAction(InputAction::Move, ETriggerEvent::Triggered, this, &ANetworkTestPawn::OnMove);
	PlayerInputComponent->BindAction(InputAction::Interact, ETriggerEvent::Started, this, &ANetworkTestPawn::OnInteract);
}

void ANetworkTestPawn::OnMove(const FInputActionValue& Value)
{
	if (!bIsLocallyControlled) {
		return;
	}

	if (bHasAuthority) {
		ApplyMovementInput(Value.Axis2D);
	}
	else {
		InvokeRPC(RPC_ServerMove, ENetRPCType::Server, ENetPacketReliability::Unreliable, Value.Axis2D);
	}
}

void ANetworkTestPawn::Server_Move(const FVector2D& MoveInput)
{
	ApplyMovementInput(MoveInput);
}

void ANetworkTestPawn::ApplyMovementInput(const FVector2D& MoveInput)
{
	if (Movement) {
		Movement->AddWorldForce(MoveInput * (-MoveSpeed / 60.0f) * 0.05f);
	}
}

void ANetworkTestPawn::OnInteract(const FInputActionValue& Value)
{
	(void)Value;

	if (!bIsLocallyControlled) {
		return;
	}

	const int playerId = static_cast<int>(OwnerConnectionId);
	M_LOG("RPC test input: actor={} player={}", NetworkId, playerId);
	InvokeRPC(RPC_ServerTest, ENetRPCType::Server, ENetPacketReliability::Reliable, playerId);
}

void ANetworkTestPawn::Server_TestRPC(int PlayerId)
{
	M_LOG("Server_TestRPC received: actor={} player={}", NetworkId, PlayerId);
	InvokeRPC(RPC_MulticastTest, ENetRPCType::Multicast, ENetPacketReliability::Reliable, PlayerId);
}

void ANetworkTestPawn::Multicast_TestRPC(int PlayerId)
{
	M_LOG("Multicast_TestRPC received: actor={} player={}", NetworkId, PlayerId);
	FlashTimer = 0.5f;
}

int ANetworkTestPawn::GetDisplayColor() const
{
	if (FlashTimer > 0.0f) {
		return GetColor(255, 255, 80);
	}

	return (bIsLocallyControlled || (bHasAuthority && OwnerConnectionId == 0))
		? GetColor(0, 220, 80)
		: GetColor(220, 60, 60);
}

void ANetworkTestPawn::BeginOverlap(AActor* OtherActor)
{
	AActor::BeginOverlap(OtherActor);

	if (auto otherPawn = dynamic_cast<ANetworkTestPawn*>(OtherActor)) {
		M_LOG("Player overlap detected! My NetworkId: {}, Other NetworkId: {}", NetworkId, otherPawn->NetworkId);
	}
}
