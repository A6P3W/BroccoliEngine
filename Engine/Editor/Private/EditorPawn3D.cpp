#include "EditorPawn3D.h"

#include <algorithm>

#include "BroccoliRaylib.h"
#include "Camera3DComponent.h"
#include "EditorMode.h"
#include "EnhancedInputComponent.h"
#include "World.h"

REGISTER_ACTOR(EditorPawn3D);

namespace {
constexpr float MinCameraSpeedMultiplier = 0.1f;
constexpr float MaxCameraSpeedMultiplier = 15.0f;
constexpr float CameraSpeedMultiplierStep = 0.5f;
}  // namespace

EditorPawn3D::EditorPawn3D() {
  SetActorLocation3D({0.0f, 5.0f, -8.0f});
  SetActorRotation3D(FQuaternion::FromRotator({25.0f, 0.0f, 0.0f}));
  EditorCamera3D = NewObject<MCamera3DComponent>(this);
  if (EditorCamera3D != nullptr) {
    EditorCamera3D->RegisterComponent();
  }
  bEditorActor = true;
}

EditorPawn3D::~EditorPawn3D() { EndCameraNavigation(); }

void EditorPawn3D::OnPossessedBy(APlayerController* NewController) {
  APawn::OnPossessedBy(NewController);
  IsPossessed = true;
  if (EditorCamera3D != nullptr) {
    EditorCamera3D->SetActiveCamera();
  }
}

void EditorPawn3D::OnUnPossessed() {
  EndCameraNavigation();
  IsPossessed = false;
  APawn::OnUnPossessed();
}

void EditorPawn3D::SetupPlayerInputComponent(MEnhancedInputComponent* PlayerInputComponent) {
  PlayerInputComponent->BindAction(
      InputAction::Move, ETriggerEvent::Triggered, this, &EditorPawn3D::OnMove
  );
  PlayerInputComponent->BindAction(
      InputAction::Move, ETriggerEvent::Completed, this, &EditorPawn3D::OnMove
  );
  PlayerInputComponent->BindAction(
      EditorInputAction::MoveVertical, ETriggerEvent::Triggered, this, &EditorPawn3D::OnVerticalMove
  );
  PlayerInputComponent->BindAction(
      EditorInputAction::MoveVertical, ETriggerEvent::Completed, this, &EditorPawn3D::OnVerticalMove
  );
  PlayerInputComponent->BindAction(
      InputActionMouse::MouseLeft, ETriggerEvent::Started, this, &EditorPawn3D::OnMouseLeftPress
  );
  PlayerInputComponent->BindAction(
      InputActionMouse::MouseRight, ETriggerEvent::Started, this, &EditorPawn3D::OnMouseRightPress
  );
  PlayerInputComponent->BindAction(
      InputActionMouse::MouseRight,
      ETriggerEvent::Completed,
      this,
      &EditorPawn3D::OnMouseRightRelease
  );
  PlayerInputComponent->BindAction(
      InputAction::Look, ETriggerEvent::Triggered, this, &EditorPawn3D::OnMouseMove
  );
  PlayerInputComponent->BindAction(
      InputActionMouse::Wheel, ETriggerEvent::Triggered, this, &EditorPawn3D::OnWheel
  );
}

void EditorPawn3D::OnUpdate(float DeltaTime) {
  if (IsPossessed) {
    UpdateCamera(DeltaTime);
  }
}

void EditorPawn3D::BeginPlay() {
  EditorModePtr = dynamic_cast<EditorMode*>(GetWorld()->GetGameMode());
}

bool EditorPawn3D::IsCameraNavigationActive() const { return CameraNavigationActive; }

void EditorPawn3D::EndCameraNavigation() {
  if (CameraNavigationActive) {
    EnableCursor();
  }
  CameraNavigationActive = false;
  DiscardNextCameraDelta = false;
  MovementInput = FVector2D::ZeroVector();
  VerticalMovementInput = 0.0f;
}

