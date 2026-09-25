#pragma once

#include "Pawn.h"
#include "UMath.h"

class AActor;
class EditorMode;
class MCamera3DComponent;
class MEnhancedInputComponent;
struct FInputActionValue;

class EditorPawn3D : public APawn {
 public:
  DEFINE_ACTOR_CLASS(EditorPawn3D);

  EditorPawn3D();
  ~EditorPawn3D() override;

  void OnUpdate(float DeltaTime) override;
  void OnPossessedBy(APlayerController* NewController) override;
  void OnUnPossessed() override;
  void SetupPlayerInputComponent(MEnhancedInputComponent* PlayerInputComponent) override;

  MCamera3DComponent* GetEditorCamera3D() const { return EditorCamera3D; }
  bool IsCameraNavigationActive() const;
  void EndCameraNavigation();
  void FocusActor3D(AActor* Actor);

 private:
  void BeginPlay() override;
  void OnMove(const FInputActionValue& Value) override;
  void OnVerticalMove(const FInputActionValue& Value);
  void OnMouseLeftPress(const FInputActionValue& Value);
  void OnMouseLeftRelease(const FInputActionValue& Value);
  void OnMouseRightPress(const FInputActionValue& Value);
  void OnMouseRightRelease(const FInputActionValue& Value);
  void OnMouseMove(const FInputActionValue& Value);
  void OnWheel(const FInputActionValue& Value);
  void UpdateCamera(float DeltaTime);

  EditorMode* EditorModePtr = nullptr;
  MCamera3DComponent* EditorCamera3D = nullptr;
  bool CameraNavigationActive = false;
  bool DiscardNextCameraDelta = false;
  FVector2D MovementInput = FVector2D::ZeroVector();
  float VerticalMovementInput = 0.0f;
  float CameraSpeedMultiplier = 1.0f;
  bool IsPossessed = false;
};
