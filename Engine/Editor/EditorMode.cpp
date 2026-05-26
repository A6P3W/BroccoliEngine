#include "EditorMode.h"
#include "ActorRegistry.h"
#include "ObjectManager.h"
#include "Actor.h"

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

        // 配置記録に追加
        FActorSaveData data;
        data.ClassName = m_selectedClass;
        data.Location = worldPos;
        data.Rotation = m_previewActor->GetActorRotation().Rotation;
        data.Scale = m_previewActor->GetActorScale().Scale;
        m_placedActors.push_back(data);

        m_previewActor = nullptr;
    }

    m_state = EEditorState::Idle;
}

bool EditorMode::SaveLevel(const std::string& filePath)
{
    return LevelSerializer::SaveData(filePath, m_placedActors);
}

bool EditorMode::LoadLevel(const std::string& filePath)
{
    // 既存アクタを全破棄
    ObjectManager::GetInstance().ClearAllObjects();
    m_placedActors.clear();
    m_previewActor = nullptr;
    m_state = EEditorState::Idle;

    // ロードしてスポーン
    std::vector<FActorSaveData> loaded;
    if (!LevelSerializer::LoadData(filePath, loaded)) return false;

    auto& registry = ActorRegistry::GetInstance();
    for (const auto& data : loaded)
    {
        AActor* actor = registry.Spawn(data.ClassName, data.Location, data.Rotation);
        if (actor) actor->SetActorScale(data.Scale);
        m_placedActors.push_back(data);
    }
    return true;
}