void EditorPawn3D::FocusActor3D(AActor* Actor) {
  if (Actor == nullptr || Actor->IsPendingDestroy()) {
    return;
  }

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

void EditorPawn3D::UpdateCamera(float DeltaTime) {
  if (EditorCamera3D == nullptr || !IsWindowFocused()) {
    EndCameraNavigation();
    return;
  }

  if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT) && EditorModePtr != nullptr &&
      EditorModePtr->IsViewportInputAvailable()) {
    CameraNavigationActive = true;
    DiscardNextCameraDelta = true;
    DisableCursor();
  }
  if (!IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
    EndCameraNavigation();
  }

  if (EditorModePtr != nullptr && EditorModePtr->IsViewportInputAvailable() &&
      IsKeyPressed(KEY_F)) {
    FocusActor3D(EditorModePtr->GetSelectedActor());
  }
  if (!CameraNavigationActive || EditorModePtr == nullptr) {
    return;
  }

  const float SpeedMultiplier =
      std::clamp(CameraSpeedMultiplier, MinCameraSpeedMultiplier, MaxCameraSpeedMultiplier);
  const float Speed = (IsKeyDown(KEY_LEFT_SHIFT) ? 20.0f : 8.0f) * SpeedMultiplier;
  FVector3D Movement = EditorCamera3D->GetForwardVector() * MovementInput.Y;
  Movement += EditorCamera3D->GetRightVector() * -MovementInput.X;
  Movement.Y += VerticalMovementInput;
  if (Movement.SizeSquared() > 0.0f) {
    SetActorLocation3D(GetActorLocation3D() + Movement.Normalize() * Speed * DeltaTime);
  }

  const Vector2 MouseDelta = GetMouseDelta();
  if (DiscardNextCameraDelta) {
    DiscardNextCameraDelta = false;
    return;
  }
  const FVector2D ViewportDelta =
      EditorModePtr->GetViewportState().ScreenDeltaToRenderTarget({MouseDelta.x, MouseDelta.y});
  if (ViewportDelta.SizeSquared() <= 0.0001f) {
    return;
  }

  constexpr float CameraLookSensitivity = 0.03f;
  const FQuaternion LocalPitch =
      FQuaternion::FromRotator({ViewportDelta.Y * CameraLookSensitivity, 0.0f, 0.0f});
  const FQuaternion WorldYaw =
      FQuaternion::FromRotator({0.0f, -ViewportDelta.X * CameraLookSensitivity, 0.0f});
  SetActorRotation3D((WorldYaw * GetActorRotation3D() * LocalPitch).Normalize());
}

void EditorPawn3D::OnMove(const FInputActionValue& Value) {
  if (CameraNavigationActive) {
    MovementInput = Value.Axis2D;
  }
}

void EditorPawn3D::OnVerticalMove(const FInputActionValue& Value) {
  if (CameraNavigationActive) {
    VerticalMovementInput = Value.Axis1D;
  }
}

void EditorPawn3D::OnMouseLeftPress(const FInputActionValue&) {
  if (EditorModePtr == nullptr || !EditorModePtr->IsViewportInputAvailable()) {
    return;
  }
  EditorModePtr->OnMousePress3D();
}

void EditorPawn3D::OnMouseRightPress(const FInputActionValue&) {
  if (EditorModePtr != nullptr && EditorModePtr->IsViewportInputAvailable()) {
    CameraNavigationActive = true;
    DiscardNextCameraDelta = true;
    DisableCursor();
  }
}

void EditorPawn3D::OnMouseRightRelease(const FInputActionValue&) { EndCameraNavigation(); }

void EditorPawn3D::OnMouseMove(const FInputActionValue&) {}

void EditorPawn3D::OnWheel(const FInputActionValue& Value) {
  if (!CameraNavigationActive) {
    return;
  }
  CameraSpeedMultiplier = std::clamp(
      CameraSpeedMultiplier + Value.Axis1D * CameraSpeedMultiplierStep,
      MinCameraSpeedMultiplier,
      MaxCameraSpeedMultiplier
  );
}
