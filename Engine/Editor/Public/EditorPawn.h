#pragma once
#include "Pawn.h"
#include "UMath.h"

class MEnhancedInputComponent;
struct FInputActionValue;
class MMovementComponent;
class EditorMode;
class MSpriteComponent;
class EditorPawn : public APawn {
 public:
  DEFINE_ACTOR_CLASS(EditorPawn);
  EditorPawn();
  ~EditorPawn() override;

  void OnUpdate(float DeltaTime) override;
  void OnPossessedBy(APlayerController* NewController) override;
  void SetupPlayerInputComponent(MEnhancedInputComponent* PlayerInputComponent) override;

 private:
  void BeginPlay() override;

  void OnMove(const FInputActionValue& Value);

  void OnMouseLeftPress(const FInputActionValue& Value);
  void OnMouseLeftRelease(const FInputActionValue& Value);

  void OnMouseRightPress(const FInputActionValue& Value);
  void OnMouseRightRelease(const FInputActionValue& Value);
  void OnMouseMove(const FInputActionValue& Value);
  void OnWheel(const FInputActionValue& Value);

  void BeginCameraDrag();
  void EndCameraDrag();
  void UpdateCameraDrag();

  EditorMode* EditorModePtr = nullptr;

  bool CameraDragActive = false;
  bool DiscardNextCameraDelta = false;
  MSpriteComponent* GameScreenView;
};
