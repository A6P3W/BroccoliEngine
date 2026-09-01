#include "EditorPawn.h"

#include "BroccoliRaylib.h"
#include "CameraComponent.h"
#include "EditorMode.h"
#include "EnhancedInputComponent.h"
#include "RenderSystem.h"
#include "SceneManager.h"
#include "SpriteComponent.h"
#include "World.h"
REGISTER_ACTOR(EditorPawn);
EditorPawn::EditorPawn() {
  GameScreenView = NewObject<MSpriteComponent>(this);
  if (GameScreenView) {
    GameScreenView->SetRenderSettings(999, RenderSpace::World);
    GameScreenView->SubmitBox(1920, 1080, FColor{255, 255, 255}, 0);
    GameScreenView->RegisterComponent();
  }
  bEditorActor = true;
}

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
  FVector2D ZeroPoint = {150, 150};
  GameScreenView->SetWorldLocation(RenderSystem::GetInstance().ScreenToWorld(ZeroPoint));
}
void EditorPawn::BeginPlay() {
  EditorModePtr = dynamic_cast<EditorMode*>(GetWorld()->GetGameMode());
}
void EditorPawn::OnMove(const FInputActionValue& Value) {
  if (!RightMousePressed) return;
}
void EditorPawn::OnMouseLeftPress(const FInputActionValue&) {
  if (EditorModePtr == nullptr || !EditorModePtr->IsViewportInputAvailable()) return;

  FVector2D MouseVirtualPosition;
  if (EditorModePtr->TryGetViewportVirtualMousePosition(MouseVirtualPosition)) {
    EditorModePtr->OnMousePress(RenderSystem::GetInstance().ScreenToWorld(MouseVirtualPosition));
  }
}

void EditorPawn::OnMouseLeftRelease(const FInputActionValue&) {
  if (EditorModePtr == nullptr || EditorModePtr->GetState() != EEditorState::Dragging) return;

  FVector2D MouseVirtualPosition;
  if (EditorModePtr->TryGetViewportVirtualMousePosition(MouseVirtualPosition, false)) {
    EditorModePtr->OnMouseRelease(RenderSystem::GetInstance().ScreenToWorld(MouseVirtualPosition));
  } else {
    EditorModePtr->OnMouseRelease(FVector2D::ZeroVector());
  }
}

void EditorPawn::OnMouseRightPress(const FInputActionValue&) {
  if (EditorModePtr == nullptr || !EditorModePtr->IsViewportInputAvailable()) return;

  RightMousePressed = true;
  const Vector2 MousePosition = GetMousePosition();
  HideCursor();
  MousePointX = static_cast<int>(MousePosition.x);
  MousePointY = static_cast<int>(MousePosition.y);
}

void EditorPawn::OnMouseRightRelease(const FInputActionValue&) {
  if (!RightMousePressed) return;
  RightMousePressed = false;
  ShowCursor();
  SetMousePosition(MousePointX, MousePointY);
}

void EditorPawn::OnMouseMove(const FInputActionValue&) {
  if (EditorModePtr == nullptr) return;

  const Vector2 MouseDelta = GetMouseDelta();
  const FVector2D VirtualDelta =
      EditorModePtr->GetViewportState().ScreenDeltaToVirtual({MouseDelta.x, MouseDelta.y});
  if (EditorModePtr->GetState() == EEditorState::Dragging) {
    EditorModePtr->OnMouseMove(VirtualDelta);
  }
  if (RightMousePressed) {
    const Vector2 MousePosition = GetMousePosition();
    const FVector2D CurrentMousePosition = {MousePosition.x, MousePosition.y};
    const FVector2D ScreenDelta = {
        CurrentMousePosition.X - static_cast<float>(MousePointX),
        CurrentMousePosition.Y - static_cast<float>(MousePointY)
    };
    const FVector2D ViewportDelta =
        EditorModePtr->GetViewportState().ScreenDeltaToVirtual(ScreenDelta);

    if (ViewportDelta.SizeSquared() > 0.0001f) {
      float FieldOfView = Camera ? Camera->GetFOV() : 1.0f;
      if (std::abs(FieldOfView) < 1e-6f) FieldOfView = 1e-6f;

      FVector2D WorldDelta = ViewportDelta * (1.0f / FieldOfView);
      WorldDelta = WorldDelta.RotateVector(GetActorRotation());

      AddActorWorldOffset(WorldDelta * -1.0f);

      SetMousePosition(MousePointX, MousePointY);
    }
  }
}

void EditorPawn::OnWheel(const FInputActionValue& Value) {
  if (EditorModePtr == nullptr || !EditorModePtr->IsViewportInputAvailable() || Camera == nullptr) {
    return;
  }

  const float ZoomAmount = Value.Axis1D * -0.1f;
  Camera->SetFOV(Camera->GetFOV() * (1.0f - ZoomAmount));
}
