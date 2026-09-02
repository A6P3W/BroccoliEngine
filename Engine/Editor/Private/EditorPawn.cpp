#include "EditorPawn.h"

#include <algorithm>

#include "BroccoliRaylib.h"
#include "CameraComponent.h"
#include "EditorMode.h"
#include "EnhancedInputComponent.h"
#include "RenderSystem.h"
#include "SceneManager.h"
#include "SpriteComponent.h"
#include "World.h"
REGISTER_ACTOR(EditorPawn);

namespace {
constexpr float MinEditorFOV = 0.1f;
constexpr float MaxEditorFOV = 10.0f;
}  // namespace

EditorPawn::EditorPawn() {
  GameScreenView = NewObject<MSpriteComponent>(this);
  if (GameScreenView) {
    GameScreenView->SetRenderSettings(999, RenderSpace::World);
    GameScreenView->SubmitBox(1920, 1080, FColor{255, 255, 255}, 0);
    GameScreenView->RegisterComponent();
  }
  bEditorActor = true;
}

EditorPawn::~EditorPawn() { EndCameraDrag(); }

void EditorPawn::OnPossessedBy(APlayerController* NewController) {
  APawn::OnPossessedBy(NewController);
}

void EditorPawn::SetupPlayerInputComponent(MEnhancedInputComponent* PlayerInputComponent) {
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
  if (CameraDragActive && (!IsWindowFocused() || EditorModePtr == nullptr || Camera == nullptr ||
                           !EditorModePtr->GetViewportState().HasValidImage() ||
                           !IsMouseButtonDown(MOUSE_BUTTON_RIGHT))) {
    EndCameraDrag();
  }
  UpdateCameraDrag();

  FVector2D ZeroPoint = {150, 150};
  GameScreenView->SetWorldLocation(RenderSystem::GetInstance().ScreenToWorld(ZeroPoint));
}
void EditorPawn::BeginPlay() {
  EditorModePtr = dynamic_cast<EditorMode*>(GetWorld()->GetGameMode());
}
void EditorPawn::OnMove(const FInputActionValue& Value) {
  if (!CameraDragActive) return;
}
void EditorPawn::OnMouseLeftPress(const FInputActionValue&) {
  if (EditorModePtr == nullptr || !EditorModePtr->IsViewportInputAvailable()) return;

  FVector2D MouseRenderTargetPosition;
  if (EditorModePtr->TryGetViewportRenderTargetMousePosition(MouseRenderTargetPosition)) {
    EditorModePtr->OnMousePress(
        RenderSystem::GetInstance().ScreenToWorld(MouseRenderTargetPosition)
    );
  }
}

void EditorPawn::OnMouseLeftRelease(const FInputActionValue&) {
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

void EditorPawn::OnMouseRightPress(const FInputActionValue&) { BeginCameraDrag(); }

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

void EditorPawn::OnMouseRightRelease(const FInputActionValue&) { EndCameraDrag(); }

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
  FVector2D WorldDelta = RenderTargetDelta * (1.0f / FieldOfView);
  WorldDelta = WorldDelta.RotateVector(GetActorRotation());
  AddActorWorldOffset(WorldDelta * -1.0f);
}

void EditorPawn::OnMouseMove(const FInputActionValue&) {
  if (EditorModePtr == nullptr || CameraDragActive) return;

  const Vector2 MouseDelta = GetMouseDelta();
  const FVector2D RenderTargetDelta =
      EditorModePtr->GetViewportState().ScreenDeltaToRenderTarget({MouseDelta.x, MouseDelta.y});
  if (EditorModePtr->GetState() == EEditorState::Dragging) {
    EditorModePtr->OnMouseMove(RenderTargetDelta);
  }
}

void EditorPawn::OnWheel(const FInputActionValue& Value) {
  if (EditorModePtr == nullptr || !EditorModePtr->IsViewportInputAvailable() || Camera == nullptr) {
    return;
  }

  const float ZoomAmount = Value.Axis1D * -0.1f;
  const float NewFieldOfView = Camera->GetFOV() * (1.0f - ZoomAmount);
  Camera->SetFOV(std::clamp(NewFieldOfView, MinEditorFOV, MaxEditorFOV));
}
