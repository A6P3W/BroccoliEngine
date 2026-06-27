#include "Pawn.h"
#include "InputMapper.h"
#include "InputManager.h"
#include "ResourceManager.h"
#include "SpriteComponent.h"
#include "CameraComponent.h"
#include <DxLib.h>
#include <EnhancedInputComponent.h>
#include "Log.h"
#include "PlayerController.h"
#include "World.h"
#include "ObjectManager.h"

REGISTER_ACTOR(APawn)

APawn::APawn()
{
	auto camera = std::make_unique<MCameraComponent>();
	Camera = camera.get();
	AddComponent(std::move(camera));
	Camera->SetFOV(1);
}
void APawn::OnPossesed()
{
	APlayerController* owningController = nullptr;
	if (GetWorld() && GetWorld()->GetObjectManager()) {
		const auto& actors = GetWorld()->GetObjectManager()->GetAllActors();
		for (const auto& actorPtr : actors) {
			if (auto controller = dynamic_cast<APlayerController*>(actorPtr.get())) {
				if (controller->GetPawn() == this) {
					owningController = controller;
					break;
				}
			}
		}
	}

	if (owningController && owningController->GetPlayerId() == 0) {
		Camera->SetActiveCamera();
	}
}
void APawn::OnUpdate(float DeltaTime)
{}

void APawn::SetupPlayerInputComponent(MEnhancedInputComponent* comp) {
	comp->BindAction(InputAction::Interact, ETriggerEvent::Started, this, &APawn::OnInteractPressed);
	comp->BindAction(InputActionLower::MoveX, ETriggerEvent::Triggered, this, &APawn::OnMove);
	comp->BindAction(InputActionLower::MoveY, ETriggerEvent::Triggered, this, &APawn::OnMove);
}

void APawn::OnInteractPressed()
{
	M_LOG("FFFF");
}

void APawn::OnMove(const FInputActionValue& Value)
{
	AddActorWorldOffset(Value.Axis2D * -10);
}
