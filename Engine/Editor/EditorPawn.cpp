#include "EditorPawn.h"
#include "EditorMode.h"
#include "EnhancedInputComponent.h"
#include "RenderSystem.h"
#include "SceneManager.h"
#include <DxLib.h>
#include "EditorMode.h"
#include "World.h"
EditorPawn::EditorPawn()
{
}


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
	m_editorMode->OnMousePress(GetMouseWorldPosition());
}

void EditorPawn::OnMouseLeftRelease(const FInputActionValue&)
{
	m_editorMode->OnMouseRelease(GetMouseWorldPosition());
}

FVector2D EditorPawn::GetMouseWorldPosition() const
{
	int mx, my;
	GetMousePoint(&mx, &my);
	return RenderSystem::GetInstance().ScreenToWorld({ static_cast<float>(mx),
													   static_cast<float>(my) });
}
