#include "Pawn.h"
#include "InputMapper.h"
#include "InputManager.h"
#include "ResourceManager.h"
#include "SpriteComponent.h"
#include "CameraComponent.h"
#include <DxLib.h>
#include <EnhancedInputComponent.h>
#include "Log.h"
APawn::APawn()
{
	auto camera = std::make_unique<MCameraComponent>();
	Camera = camera.get();
	AddComponent(std::move(camera));
	Camera->SetFOV(1);
}
void APawn::OnPossesed()
{
	Camera->SetActiveCamera();
}
void APawn::OnUpdate(float DeltaTime)
{
}

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
	AddActorWorldOffset(Value.Axis2D*-10);
}
