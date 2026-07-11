#include "Pawn.h"

#include <DxLib.h>
#include <NetMovementComponent.h>
#include <EnhancedInputComponent.h>

#include "CameraComponent.h"
#include "InputManager.h"
#include "InputMapper.h"
#include "Log.h"
#include "ActorManager.h"
#include "PlayerController.h"
#include "ResourceManager.h"
#include "SpriteComponent.h"
#include "World.h"

REGISTER_ACTOR(APawn)

APawn::APawn() {
  Camera = NewObject<MCameraComponent>(this);
  if (Camera) {
    Camera->SetFOV(1);
    Camera->RegisterComponent();
  }
}
void APawn::OnPossessedBy(APlayerController* NewController) {
  Controller = NewController;

  if (Controller && Controller->GetPlayerId() == 0) {
    Camera->SetActiveCamera();
  }
}

void APawn::OnUnPossessed() { Controller = nullptr; }
void APawn::OnUpdate(float DeltaTime) {}

void APawn::SetupPlayerInputComponent(MEnhancedInputComponent* comp) {
  comp->BindAction(InputAction::Interact, ETriggerEvent::Started, this, &APawn::OnInteractPressed);
  comp->BindAction(InputActionLower::MoveX, ETriggerEvent::Triggered, this, &APawn::OnMove);
  comp->BindAction(InputActionLower::MoveY, ETriggerEvent::Triggered, this, &APawn::OnMove);
}

void APawn::OnInteractPressed() { M_LOG("FFFF"); }

void APawn::OnMove(const FInputActionValue& Value) {
  auto MovementComponents = GetComponents<MNetMovementComponent>();
  if (!MovementComponents.empty() && MovementComponents.front()) {
    MovementComponents.front()->AddMovementInput(Value.Axis2D);
  }
}
