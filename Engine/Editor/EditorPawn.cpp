#include "EditorPawn.h"
#include "EditorMode.h"
#include "EnhancedInputComponent.h"
#include "RenderSystem.h"
#include "SceneManager.h"
#include <DxLib.h>
#include "EditorMode.h"
#include "World.h"
#include <imgui.h>
#include "CameraComponent.h"
EditorPawn::EditorPawn()
{}


void EditorPawn::OnPossesed()
{
	APawn::OnPossesed();


}

void EditorPawn::SetupPlayerInputComponent(MEnhancedInputComponent* comp)
{
	comp->BindAction(InputActionMouse::MouseLeft, ETriggerEvent::Started, this, &EditorPawn::OnMouseLeftPress);
	comp->BindAction(InputActionMouse::MouseLeft, ETriggerEvent::Completed, this, &EditorPawn::OnMouseLeftRelease);

	comp->BindAction(InputActionMouse::MouseRight, ETriggerEvent::Started, this, &EditorPawn::OnMouseRightPress);
	comp->BindAction(InputActionMouse::MouseRight, ETriggerEvent::Completed, this, &EditorPawn::OnMouseRightRelease);


	comp->BindAction(InputAction::Look, ETriggerEvent::Triggered, this, &EditorPawn::OnMouseMove);
}

void EditorPawn::OnUpdate(float DeltaTime)
{
	// ドラッグ中はプレビューアクタをマウス追従
	if (m_editorMode->GetState() == EEditorState::Dragging)
	{
		m_editorMode->OnMouseMove(GetMouseWorldPosition());
	}
}
void EditorPawn::BeginPlay()
{
	m_editorMode = dynamic_cast<EditorMode*>(GetWorld()->GetGameMode());
}
void EditorPawn::OnMouseLeftPress(const FInputActionValue&)
{
	if (ImGui::GetIO().WantCaptureMouse) return;
	m_editorMode->OnMousePress(GetMouseWorldPosition());
}

void EditorPawn::OnMouseLeftRelease(const FInputActionValue&)
{
	m_editorMode->OnMouseRelease(GetMouseWorldPosition());
}

void EditorPawn::OnMouseRightPress(const FInputActionValue& Value)
{
	m_RightMousePressed = true;
	int mx, my;
	GetMousePoint(&mx, &my);
	m_dragStartMousePos = { static_cast<float>(mx), static_cast<float>(my) };
	m_dragStartCameraPos = GetActorLocation();
}

void EditorPawn::OnMouseRightRelease(const FInputActionValue & Value)
{
	m_RightMousePressed = false;
}

void EditorPawn::OnMouseMove(const FInputActionValue& Value)
{
	if (m_RightMousePressed)
	{
		int mx, my;
		GetMousePoint(&mx, &my);
		FVector2D currentMousePos = { static_cast<float>(mx), static_cast<float>(my) };

		FVector2D screenDelta = { currentMousePos.X - m_dragStartMousePos.X , currentMousePos.Y - m_dragStartMousePos.Y };

		float fov = m_camera ? m_camera->GetFOV() : 1.0f;
		if (std::abs(fov) < 1e-6f) fov = 1e-6f;

		FVector2D worldDelta = screenDelta * (1.0f / fov);

		worldDelta = worldDelta.RotateVector(GetActorRotation());

		SetActorLocation({ m_dragStartCameraPos.X - worldDelta.X, m_dragStartCameraPos.Y - worldDelta.Y });
	}
}

FVector2D EditorPawn::GetMouseWorldPosition() const
{
	int mx, my;
	GetMousePoint(&mx, &my);
	return RenderSystem::GetInstance().ScreenToWorld({ static_cast<float>(mx),
													   static_cast<float>(my) });
}
