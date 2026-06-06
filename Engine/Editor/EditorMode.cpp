#include "EditorMode.h"
#include "ActorRegistry.h"
#include "ObjectManager.h"
#include "Actor.h"
#include "EditorUI.h"
#include "EditorPawn.h"
#include <PlayerController.h>
#include "Utils/Log.h"
#include "World.h"
#include "SceneManager.h"
#include "EditorSelectPointComponent.h"
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
			if(SelectedPointComponent)
			{
				SelectedPointComponent->Selected(false);
			}
			SelectedPointComponent = hitActor->GetComponents<EditorSelectPointComponent>()[0];
			SelectedPointComponent->Selected(true);
			M_LOG("Hit Actor: {}", hitActor->GetActorClassName());
			return;

		}
	}


	if (m_selectedClass.empty()) return;
	if (m_state == EEditorState::Dragging) return;

	// プレビュー用アクタをスポーン
	m_previewActor = ActorRegistry::GetInstance().Spawn(GetWorld(), m_selectedClass, worldPos);
	if (!m_previewActor) return;
	if (SelectedPointComponent)
	{
		SelectedPointComponent->Selected(false);
	}
	SelectedPointComponent = m_previewActor->GetComponents<EditorSelectPointComponent>()[0];
	SelectedPointComponent->Selected(true);

	SetSelectedActor(m_previewActor);

	m_state = EEditorState::Dragging;
}

void EditorMode::OnMouseMove(const FVector2D& worldPos)
{
	if (m_state != EEditorState::Dragging) return;
	if (!m_previewActor) return;

	// プレビューアクタをマウス位置に追従
	m_previewActor->SetActorLocation(worldPos);
}

void EditorMode::OnMouseRelease(const FVector2D& worldPos)
{
	if (m_state != EEditorState::Dragging) return;

	if (m_previewActor)
	{
		m_previewActor->SetActorLocation(worldPos);

		m_previewActor = nullptr;
	}
	m_state = EEditorState::Idle;
}

bool EditorMode::SaveLevel(const std::string& filePath)
{
	// ObjectManager上のアクタを保存
	return LevelSerializer::Save(GetWorld(), filePath);
}

bool EditorMode::LoadLevel(const std::string& filePath)
{
	SceneManager::GetInstance().OpenScene<EditorMode>();
	return LevelSerializer::Load(GetWorld(), filePath);
}

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
	SpawnPlayer<EditorPawn, APlayerController>({ 0,0 }, 0);
}

