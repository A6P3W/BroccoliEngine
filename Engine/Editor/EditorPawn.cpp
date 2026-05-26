#include "EditorPawn.h"
#include "EditorMode.h"
#include "EnhancedInputComponent.h"
#include "RenderSystem.h"
#include <DxLib.h>

EditorPawn::EditorPawn()
{}

void EditorPawn::OnPossesed()
{
	APawn::OnPossesed();
}

void EditorPawn::SetupPlayerInputComponent(MEnhancedInputComponent* comp)
{
	// 左クリック押下・離し
	comp->BindAction(InputActionMouse::MouseLeft, ETriggerEvent::Started,
		this, &EditorPawn::OnMouseLeftPress);
	comp->BindAction(InputActionMouse::MouseLeft, ETriggerEvent::Completed,
		this, &EditorPawn::OnMouseLeftRelease);
}

void EditorPawn::OnUpdate(float DeltaTime)
{
	// ドラッグ中はプレビューアクタをマウス追従
	if (EditorMode::GetInstance().GetState() == EEditorState::Dragging)
	{
		EditorMode::GetInstance().OnMouseMove(GetMouseWorldPosition());
	}
}
void EditorPawn::OnMouseLeftPress(const FInputActionValue&)
{
	EditorMode::GetInstance().OnMousePress(GetMouseWorldPosition());
}

void EditorPawn::OnMouseLeftRelease(const FInputActionValue&)
{
	EditorMode::GetInstance().OnMouseRelease(GetMouseWorldPosition());
}

FVector2D EditorPawn::GetMouseWorldPosition() const
{
	int mx, my;
	GetMousePoint(&mx, &my);
	return RenderSystem::GetInstance().ScreenToWorld({ static_cast<float>(mx),
													   static_cast<float>(my) });
}
