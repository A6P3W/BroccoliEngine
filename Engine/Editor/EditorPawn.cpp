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
#include "SpriteComponent.h"

EditorPawn::EditorPawn()
{
	auto GameScreenComp = std::make_unique<MSpriteComponent>();
	GameScreenComp->SubmitBox(1920, 1080, GetColor(255,255,255), 0);
	GameScreenView = GameScreenComp.get();
	AddComponent(std::move(GameScreenComp));
}


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
	comp->BindAction(InputActionMouse::Wheel, ETriggerEvent::Triggered, this, &EditorPawn::OnWheel);
}

void EditorPawn::OnUpdate(float DeltaTime)
{
	// ドラッグ中はプレビューアクタをマウス追従
	if (m_editorMode->GetState() == EEditorState::Dragging)
	{
		m_editorMode->OnMouseMove(GetMouseWorldPosition());
	}
	FVector2D ZeroPoint={150,150};
	GameScreenView->SetWorldLocation(RenderSystem::GetInstance().ScreenToWorld(ZeroPoint));
}
void EditorPawn::BeginPlay()
{
	m_editorMode = dynamic_cast<EditorMode*>(GetWorld()->GetGameMode());
}
void EditorPawn::OnMove(const FInputActionValue& Value)
{
	if (!m_RightMousePressed)return;

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
	SetMouseDispFlag(FALSE);
	GetMousePoint(&MousePointX, &MousePointY);
}

void EditorPawn::OnMouseRightRelease(const FInputActionValue & Value)
{
	m_RightMousePressed = false;
	SetMouseDispFlag(true);
	SetMousePoint(MousePointX, MousePointY);
}

void EditorPawn::OnMouseMove(const FInputActionValue& Value) {
	if (m_RightMousePressed) {
		int mx, my;
		GetMousePoint(&mx, &my);

		FVector2D currentMousePos = { static_cast<float>(mx), static_cast<float>(my) };
		FVector2D screenDelta = { currentMousePos.X - static_cast<float>(MousePointX),
								  currentMousePos.Y - static_cast<float>(MousePointY) };

		if (screenDelta.SizeSquared() > 0.0001f) {
			float fov = m_camera ? m_camera->GetFOV() : 1.0f;
			if (std::abs(fov) < 1e-6f) fov = 1e-6f;

			FVector2D worldDelta = screenDelta * (1.0f / fov);
			worldDelta = worldDelta.RotateVector(GetActorRotation());

			AddActorWorldOffset(worldDelta * -1.0f);

			SetMousePoint(MousePointX, MousePointY);
		}
	}
}

void EditorPawn::OnWheel(const FInputActionValue& Value)
{
	if (m_RightMousePressed)
	{
		float zoomAmount = Value.Axis1D * -0.1f;
		if (m_camera) {
			m_camera->SetFOV(m_camera->GetFOV() * (1.0f - zoomAmount));
		}
	}
}

FVector2D EditorPawn::GetMouseWorldPosition() const
{
	int mx, my;
	GetMousePoint(&mx, &my);
	return RenderSystem::GetInstance().ScreenToWorld({ static_cast<float>(mx),
													   static_cast<float>(my) });
}
