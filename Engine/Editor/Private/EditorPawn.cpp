#include "EditorPawn.h"

#include <algorithm>

#include "BroccoliRaylib.h"
#include "Camera3DComponent.h"
#include "CameraComponent.h"
#include "EditorMode.h"
#include "EnhancedInputComponent.h"
#include "RenderSystem.h"
#include "SceneManager.h"
#include "SpriteComponent.h"
#include "World.h"
REGISTER_ACTOR(EditorPawn);

namespace {
constexpr float MinEditorFOV = 0.01f;
constexpr float MaxEditorFOV = 1000.0f;
constexpr float MinCameraSpeedMultiplier = 0.1f;
constexpr float MaxCameraSpeedMultiplier = 15.0f;
constexpr float CameraSpeedMultiplierStep = 0.5f;
constexpr float ThreeDCameraBaseSpeedMultiplier = 10.0f;
}  // namespace

EditorPawn::EditorPawn() {
  SetActorLocation3D({0.0f, 5.0f, -8.0f});
  SetActorRotation3D(FQuaternion::FromRotator({25.0f, 0.0f, 0.0f}));
  EditorCamera3D = NewObject<MCamera3DComponent>(this);
  if (EditorCamera3D != nullptr) {
    EditorCamera3D->RegisterComponent();
  }
  GameScreenView = NewObject<MSpriteComponent>(this);
  if (GameScreenView) {
    GameScreenView->SetRenderSettings(999, RenderSpace::World);
    GameScreenView->SubmitBox(1920, 1080, FColor{255, 255, 255}, 0);
    GameScreenView->RegisterComponent();
  }
  bEditorActor = true;
}

EditorPawn::~EditorPawn() { EndCameraDrag(); }

void EditorPawn::SetEditorCamera3DActive() {
  if (EditorCamera3D != nullptr) EditorCamera3D->SetActiveCamera();
}

void EditorPawn::OnPossessedBy(APlayerController* NewController) {
  APawn::OnPossessedBy(NewController);
}

void EditorPawn::SetupPlayerInputComponent(MEnhancedInputComponent* PlayerInputComponent) {
  PlayerInputComponent->BindAction(
      InputAction::Move, ETriggerEvent::Triggered, this, &EditorPawn::OnMove
  );
  PlayerInputComponent->BindAction(
      InputAction::Move, ETriggerEvent::Completed, this, &EditorPawn::OnMove
  );
  PlayerInputComponent->BindAction(
      EditorInputAction::MoveVertical, ETriggerEvent::Triggered, this, &EditorPawn::OnVerticalMove
  );
  PlayerInputComponent->BindAction(
      EditorInputAction::MoveVertical, ETriggerEvent::Completed, this, &EditorPawn::OnVerticalMove
  );
  PlayerInputComponent->BindAction(
      InputActionMouse::MouseLeft, ETriggerEvent::Started, this, &EditorPawn::OnMouseLeftPress
  );
  PlayerInputComponent->BindAction(
      InputActionMouse::MouseLeft, ETriggerEvent::Completed, this, &EditorPawn::OnMouseLeftRelease
  );

  PlayerInputComponent->BindAction(
      InputActionMouse::MouseRight, ETriggerEvent::Started, this, &EditorPawn::OnMouseRightPress
  );
  PlayerInputComponent->BindAction(
      InputActionMouse::MouseRight, ETriggerEvent::Completed, this, &EditorPawn::OnMouseRightRelease
  );

  PlayerInputComponent->BindAction(
      InputAction::Look, ETriggerEvent::Triggered, this, &EditorPawn::OnMouseMove
  );
  PlayerInputComponent->BindAction(
      InputActionMouse::Wheel, ETriggerEvent::Triggered, this, &EditorPawn::OnWheel
  );
}

void EditorPawn::OnUpdate(float DeltaTime) {
  if (EditorModePtr != nullptr && GameScreenView != nullptr) {
    GameScreenView->SetVisibility(EditorModePtr->GetViewportMode() == EEditorViewportMode::TwoD);
  }
  if (CameraDragActive && (!IsWindowFocused() || EditorModePtr == nullptr || Camera == nullptr ||
                           EditorModePtr->GetViewportMode() == EEditorViewportMode::ThreeD ||
                           !EditorModePtr->GetViewportState().HasValidImage() ||
                           !IsMouseButtonDown(MOUSE_BUTTON_RIGHT))) {
    EndCameraDrag();
  }
  UpdateThreeDCamera(DeltaTime);
  UpdateCameraDrag();
}
void EditorPawn::BeginPlay() {
  EditorModePtr = dynamic_cast<EditorMode*>(GetWorld()->GetGameMode());
  if (EditorModePtr != nullptr) EditorModePtr->SetEditorPawn(this);
}

