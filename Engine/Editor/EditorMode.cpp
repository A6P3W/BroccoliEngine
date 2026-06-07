#include "EditorMode.h"
#include "ActorRegistry.h"
#include "ObjectManager.h"
#include "Actor.h"
#include "EditorUI.h"
#include "EditorPawn.h"
#include <PlayerController.h>
#include "Utils/Log.h"
#include "Utils/UMath.h"
#include "World.h"
#include "SceneManager.h"
#include "EditorSelectPointComponent.h"
#include "EditorController.h"
#include <DxLib.h>
#include "RenderSystem.h"
const std::vector<std::string>& EditorMode::GetClassList() const
{
	return ActorRegistry::GetInstance().GetClassNames();
}

void EditorMode::OnMousePress(const FVector2D& worldPos)
{
	const auto& actors = GetWorld()->GetObjectManager()->GetAllActors();
	AActor* hitActor = nullptr;

	for (auto& actor : actors) {
		FVector2D actorPos = actor->GetActorLocation();
		float distanceSq = FVector2D({ actorPos.X - worldPos.X, actorPos.Y - worldPos.Y }).SizeSquared();
		const float selectionRadiusSq = 15.0f * 15.0f;
		if (distanceSq <= selectionRadiusSq) {
			hitActor = actor.get();
			SetSelectedActor(hitActor);
			if (SelectedPointComponent)
			{
				SelectedPointComponent->Selected(false);
			}
			SelectingActor = hitActor;
			SelectedPointComponent = hitActor->GetComponents<EditorSelectPointComponent>()[0];
			SelectedPointComponent->Selected(true);
			M_LOG("Hit Actor: {}", hitActor->GetActorClassName());
			M_LOG("Current Actor Action: {}", (int)GetActorAction());
			m_state = EEditorState::Dragging;
			return;

		}
	}


	if (m_selectedClass.empty()) return;
	if (m_state == EEditorState::Dragging) return;

	// プレビュー用アクタをスポーン
	SelectingActor = ActorRegistry::GetInstance().Spawn(GetWorld(), m_selectedClass, worldPos);
	if (!SelectingActor) return;
	if (SelectedPointComponent)
	{
		SelectedPointComponent->Selected(false);
	}
	SelectedPointComponent = SelectingActor->GetComponents<EditorSelectPointComponent>()[0];
	SelectedPointComponent->Selected(true);

	SetSelectedActor(SelectingActor);

	m_state = EEditorState::Dragging;
}

void EditorMode::OnMouseMove(const FVector2D& Delta)
{
	if (m_state != EEditorState::Dragging) return;
	if (!SelectingActor) return;
	
	switch (GetActorAction())
	{
	case EActorAction::Select:
		break;
	case EActorAction::Move:
		SelectingActor->SetActorLocation(GetMouseWorldPosition());
		break;
	case EActorAction::Rotate:
		SelectingActor->AddActorRotation(Delta.X*0.25);
		break;
	case EActorAction::Scale:
		float NewScale = SelectingActor->GetActorScale().Scale * (1 + Delta.X * 0.001f);
		SelectingActor->SetActorScale(NewScale);
		break;
	}
}

void EditorMode::OnMouseRelease(const FVector2D& worldPos)
{
	if (m_state != EEditorState::Dragging) return;

	if (!SelectingActor) return;

	SelectingActor = nullptr;

	m_state = EEditorState::Idle;
}

bool EditorMode::SaveLevel(const std::string& filePath)
{
	// ObjectManager上のアクタを保存
	return LevelSerializer::Save(GetWorld(), filePath);
}

bool EditorMode::LoadLevel(const std::string& filePath)
{
	PendingLoadPath = filePath;
	SceneManager::GetInstance().OpenScene<EditorMode>();
	return true;
}
std::string EditorMode::PendingLoadPath = "";
EditorMode::EditorMode()
{
	M_LOG("EditorMode initialized");

}

void EditorMode::OnUpdate(float DeltaTime)
{
	// ここでUIの更新（構築）を行う
	static EditorUI ui;
	ui.UpdateAndDraw(this);
}

void EditorMode::BeginPlay()
{
	SpawnPlayer<EditorPawn, EditorController>({ 0,0 }, 0);

	if (PendingLoadPath != "")
	{
		LevelSerializer::Load(GetWorld(), PendingLoadPath);
	}
	PendingLoadPath.clear();
}

FVector2D EditorMode::GetMouseWorldPosition() const
{
	int mx, my;
	GetMousePoint(&mx, &my);
	return RenderSystem::GetInstance().ScreenToWorld({ static_cast<float>(mx),
													   static_cast<float>(my) });
}
