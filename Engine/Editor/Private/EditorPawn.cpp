#include "EditorPawn.h"

#include <imgui.h>

#include "BroccoliRaylib.h"
#include "CameraComponent.h"
#include "EditorMode.h"
#include "EnhancedInputComponent.h"
#include "InputManager.h"
#include "MouseDevice.h"
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

void EditorPawn::SetupPlayerInputComponent(MEnhancedInputComponent* comp) {
  comp->BindAction(
      InputActionMouse::MouseLeft, ETriggerEvent::Started, this, &EditorPawn::OnMouseLeftPress
  );
  comp->BindAction(
      InputActionMouse::MouseLeft, ETriggerEvent::Completed, this, &EditorPawn::OnMouseLeftRelease
  );

  comp->BindAction(
      InputActionMouse::MouseRight, ETriggerEvent::Started, this, &EditorPawn::OnMouseRightPress
  );
  comp->BindAction(
      InputActionMouse::MouseRight, ETriggerEvent::Completed, this, &EditorPawn::OnMouseRightRelease
  );

  comp->BindAction(InputAction::Look, ETriggerEvent::Triggered, this, &EditorPawn::OnMouseMove);
  comp->BindAction(InputActionMouse::Wheel, ETriggerEvent::Triggered, this, &EditorPawn::OnWheel);
}

void EditorPawn::OnUpdate(float DeltaTime) {
  FVector2D ZeroPoint = {150, 150};
  GameScreenView->SetWorldLocation(RenderSystem::GetInstance().ScreenToWorld(ZeroPoint));
}
void EditorPawn::BeginPlay() {
  EditorModePtr = dynamic_cast<EditorMode*>(GetWorld()->GetGameMode());
}
void EditorPawn::OnMove(const FInputActionValue& Value) {
  if (!bRightMousePressed) return;
}
void EditorPawn::OnMouseLeftPress(const FInputActionValue&) {
  if (ImGui::GetIO().WantCaptureMouse) return;
  EditorModePtr->OnMousePress(GetMouseWorldPosition());
}

void EditorPawn::OnMouseLeftRelease(const FInputActionValue&) {
  EditorModePtr->OnMouseRelease(GetMouseWorldPosition());
}

void EditorPawn::OnMouseRightPress(const FInputActionValue& Value) {
  if (ImGui::GetIO().WantCaptureMouse) return;
  bRightMousePressed = true;
  const Vector2 MousePosition = GetMousePosition();
  DragStartMousePos = {MousePosition.x, MousePosition.y};
  DragStartCameraPos = GetActorLocation();
  HideCursor();
  MousePointX = static_cast<int>(MousePosition.x);
  MousePointY = static_cast<int>(MousePosition.y);
}

void EditorPawn::OnMouseRightRelease(const FInputActionValue& Value) {
  if (!bRightMousePressed) return;
  bRightMousePressed = false;
  ShowCursor();
  SetMousePosition(MousePointX, MousePointY);
}

void EditorPawn::OnMouseMove(const FInputActionValue& Value) {
  if (EditorModePtr->GetState() == EEditorState::Dragging) {
    EditorModePtr->OnMouseMove(Value.Axis2D);
  }
  if (bRightMousePressed) {
    const Vector2 MousePosition = GetMousePosition();
    FVector2D currentMousePos = {MousePosition.x, MousePosition.y};
    FVector2D screenDelta = {
        currentMousePos.X - static_cast<float>(MousePointX),
        currentMousePos.Y - static_cast<float>(MousePointY)
    };

    if (screenDelta.SizeSquared() > 0.0001f) {
      float fov = Camera ? Camera->GetFOV() : 1.0f;
      if (std::abs(fov) < 1e-6f) fov = 1e-6f;

      FVector2D worldDelta = screenDelta * (1.0f / fov);
      worldDelta = worldDelta.RotateVector(GetActorRotation());

      AddActorWorldOffset(worldDelta * -1.0f);

      SetMousePosition(MousePointX, MousePointY);
    }
  }
}

void EditorPawn::OnWheel(const FInputActionValue& Value) {
  if (bRightMousePressed) {
    float zoomAmount = Value.Axis1D * -0.1f;
    if (Camera) {
      Camera->SetFOV(Camera->GetFOV() * (1.0f - zoomAmount));
    }
  }
}

FVector2D EditorPawn::GetMouseWorldPosition() const {
  const MouseDevice* Mouse = InputManager::GetInstance().GetDevice<MouseDevice>();
  if (Mouse == nullptr) return FVector2D::ZeroVector();
  return RenderSystem::GetInstance().ScreenToWorld(
      {static_cast<float>(Mouse->GetMouseX()), static_cast<float>(Mouse->GetMouseY())}
  );
}