void EditorPawn::FocusActor3D(AActor* Actor) {
  if (Actor == nullptr || Actor->IsPendingDestroy()) return;

  const FVector3D Target = Actor->GetActorLocation3D();
  const FVector3D Direction = FVector3D{0.0f, 0.45f, -1.0f}.Normalize();
  SetActorLocation3D(Target - Direction * 10.0f);

  const FVector3D Forward = (Target - GetActorLocation3D()).Normalize();
  const FRotator3D Rotation{
      UMath::RadToDeg(std::asin((std::clamp)(Forward.Z, -1.0f, 1.0f))),
      UMath::RadToDeg(std::atan2(-Forward.X, Forward.Y)),
      0.0f,
  };
  SetActorRotation3D(FQuaternion::FromRotator(Rotation));
}

bool EditorPawn::IsThreeDCameraNavigationActive() const { return ThreeDCameraNavigationActive; }

void EditorPawn::EndThreeDCameraNavigation() {
  if (ThreeDCameraNavigationActive) EnableCursor();
  ThreeDCameraNavigationActive = false;
  DiscardNextThreeDCameraDelta = false;
  ThreeDMovementInput = FVector2D::ZeroVector();
  ThreeDVerticalMovementInput = 0.0f;
}

void EditorPawn::UpdateThreeDCamera(float DeltaTime) {
  if (EditorModePtr == nullptr || EditorModePtr->GetViewportMode() != EEditorViewportMode::ThreeD ||
      EditorCamera3D == nullptr) {
    EndThreeDCameraNavigation();
    return;
  }
  if (!IsWindowFocused()) {
    EndThreeDCameraNavigation();
    return;
  }

  EditorCamera3D->SetActiveCamera();
  if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT) && EditorModePtr->IsViewportInputAvailable()) {
    ThreeDCameraNavigationActive = true;
    DiscardNextThreeDCameraDelta = true;
    DisableCursor();
  }
  if (!IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) EndThreeDCameraNavigation();

  if (EditorModePtr->IsViewportInputAvailable() && IsKeyPressed(KEY_F)) {
    FocusActor3D(EditorModePtr->GetSelectedActor());
  }
  if (!ThreeDCameraNavigationActive) return;

  const float SpeedMultiplier =
      std::clamp(CameraSpeedMultiplier, MinCameraSpeedMultiplier, MaxCameraSpeedMultiplier);
  const float Speed = (IsKeyDown(KEY_LEFT_SHIFT) ? 20.0f : 8.0f) * ThreeDCameraBaseSpeedMultiplier *
                      SpeedMultiplier;
  FVector3D Movement = EditorCamera3D->GetForwardVector() * ThreeDMovementInput.Y;
  Movement += EditorCamera3D->GetRightVector() * -ThreeDMovementInput.X;
  Movement.Y += ThreeDVerticalMovementInput;
  if (Movement.SizeSquared() > 0.0f) {
    SetActorLocation3D(GetActorLocation3D() + Movement.Normalize() * Speed * DeltaTime);
  }

  const Vector2 MouseDelta = GetMouseDelta();
  if (DiscardNextThreeDCameraDelta) {
    DiscardNextThreeDCameraDelta = false;
    return;
  }
  const FVector2D ViewportDelta =
      EditorModePtr->GetViewportState().ScreenDeltaToRenderTarget({MouseDelta.x, MouseDelta.y});
  if (ViewportDelta.SizeSquared() <= 0.0001f) return;

  constexpr float CameraLookSensitivity = 0.03f;
  const FQuaternion LocalPitch =
      FQuaternion::FromRotator({ViewportDelta.Y * CameraLookSensitivity, 0.0f, 0.0f});
  const FQuaternion WorldYaw =
      FQuaternion::FromRotator({0.0f, -ViewportDelta.X * CameraLookSensitivity, 0.0f});
  SetActorRotation3D((WorldYaw * GetActorRotation3D() * LocalPitch).Normalize());
}
void EditorPawn::OnMove(const FInputActionValue& Value) {
  if (EditorModePtr == nullptr || EditorModePtr->GetViewportMode() != EEditorViewportMode::ThreeD ||
      !ThreeDCameraNavigationActive) {
    return;
  }
  ThreeDMovementInput = Value.Axis2D;
}

void EditorPawn::OnVerticalMove(const FInputActionValue& Value) {
  if (EditorModePtr == nullptr || EditorModePtr->GetViewportMode() != EEditorViewportMode::ThreeD ||
      !ThreeDCameraNavigationActive) {
    return;
  }
  ThreeDVerticalMovementInput = Value.Axis1D;
}
void EditorPawn::OnMouseLeftPress(const FInputActionValue&) {
  if (EditorModePtr != nullptr && EditorModePtr->GetViewportMode() == EEditorViewportMode::ThreeD) {
    return;
  }
  if (EditorModePtr == nullptr || !EditorModePtr->IsViewportInputAvailable()) return;

  FVector2D MouseRenderTargetPosition;
  if (EditorModePtr->TryGetViewportRenderTargetMousePosition(MouseRenderTargetPosition)) {
    EditorModePtr->OnMousePress(
        RenderSystem::GetInstance().ScreenToWorld(MouseRenderTargetPosition)
    );
  }
}

