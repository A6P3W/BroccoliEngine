#pragma once

#include "Pawn.h"

class MEnhancedInputComponent;
class MMovementComponent;
struct FInputActionValue;

// Minimal playable pawn: an Enhanced Input Move action drives MovementComponent.
class ABasicGameplayPawn : public APawn {
 public:
  DEFINE_ACTOR_CLASS(ABasicGameplayPawn)

  ABasicGameplayPawn();
  void BeginPlay() override;
  void OnUpdate(float DeltaTime) override;
  void SetupPlayerInputComponent(MEnhancedInputComponent* PlayerInputComponent) override;

 private:
  void Move(const FInputActionValue& Value);
  MMovementComponent* Movement = nullptr;
};
