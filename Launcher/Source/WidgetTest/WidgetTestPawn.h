#pragma once
#include "Pawn.h"
#include "UMath.h"

class MEnhancedInputComponent;
struct FInputActionValue;
class MMovementComponent;

class AWidgetTestPawn : public APawn {
 public:
  DEFINE_ACTOR_CLASS(AWidgetTestPawn)
  AWidgetTestPawn();
  void BeginPlay() override;
  void OnPossessedBy(APlayerController* NewController) override;
  void OnUpdate(float DeltaTime) override;
  void SetupPlayerInputComponent(MEnhancedInputComponent* PlayerInputComponent) override;

 private:
  void OnInteractPressed();
  void OnMove(const FInputActionValue& Value);
  MMovementComponent* Movement = nullptr;
};
