#include "NetworkTest/NetworkTestPawn.h"

#include "EnhancedInputComponent.h"
#include "Log.h"
#include "SpriteComponent.h"

#include <DxLib.h>

namespace
{
	enum : FNetworkRPCId
	{
		RPC_ServerTest = 1,
		RPC_MulticastTest = 2
	};
}

REGISTER_ACTOR(ANetworkTestPawn)

ANetworkTestPawn::ANetworkTestPawn()
{
	bReplicates = true;

	RegisterRPC(RPC_ServerTest, ENetRPCType::Server, this, &ANetworkTestPawn::Server_TestRPC);
	RegisterRPC(RPC_MulticastTest, ENetRPCType::Multicast, this, &ANetworkTestPawn::Multicast_TestRPC);

	auto sprite = std::make_unique<MSpriteComponent>(10, RenderSpace::World);
	BodySprite = sprite.get();
	BodySprite->SubmitBox(48.0f, 48.0f, GetDisplayColor(), true);
	BodySprite->SetRelativeLocation({ -24.0f, -24.0f });
	AddComponent(std::move(sprite));
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
	if (!CanProcessMovementInput()) {
		return;
	}

	AddActorWorldOffset(Value.Axis2D * (-MoveSpeed / 60.0f));
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

bool ANetworkTestPawn::CanProcessMovementInput() const
{
	return bHasAuthority && bIsLocallyControlled;
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