void EditorPawn::OnMouseLeftRelease(const FInputActionValue&) {
  if (EditorModePtr != nullptr && EditorModePtr->GetViewportMode() == EEditorViewportMode::ThreeD) {
    return;
  }
  if (EditorModePtr == nullptr || EditorModePtr->GetState() != EEditorState::Dragging) return;

  FVector2D MouseRenderTargetPosition;
  if (EditorModePtr->TryGetViewportRenderTargetMousePosition(MouseRenderTargetPosition, false)) {
    EditorModePtr->OnMouseRelease(
        RenderSystem::GetInstance().ScreenToWorld(MouseRenderTargetPosition)
    );
  } else {
    EditorModePtr->OnMouseRelease(FVector2D::ZeroVector());
  }
}

void EditorPawn::OnMouseRightPress(const FInputActionValue&) {
  if (EditorModePtr != nullptr && EditorModePtr->GetViewportMode() == EEditorViewportMode::ThreeD) {
    return;
  }
  BeginCameraDrag();
}

void EditorPawn::BeginCameraDrag() {
  if (CameraDragActive || EditorModePtr == nullptr || Camera == nullptr ||
      EditorModePtr->GetState() == EEditorState::Dragging ||
      !EditorModePtr->IsViewportInputAvailable()) {
    return;
  }

  CameraDragActive = true;
  DiscardNextCameraDelta = true;
  DisableCursor();
}

void EditorPawn::EndCameraDrag() {
  if (!CameraDragActive) return;

  CameraDragActive = false;
  DiscardNextCameraDelta = false;
  EnableCursor();
}

void EditorPawn::OnMouseRightRelease(const FInputActionValue&) {
  if (EditorModePtr != nullptr && EditorModePtr->GetViewportMode() == EEditorViewportMode::ThreeD) {
    EndThreeDCameraNavigation();
    return;
  }
  EndCameraDrag();
}

void EditorPawn::UpdateCameraDrag() {
  if (!CameraDragActive || EditorModePtr == nullptr || Camera == nullptr) return;

  const Vector2 MouseDelta = GetMouseDelta();
  if (DiscardNextCameraDelta) {
    DiscardNextCameraDelta = false;
    return;
  }
  const FVector2D RenderTargetDelta =
      EditorModePtr->GetViewportState().ScreenDeltaToRenderTarget({MouseDelta.x, MouseDelta.y});
  if (RenderTargetDelta.SizeSquared() <= 0.0001f) return;

  const float FieldOfView = std::clamp(Camera->GetFOV(), MinEditorFOV, MaxEditorFOV);
  FVector2D WorldDelta = {RenderTargetDelta.X, -RenderTargetDelta.Y};
  const float SpeedMultiplier =
      std::clamp(CameraSpeedMultiplier, MinCameraSpeedMultiplier, MaxCameraSpeedMultiplier);
  WorldDelta *= SpeedMultiplier / FieldOfView;
  WorldDelta = WorldDelta.RotateVector(GetActorRotation());
  AddActorWorldOffset(WorldDelta * -1.0f);
}

void EditorPawn::OnMouseMove(const FInputActionValue&) {
  if (EditorModePtr != nullptr && EditorModePtr->GetViewportMode() == EEditorViewportMode::ThreeD) {
    return;
  }
  if (EditorModePtr == nullptr || CameraDragActive) return;

  const Vector2 MouseDelta = GetMouseDelta();
  const FVector2D RenderTargetDelta =
      EditorModePtr->GetViewportState().ScreenDeltaToRenderTarget({MouseDelta.x, MouseDelta.y});
  if (EditorModePtr->GetState() == EEditorState::Dragging) {
    EditorModePtr->OnMouseMove(RenderTargetDelta);
  }
}

void EditorPawn::OnWheel(const FInputActionValue& Value) {
  if (EditorModePtr == nullptr) return;

  if (CameraDragActive || ThreeDCameraNavigationActive) {
    CameraSpeedMultiplier = std::clamp(
        CameraSpeedMultiplier + Value.Axis1D * CameraSpeedMultiplierStep,
        MinCameraSpeedMultiplier,
        MaxCameraSpeedMultiplier
    );
    return;
  }

  if (EditorModePtr->GetViewportMode() == EEditorViewportMode::ThreeD) {
    return;
  }
  if (!EditorModePtr->IsViewportInputAvailable() || Camera == nullptr) {
    return;
  }

  const float ZoomAmount = Value.Axis1D * -0.1f;
  const float NewFieldOfView = Camera->GetFOV() * (1.0f - ZoomAmount);
  Camera->SetFOV(std::clamp(NewFieldOfView, MinEditorFOV, MaxEditorFOV));
}
