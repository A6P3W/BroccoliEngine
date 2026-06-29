#pragma once
#include <string>
#include <vector>

#include "GameModeBase.h"
#include "LevelSerializer.h"
#include "UMath.h"

class AActor;
class EditorUI;
class EditorSelectPointComponent;

enum class EEditorState {
  Idle,
  Dragging,
};
enum class EActorAction { Select, Move, Rotate, Scale };

class EditorMode : public AGameModeBase {
 public:
  DEFINE_ACTOR_CLASS(EditorMode);

  // --- アクタ選択 (クラスブラウザ用) ---
  void SelectClass(const std::string& className) { SelectedClass = className; }
  const std::string& GetSelectedClass() const { return SelectedClass; }
  const std::vector<std::string>& GetClassList() const;
  const std::vector<std::string>& GetGameModeClassList() const;
  void SetSelectedGameModeClass(const std::string& className) { SelectedGameModeClass = className; }
  const std::string& GetSelectedGameModeClass() const { return SelectedGameModeClass; }

  // --- アクタ選択 (インスペクタ・アウトライナ用) ---
  void SetSelectedActor(AActor* actor);
  AActor* GetSelectedActor() const { return SelectedActor; }

  // --- マウス入力（EditorPawnから呼ぶ） ---
  void OnMousePress(const FVector2D& worldPos);    // ドラッグ開始
  void OnMouseMove(const FVector2D& Delta);        // プレビュー位置更新
  void OnMouseRelease(const FVector2D& worldPos);  // 配置確定

  // --- 保存/ロード ---
  bool SaveLevel(const std::string& filePath);
  bool LoadLevel(const std::string& filePath);  // 現シーンをクリアしてからロード
  bool QuickSaveLevel();

  void SetCurrentLevelPath(const std::string& path) { CurrentLevelPath = path; }

  EEditorState GetState() const { return State; }

  EActorAction GetActorAction() const { return ActorAction; }
  void SetActorAction(EActorAction action) { ActorAction = action; }

  void Simulate();

  // --- アクタ操作 ---
  void CopySelectedActor();
  void PasteActor();
  void CutSelectedActor();
  void DeleteSelectedActor();

 public:
  EditorMode();
  void OnUpdate(float DeltaTime) override;

 private:
  EditorMode(const EditorMode&) = delete;
  EditorMode& operator=(const EditorMode&) = delete;
  EditorMode(EditorMode&&) = delete;
  EditorMode& operator=(EditorMode&&) = delete;

  void BeginPlay() override;
  EEditorState State = EEditorState::Idle;
  std::string SelectedClass;
  std::string SelectedGameModeClass;
  AActor* SelectingActor = nullptr;  // ドラッグ中のゴースト
  AActor* SelectedActor = nullptr;   // 選択中のアクタ
  EditorSelectPointComponent* SelectedPointComponent = nullptr;
  EActorAction ActorAction = EActorAction::Select;

  static std::string PendingLoadPath;

  std::string CurrentLevelPath;

  FVector2D GetMouseWorldPosition() const;

  // --- クリップボード ---
  FActorSaveData ClipboardData;
  bool bHasClipboard = false;
};
