#include "EditorMode.h"
#include "ActorRegistry.h"
#include "ObjectManager.h"
#include "Actor.h"
#include "EditorUI.h"
#include "EditorPawn.h"
#include <PlayerController.h>
#include "Log.h"
#include "UMath.h"
#include "World.h"
#include "SceneManager.h"
#include "EditorSelectPointComponent.h"
#include "EditorController.h"
#include <DxLib.h>
#include "RenderSystem.h"
#include "SpriteActor.h"
const std::vector<std::string>& EditorMode::GetClassList() const
{
	return ActorRegistry::GetInstance().GetClassNames();
}

void EditorMode::SetSelectedActor(AActor* actor)
{
	if (SelectedPointComponent != nullptr)
	{
		try {
			SelectedPointComponent->Selected(false);
		}
		catch (...) {}
	}

	m_selectedActor = actor;
	SelectedPointComponent = nullptr; 

	if (m_selectedActor)
	{
		auto components = m_selectedActor->GetComponents<EditorSelectPointComponent>();
		if (!components.empty())
		{
			SelectedPointComponent = components[0];
			SelectedPointComponent->Selected(true);
		}
	}
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
			if (SelectedPointComponent != nullptr)
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
	SetSelectedActor(nullptr);

	if (m_selectedClass.empty()) return;
	if (m_state == EEditorState::Dragging) return;

	// プレビュー用アクタをスポーン
	SelectingActor = ActorRegistry::GetInstance().Spawn(GetWorld(), m_selectedClass, worldPos);
	if (!SelectingActor) return;
	if (SelectedPointComponent != nullptr)
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
void EditorMode::Simulate()
{
	
	GetWorld()->SetSimulating(!GetWorld()->IsSimulating());
}
EditorMode::EditorMode()
{
	M_LOG("EditorMode initialized");
	bEditorActor = true;
}



void EditorMode::CopySelectedActor()
{
	if (!m_selectedActor || m_selectedActor->IsPendingDestroy()) {
		M_LOG("Copy failed: No actor selected.");
		return;
	}

	m_ClipboardData.ClassName = m_selectedActor->GetActorClassName();
	m_ClipboardData.Location = m_selectedActor->GetActorLocation();
	m_ClipboardData.Rotation = m_selectedActor->GetActorRotation().Rotation;
	m_ClipboardData.Scale = m_selectedActor->GetActorScale().Scale;
	m_ClipboardData.CustomProperties.clear();

	if (auto spriteActor = dynamic_cast<ASpriteActor*>(m_selectedActor))
	{
		m_ClipboardData.CustomProperties["ImagePath"] = spriteActor->GetImagePath();
	}

	m_bHasClipboard = true;
	M_LOG("Copied Actor: {} at ({}, {})", m_ClipboardData.ClassName, m_ClipboardData.Location.X, m_ClipboardData.Location.Y);
}

void EditorMode::PasteActor()
{
	if (!m_bHasClipboard) {
		M_LOG("Paste failed: Clipboard is empty.");
		return;
	}

	FVector2D pasteLocation = GetMouseWorldPosition();

	AActor* newActor = ActorRegistry::GetInstance().Spawn(
		GetWorld(), m_ClipboardData.ClassName, pasteLocation, m_ClipboardData.Rotation);

	if (!newActor) {
		M_LOG("Paste failed: Could not spawn actor '{}'.", m_ClipboardData.ClassName);
		return;
	}

	newActor->SetActorScale(m_ClipboardData.Scale);

	if (auto spriteActor = dynamic_cast<ASpriteActor*>(newActor))
	{
		auto it = m_ClipboardData.CustomProperties.find("ImagePath");
		if (it != m_ClipboardData.CustomProperties.end())
		{
			spriteActor->SetImagePath(it->second);
		}
	}

	SetSelectedActor(newActor);

	M_LOG("Pasted Actor: {} at ({}, {})", m_ClipboardData.ClassName, pasteLocation.X, pasteLocation.Y);
}

void EditorMode::CutSelectedActor()
{
	if (!m_selectedActor || m_selectedActor->IsPendingDestroy()) {
		M_LOG("Cut failed: No actor selected.");
		return;
	}

	CopySelectedActor();
	DeleteSelectedActor();

	M_LOG("Cut completed.");
}

void EditorMode::DeleteSelectedActor()
{
	if (!m_selectedActor || m_selectedActor->IsPendingDestroy()) {
		M_LOG("Delete failed: No actor selected.");
		return;
	}
	m_selectedActor->Destroy();
	SetSelectedActor(nullptr);
	M_LOG("Selected actor destroyed.");
}

void EditorMode::OnUpdate(float DeltaTime)
{
	static EditorUI ui;
	ui.UpdateAndDraw(this);
}

void EditorMode::BeginPlay()
{
	GetWorld()->SetSimulating(false);
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
