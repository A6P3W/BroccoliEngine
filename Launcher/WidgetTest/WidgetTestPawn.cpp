#include "WidgetTestPawn.h"

#include <CircleCollisionComponent.h>
#include <DxLib.h>
#include <EnhancedInputComponent.h>
#include <MovementComponent.h>

#include "CameraComponent.h"
#include "InputManager.h"
#include "InputMapper.h"
#include "Log.h"
#include "WidgetTestUIMain.h"
#include "ResourceManager.h"
#include "SpriteComponent.h"
#include "UIManager.h"
#include "World.h"

REGISTER_ACTOR(AWidgetTestPawn)

AWidgetTestPawn::AWidgetTestPawn() {
  auto col = std::make_unique<MCircleCollisionComponent>(32.0f);
  col->SetParentComponent(GetRootComponent());
  AddComponent(std::move(col));

  auto movement = std::make_unique<MMovementComponent>();
  Movement = movement.get();
  AddComponent(std::move(movement));
}
void AWidgetTestPawn::BeginPlay() {
  auto* mainMenuWidget = GetWorld()->GetObjectManager()->SpawnObject<AWidgetTestUIMain>();

  UIManager::GetInstance()->AddWidget(mainMenuWidget);
  UIManager::GetInstance()->SetFocusedWidget(mainMenuWidget);
}
void AWidgetTestPawn::OnPossessedBy(APlayerController* NewController) {
  APawn::OnPossessedBy(NewController);
  Camera->SetFOV(1);
}
void AWidgetTestPawn::OnUpdate(float DeltaTime) {}

void AWidgetTestPawn::SetupPlayerInputComponent(MEnhancedInputComponent* PlayerInputComponent) {
  PlayerInputComponent->BindAction(
      InputAction::Interact, ETriggerEvent::Started, this, &AWidgetTestPawn::OnInteractPressed
  );
  PlayerInputComponent->BindAction(
      InputAction::Move, ETriggerEvent::Triggered, this, &AWidgetTestPawn::OnMove
  );
}

void AWidgetTestPawn::OnInteractPressed() { M_LOG("F"); }

void AWidgetTestPawn::OnMove(const FInputActionValue& Value) {
  Movement->AddWorldForce(Value.Axis2D * -2);
}
