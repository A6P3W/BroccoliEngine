#pragma once

#include "Pawn.h"

class EditorMode;
class MEnhancedInputComponent;
class MSpriteComponent;
struct FInputActionValue;

class EditorPawn2D : public APawn {
 public:
  DEFINE_ACTOR_CLASS(EditorPawn2D);

  EditorPawn2D();
  ~EditorPawn2D() override;

  void OnUpdate(float DeltaTime) override;
  void OnPossessedBy(APlayerController* NewController) override;
  void OnUnPossessed() override;
  void SetupPlayerInputComponent(MEnhancedInputComponent* PlayerInputComponent) override;

 private:
  void BeginPlay() override;
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
  MSpriteComponent* GameScreenView = nullptr;
  bool CameraDragActive = false;
  bool DiscardNextCameraDelta = false;
  bool IsPossessed = false;
};
