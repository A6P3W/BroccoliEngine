#pragma once
#include <string>
#include <unordered_map>
#include <vector>

#include "EditorClipboard.h"
#include "EditorPlacementTool.h"
#include "EditorSelection.h"
#include "EditorTransformTool.h"
#include "EditorViewportState.h"
#include "GameModeBase.h"
#include "LevelSerializer.h"
#include "PhysicsQuery3D.h"
#include "UMath.h"

class AActor;
class EditorUI;
class EditorController;
class EditorPawn2D;
class EditorPawn3D;

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
  EditorPawn3D* GetEditorPawn3D() const { return EditorPawn3DPtr; }
  void OnMousePress3D();
  AActor* PlaceSelectedClassAtViewportCenter();
  AActor* PlaceActorAtViewportCenter(const std::string& ClassName);

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
  void OnPlayerSpawned(
      APlayerController* Controller, APawn* Pawn, FNetworkConnectionId ConnectionId
  ) override;
  EEditorState State = EEditorState::Idle;
  std::string SelectedClass;
  std::string SelectedGameModeClass;
  AActor* SelectingActor = nullptr;  // ドラッグ中のゴースト
  EditorController* EditorControllerPtr = nullptr;
  EditorPawn2D* EditorPawn2DPtr = nullptr;
  EditorPawn3D* EditorPawn3DPtr = nullptr;
  EActorAction ActorAction = EActorAction::Select;
  FEditorViewportState ViewportState;
  EditorSelection Selection;
  EditorPlacementTool PlacementTool;
  EditorTransformTool TransformTool;
  EditorClipboard Clipboard;

  static std::string PendingLoadPath;

  std::string CurrentLevelPath;

  bool TryGetMouseWorldPosition(FVector2D& OutPosition, bool RequireInside = true) const;
  FEditorPickingProxy3D ResolvePickingProxy(AActor* Actor) const;
  void RefreshPickingProxies();
  void DrawPickingProxies() const;
  bool BuildViewportRay(FPhysicsRay3D& OutRay) const;
  void UpdateHoveredActor();

  std::unordered_map<AActor*, FEditorPickingProxy3D> PickingProxies;
  AActor* HoveredActor = nullptr;
};
