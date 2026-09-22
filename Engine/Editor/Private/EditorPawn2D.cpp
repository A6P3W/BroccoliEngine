#include "EditorPawn2D.h"

#include <algorithm>

#include "BroccoliRaylib.h"
#include "Camera2DComponent.h"
#include "EditorMode.h"
#include "EnhancedInputComponent.h"
#include "RenderSystem.h"
#include "SpriteComponent.h"
#include "World.h"

REGISTER_ACTOR(EditorPawn2D);

namespace {
constexpr float MinEditorFOV = 0.01f;
constexpr float MaxEditorFOV = 1000.0f;
}  // namespace

EditorPawn2D::EditorPawn2D() {
  GameScreenView = NewObject<MSpriteComponent>(this);
  if (GameScreenView != nullptr) {
    GameScreenView->SetRenderSettings(999, RenderSpace::World);
    GameScreenView->SubmitBox(1920, 1080, FColor{255, 255, 255}, 0);
    GameScreenView->RegisterComponent();
  }
  bEditorActor = true;
}

EditorPawn2D::~EditorPawn2D() { EndCameraDrag(); }

void EditorPawn2D::OnPossessedBy(APlayerController* NewController) {
  APawn::OnPossessedBy(NewController);
  IsPossessed = true;
  if (GameScreenView != nullptr) {
    GameScreenView->SetVisibility(true);
  }
  RenderSystem::GetInstance().SetCameraView3D(nullptr);
}

void EditorPawn2D::OnUnPossessed() {
  EndCameraDrag();
  IsPossessed = false;
  if (GameScreenView != nullptr) {
    GameScreenView->SetVisibility(false);
  }
  APawn::OnUnPossessed();
}

void EditorPawn2D::SetupPlayerInputComponent(MEnhancedInputComponent* PlayerInputComponent) {
  PlayerInputComponent->BindAction(
      InputActionMouse::MouseLeft, ETriggerEvent::Started, this, &EditorPawn2D::OnMouseLeftPress
  );
  PlayerInputComponent->BindAction(
      InputActionMouse::MouseLeft, ETriggerEvent::Completed, this, &EditorPawn2D::OnMouseLeftRelease
  );
  PlayerInputComponent->BindAction(
      InputActionMouse::MouseRight, ETriggerEvent::Started, this, &EditorPawn2D::OnMouseRightPress
  );
  PlayerInputComponent->BindAction(
      InputActionMouse::MouseRight,
      ETriggerEvent::Completed,
      this,
      &EditorPawn2D::OnMouseRightRelease
  );
  PlayerInputComponent->BindAction(
      InputAction::Look, ETriggerEvent::Triggered, this, &EditorPawn2D::OnMouseMove
  );
  PlayerInputComponent->BindAction(
      InputActionMouse::Wheel, ETriggerEvent::Triggered, this, &EditorPawn2D::OnWheel
  );
}

void EditorPawn2D::OnUpdate(float DeltaTime) {
  (void)DeltaTime;
  if (!IsPossessed) {
    return;
  }
  if (CameraDragActive && (!IsWindowFocused() || EditorModePtr == nullptr || Camera == nullptr ||
                           !EditorModePtr->GetViewportState().HasValidImage() ||
                           !IsMouseButtonDown(MOUSE_BUTTON_RIGHT))) {
    EndCameraDrag();
  }
  UpdateCameraDrag();
}

void EditorPawn2D::BeginPlay() {
  EditorModePtr = dynamic_cast<EditorMode*>(GetWorld()->GetGameMode());
}

void EditorPawn2D::OnMouseLeftPress(const FInputActionValue&) {
  if (EditorModePtr == nullptr || !EditorModePtr->IsViewportInputAvailable()) {
    return;
  }

  FVector2D MouseRenderTargetPosition;
  if (EditorModePtr->TryGetViewportRenderTargetMousePosition(MouseRenderTargetPosition)) {
    EditorModePtr->OnMousePress(
        RenderSystem::GetInstance().ScreenToWorld(MouseRenderTargetPosition)
    );
  }
}

void EditorPawn2D::OnMouseLeftRelease(const FInputActionValue&) {
  if (EditorModePtr == nullptr || EditorModePtr->GetState() != EEditorState::Dragging) {
    return;
  }

  FVector2D MouseRenderTargetPosition;
  if (EditorModePtr->TryGetViewportRenderTargetMousePosition(MouseRenderTargetPosition, false)) {
    EditorModePtr->OnMouseRelease(
        RenderSystem::GetInstance().ScreenToWorld(MouseRenderTargetPosition)
    );
  } else {
    EditorModePtr->OnMouseRelease(FVector2D::ZeroVector());
  }
}

void EditorPawn2D::OnMouseRightPress(const FInputActionValue&) { BeginCameraDrag(); }

void EditorPawn2D::BeginCameraDrag() {
  if (CameraDragActive || EditorModePtr == nullptr || Camera == nullptr ||
      EditorModePtr->GetState() == EEditorState::Dragging ||
      !EditorModePtr->IsViewportInputAvailable()) {
    return;
  }

  CameraDragActive = true;
  DiscardNextCameraDelta = true;
  DisableCursor();
}

void EditorPawn2D::EndCameraDrag() {
  if (!CameraDragActive) {
    return;
  }

  CameraDragActive = false;
  DiscardNextCameraDelta = false;
  EnableCursor();
}

void EditorPawn2D::OnMouseRightRelease(const FInputActionValue&) { EndCameraDrag(); }

void EditorPawn2D::UpdateCameraDrag() {
  if (!CameraDragActive || EditorModePtr == nullptr || Camera == nullptr) {
    return;
  }

  const Vector2 MouseDelta = GetMouseDelta();
  if (DiscardNextCameraDelta) {
    DiscardNextCameraDelta = false;
    return;
  }
  const FVector2D RenderTargetDelta =
      EditorModePtr->GetViewportState().ScreenDeltaToRenderTarget({MouseDelta.x, MouseDelta.y});
  if (RenderTargetDelta.SizeSquared() <= 0.0001f) {
    return;
  }

  const float FieldOfView = std::clamp(Camera->GetFOV(), MinEditorFOV, MaxEditorFOV);
  FVector2D WorldDelta = {RenderTargetDelta.X, -RenderTargetDelta.Y};
  WorldDelta = WorldDelta.RotateVector(GetActorRotation());
  AddActorWorldOffset(WorldDelta * -1.0f / FieldOfView);
}

void EditorPawn2D::OnMouseMove(const FInputActionValue&) {
  if (EditorModePtr == nullptr || CameraDragActive) {
    return;
  }

  const Vector2 MouseDelta = GetMouseDelta();
  const FVector2D RenderTargetDelta =
      EditorModePtr->GetViewportState().ScreenDeltaToRenderTarget({MouseDelta.x, MouseDelta.y});
  if (EditorModePtr->GetState() == EEditorState::Dragging) {
    EditorModePtr->OnMouseMove(RenderTargetDelta);
  }
}

void EditorPawn2D::OnWheel(const FInputActionValue& Value) {
  if (EditorModePtr == nullptr) {
    return;
  }
  if (!EditorModePtr->IsViewportInputAvailable() || Camera == nullptr) {
    return;
  }

  const float ZoomAmount = Value.Axis1D * -0.1f;
  const float NewFieldOfView = Camera->GetFOV() * (1.0f - ZoomAmount);
  Camera->SetFOV(std::clamp(NewFieldOfView, MinEditorFOV, MaxEditorFOV));
}
