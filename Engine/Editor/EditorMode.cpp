#include "EditorMode.h"
#include "ActorRegistry.h"
#include "ObjectManager.h"
#include "Actor.h"
#include "EditorUI.h"
#include "EditorPawn.h"
#include <PlayerController.h>
#include "Utils/Log.h"
const std::vector<std::string>& EditorMode::GetClassList() const
{
    return ActorRegistry::GetInstance().GetClassNames();
}

void EditorMode::OnMousePress(const FVector2D& worldPos)
{
    if (m_selectedClass.empty()) return;
    if (m_state == EEditorState::Dragging) return;

    // プレビュー用アクタをスポーン
    m_previewActor = ActorRegistry::GetInstance().Spawn(m_selectedClass, worldPos);
    if (!m_previewActor) return;

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

        // 配置したアクタを選択状態にする
        SetSelectedActor(m_previewActor);

        m_previewActor = nullptr;
    }
    m_state = EEditorState::Idle;
}

bool EditorMode::SaveLevel(const std::string& filePath)
{
    // ObjectManager上のアクタを保存
    return LevelSerializer::Save(filePath);
}

bool EditorMode::LoadLevel(const std::string& filePath)
{
    // 既存アクタを全破棄
    ObjectManager::GetInstance().ClearAllObjects();
    
    m_previewActor = nullptr;
    m_selectedActor = nullptr;
    m_state = EEditorState::Idle;

    // ロードしてスポーン
    return LevelSerializer::Load(filePath);
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

