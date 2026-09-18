#pragma once
#include "Pawn.h"
#include "UMath.h"

class MEnhancedInputComponent;
struct FInputActionValue;
class MMovementComponent;
class EditorMode;
class MCamera3DComponent;
class AActor;
class MSpriteComponent;
class EditorPawn : public APawn {
 public:
  DEFINE_ACTOR_CLASS(EditorPawn);
  EditorPawn();
  ~EditorPawn() override;

  void OnUpdate(float DeltaTime) override;
  void OnPossessedBy(APlayerController* NewController) override;
  void SetupPlayerInputComponent(MEnhancedInputComponent* PlayerInputComponent) override;
  MCamera3DComponent* GetEditorCamera3D() const { return EditorCamera3D; }
  void SetEditorCamera3DActive();
  void FocusActor3D(AActor* Actor);
  bool IsThreeDCameraNavigationActive() const;
  void EndThreeDCameraNavigation();
  float GetCameraSpeedMultiplier() const { return CameraSpeedMultiplier; }

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

  void BeginCameraDrag();
  void EndCameraDrag();
  void UpdateCameraDrag();
  void UpdateThreeDCamera(float DeltaTime);

  EditorMode* EditorModePtr = nullptr;

  bool CameraDragActive = false;
  bool DiscardNextCameraDelta = false;
  bool ThreeDCameraNavigationActive = false;
  bool DiscardNextThreeDCameraDelta = false;
  FVector2D ThreeDMovementInput = FVector2D::ZeroVector();
  float ThreeDVerticalMovementInput = 0.0f;
  float CameraSpeedMultiplier = 1.0f;
  MCamera3DComponent* EditorCamera3D = nullptr;
  MSpriteComponent* GameScreenView;
};
