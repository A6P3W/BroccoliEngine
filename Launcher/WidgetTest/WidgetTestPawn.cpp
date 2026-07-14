#include "WidgetTestPawn.h"

#include <CircleCollisionComponent.h>
#include <DxLib.h>
#include <EnhancedInputComponent.h>
#include <MovementComponent.h>

#include "CameraComponent.h"
#include "InputManager.h"
#include "InputMapper.h"
#include "Log.h"
#include "ResourceManager.h"
#include "SpriteComponent.h"
#include "World.h"

REGISTER_ACTOR(AWidgetTestPawn)

AWidgetTestPawn::AWidgetTestPawn() {
  auto* col = NewObject<MCircleCollisionComponent>(this);
  col->SetRadius(32.0f);
  col->AttachToComponent(GetRootComponent());
  col->RegisterComponent();

  Movement = NewObject<MMovementComponent>(this);
  Movement->RegisterComponent();
}
void AWidgetTestPawn::BeginPlay() {}
void AWidgetTestPawn::OnPossessedBy(APlayerController* NewController) {
  APawn::OnPossessedBy(NewController);
  Camera->SetFOV(1);
}
void AWidgetTestPawn::OnUpdate(float DeltaTime) {
  const FVector2D PawnPosition = GetActorLocation();
  const FVector2D WorldLogPosition = PawnPosition + FVector2D(0.0f, -60.0f);

  DRAW_SCREEN_LOG(
      "WidgetTestPawnPosition",
      0.1f,
      "WidgetTest Pawn Position: X={:.1f}, Y={:.1f}",
      PawnPosition.X,
      PawnPosition.Y
  );
  DRAW_WORLD_LOG(
      "WidgetTestPawnWorldLog",
      0.1f,
      WorldLogPosition,
      "WidgetTest Pawn"
  );
}

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
