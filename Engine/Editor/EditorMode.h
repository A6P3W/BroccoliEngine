#pragma once
#include "GameModeBase.h"
#include "LevelSerializer.h"
#include "Utils/UMath.h"
#include <string>
#include <vector>
class AActor;

// エディタの操作状態
enum class EEditorState
{
	Idle,       // 何もしていない
	Dragging,   // アクタをドラッグ中（プレビュー表示）
};

class EditorMode : public AGameModeBase
{
public:
	static EditorMode& GetInstance()
	{
		static EditorMode instance;
		return instance;
	}

	// --- アクタ選択 ---
	void SelectClass(const std::string& className) { m_selectedClass = className; }
	const std::string& GetSelectedClass() const { return m_selectedClass; }
	const std::vector<std::string>& GetClassList() const;

	// --- マウス入力（EditorPawnから呼ぶ） ---
	void OnMousePress(const FVector2D& worldPos);   // ドラッグ開始
	void OnMouseMove(const FVector2D& worldPos);    // プレビュー位置更新
	void OnMouseRelease(const FVector2D& worldPos); // 配置確定

	// --- 保存/ロード ---
	bool SaveLevel(const std::string& filePath);
	bool LoadLevel(const std::string& filePath);    // 現シーンをクリアしてからロード

	// --- 配置済みアクタ一覧 ---
	const std::vector<FActorSaveData>& GetPlacedActors() const { return m_placedActors; }

	EEditorState GetState() const { return m_state; }

public:
	EditorMode() = default;

private:
	EditorMode(const EditorMode&) = delete;
	EditorMode& operator=(const EditorMode&) = delete;
	EditorMode(EditorMode&&) = delete;
	EditorMode& operator=(EditorMode&&) = delete;

	EEditorState             m_state = EEditorState::Idle;
	std::string              m_selectedClass;
	AActor*                  m_previewActor = nullptr; // ドラッグ中のゴースト
	std::vector<FActorSaveData> m_placedActors;         // 保存用の記録
};
