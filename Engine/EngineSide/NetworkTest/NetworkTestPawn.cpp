#include "NetworkTest/NetworkTestPawn.h"

#include "EnhancedInputComponent.h"
#include "SpriteComponent.h"

#include <DxLib.h>

REGISTER_ACTOR(ANetworkTestPawn)

ANetworkTestPawn::ANetworkTestPawn()
{
	bReplicates = true;

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
}

void ANetworkTestPawn::OnMove(const FInputActionValue& Value)
{
	if (!CanProcessMovementInput()) {
		return;
	}

	AddActorWorldOffset(Value.Axis2D * (-MoveSpeed / 60.0f));
}

bool ANetworkTestPawn::CanProcessMovementInput() const
{
	return bHasAuthority && bIsLocallyControlled;
}

int ANetworkTestPawn::GetDisplayColor() const
{
	return (bIsLocallyControlled || (bHasAuthority && OwnerConnectionId == 0))
		? GetColor(0, 220, 80)
		: GetColor(220, 60, 60);
}
