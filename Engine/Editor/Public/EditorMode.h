#pragma once
#include <string>
#include <vector>

#include "EditorClipboard.h"
#include "EditorSelection.h"
#include "EditorTransformTool.h"
#include "EditorViewportState.h"
#include "GameModeBase.h"
#include "LevelSerializer.h"
#include "UMath.h"

class AActor;
class EditorUI;
class EditorPawn;

enum class EEditorState {
  Idle,
  Dragging,
};

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
  void SetSelectedActor(AActor* Actor) { Selection.Select(Actor); }
  AActor* GetSelectedActor() const { return Selection.GetSelectedActor(); }

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

  EEditorViewportMode GetViewportMode() const { return ViewportState.Mode; }
  void SetViewportMode(EEditorViewportMode Mode);
  bool IsThreeDCameraNavigationActive() const;
  void SetEditorPawn(EditorPawn* Pawn) { EditorPawnPtr = Pawn; }
  EditorPawn* GetEditorPawn() const { return EditorPawnPtr; }

  FEditorViewportState& GetViewportState() { return ViewportState; }
  const FEditorViewportState& GetViewportState() const { return ViewportState; }
  void SetViewportRenderTexture(void* RenderTexture, int Width, int Height) {
    ViewportState.RenderTexture = RenderTexture;
    ViewportState.RenderTargetSize = {static_cast<float>(Width), static_cast<float>(Height)};
  }
  bool IsViewportInputAvailable() const;
  bool TryGetViewportRenderTargetMousePosition(
      FVector2D& OutPosition, bool RequireInside = true
  ) const;

  void Simulate();

  // --- アクタ操作 ---
  void CopySelectedActor();
  void PasteActor();
  void CutSelectedActor();
  void DeleteSelectedActor();

  const EditorClipboard& GetClipboard() const { return Clipboard; }
  EditorClipboard& GetClipboard() { return Clipboard; }

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
  EditorPawn* EditorPawnPtr = nullptr;
  EActorAction ActorAction = EActorAction::Select;
  FEditorViewportState ViewportState;
  EditorSelection Selection;
  EditorTransformTool TransformTool;
  EditorClipboard Clipboard;

  static std::string PendingLoadPath;

  std::string CurrentLevelPath;

  bool TryGetMouseWorldPosition(FVector2D& OutPosition, bool RequireInside = true) const;
};
