#include "BasicGameplayPawn.h"

#include "CircleCollisionComponent.h"
#include "EnhancedInputComponent.h"
#include "Log.h"
#include "MovementComponent.h"
#include "SpriteComponent.h"

REGISTER_ACTOR(ABasicGameplayPawn)

ABasicGameplayPawn::ABasicGameplayPawn() {
  auto* Body = NewObject<MSpriteComponent>(this);
  Body->SetRenderSettings(10, RenderSpace::World);
  Body->SubmitCircle(24.0f, FColor{80, 190, 255}, true);
  Body->AttachToComponent(GetRootComponent());
  Body->RegisterComponent();

  auto* Collision = NewObject<MCircleCollisionComponent>(this);
  Collision->SetRadius(24.0f);
  Collision->SetStatic(false);
  Collision->AttachToComponent(GetRootComponent());
  Collision->RegisterComponent();

  Movement = NewObject<MMovementComponent>(this);
  Movement->SetFriction(0.90f);
  Movement->RegisterComponent();
}

void ABasicGameplayPawn::BeginPlay() {
  APawn::BeginPlay();
  M_LOG("BasicGameplayPawn: WASD moves the pawn; the pawn camera follows automatically.");
}

void ABasicGameplayPawn::OnUpdate(float DeltaTime) {
  APawn::OnUpdate(DeltaTime);
  const FVector2D Position = GetActorLocation();
  DRAW_SCREEN_LOG("BasicGameplayHelp", 0.1f, "BasicGameplay: WASD move / Escape LevelStarter");
  DRAW_WORLD_LOG(
      "BasicGameplayPosition", 0.1f, this, "Pawn ({:.0f}, {:.0f})", Position.X, Position.Y
  );
}

void ABasicGameplayPawn::SetupPlayerInputComponent(MEnhancedInputComponent* PlayerInputComponent) {
  if (PlayerInputComponent) {
    PlayerInputComponent->BindAction(
        InputAction::Move, ETriggerEvent::Triggered, this, &ABasicGameplayPawn::Move
    );
  }
}

void ABasicGameplayPawn::Move(const FInputActionValue& Value) {
  if (Movement) {
    Movement->AddWorldForce(Value.Axis2D * -2.0f);
  }
}
