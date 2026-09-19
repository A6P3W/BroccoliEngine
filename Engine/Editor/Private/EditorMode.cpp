#include "EditorMode.h"

#include <PlayerController.h>

#include "Actor.h"
#include "ActorRegistry.h"
#include "BroccoliRaylib.h"
#include "EditorController.h"
#include "EditorPawn.h"
#include "EditorUI.h"
#include "FileDialog.h"
#include "Log.h"
#include "PathResolver.h"
#include "RenderSystem.h"
#include "SceneManager.h"
#include "SpriteActor.h"
#include "StaticMeshActor.h"
#include "World.h"
const std::vector<std::string>& EditorMode::GetClassList() const {
  return ActorRegistry::GetInstance().GetClassNames();
}

const std::vector<std::string>& EditorMode::GetGameModeClassList() const {
  return ActorRegistry::GetInstance().GetGameModeClassNames();
}

void EditorMode::SetViewportMode(EEditorViewportMode Mode) {
  if (IsThreeDCameraNavigationActive()) return;
  if (ViewportState.Mode == Mode) return;

  if (EditorPawnPtr != nullptr) EditorPawnPtr->EndThreeDCameraNavigation();
  ViewportState.Mode = Mode;
  if (Mode == EEditorViewportMode::ThreeD && EditorPawnPtr != nullptr) {
    EditorPawnPtr->SetEditorCamera3DActive();
  } else {
    RenderSystem::GetInstance().SetCameraView3D(nullptr);
  }
}

bool EditorMode::IsThreeDCameraNavigationActive() const {
  return EditorPawnPtr != nullptr && EditorPawnPtr->IsThreeDCameraNavigationActive();
}

void EditorMode::OnMousePress(const FVector2D& worldPos) {
  if (AActor* HitActor = Selection.HitTest2D(GetWorld(), worldPos)) {
    Selection.Select(HitActor);
    SelectingActor = HitActor;
    TransformTool.Begin(SelectingActor, GetActorAction());
    M_LOG(Log, "Hit Actor: {}", HitActor->GetActorClassName());
    M_LOG(Log, "Current Actor Action: {}", static_cast<int>(GetActorAction()));
    State = EEditorState::Dragging;
    return;
  }
  Selection.Clear();

  if (SelectedClass.empty()) return;
  if (State == EEditorState::Dragging) return;

  // プレビュー用アクタをスポーン
  SelectingActor = ActorRegistry::GetInstance().Spawn(GetWorld(), SelectedClass, worldPos);
  if (!SelectingActor) return;
  Selection.Select(SelectingActor);
  TransformTool.Begin(SelectingActor, GetActorAction());

  State = EEditorState::Dragging;
}

void EditorMode::OnMouseMove(const FVector2D& Delta) {
  if (State != EEditorState::Dragging) return;
  if (!TransformTool.IsActive()) return;

  FVector2D MouseWorldPosition;
  if (!TryGetMouseWorldPosition(MouseWorldPosition, false) &&
      GetActorAction() == EActorAction::Move) {
    return;
  }
  TransformTool.Update(Delta, MouseWorldPosition);
}

void EditorMode::OnMouseRelease(const FVector2D& worldPos) {
  if (State != EEditorState::Dragging) return;

  SelectingActor = nullptr;
  TransformTool.End();
  State = EEditorState::Idle;
}

bool EditorMode::SaveLevel(const std::string& filePath) {
  if (LevelSerializer::Save(GetWorld(), filePath, SelectedGameModeClass)) {
    CurrentLevelPath = filePath;
    M_LOG(Log, "Level saved to '{}'", filePath);

  } else {
    M_LOG(Log, "Failed to save level to '{}'", filePath);
    return false;
  }
  return true;
}

bool EditorMode::LoadLevel(const std::string& filePath) {
  PendingLoadPath = filePath;
  SceneManager::GetInstance().OpenGameMode<EditorMode>();
  return true;
}
bool EditorMode::QuickSaveLevel() {
  if (CurrentLevelPath.empty()) {
    const std::string FilePath = FileDialog::SaveFile(
        "Broccoli Level JSON (*.BLevel.json)\0*.BLevel.json\0All Files (*.*)\0*.*\0",
        "BLevel.json",
        PathResolver::GetGameResourceDir()
    );
    if (FilePath.empty()) {
      return false;
    }
    std::string PathStr = FilePath;
    std::string LowerPath = PathStr;
    std::transform(
        LowerPath.begin(), LowerPath.end(), LowerPath.begin(), [](unsigned char Character) {
          return static_cast<char>(std::tolower(Character));
        }
    );
    const std::string JsonSuffix = ".blevel.json";
    const std::string BLevelSuffix = ".blevel";
    if (LowerPath.size() >= JsonSuffix.size() &&
        LowerPath.compare(LowerPath.size() - JsonSuffix.size(), JsonSuffix.size(), JsonSuffix) ==
            0) {
      PathStr = PathStr.substr(0, PathStr.size() - JsonSuffix.size()) + ".BLevel.json";
    } else if (
        LowerPath.size() >= BLevelSuffix.size() &&
        LowerPath.compare(
            LowerPath.size() - BLevelSuffix.size(), BLevelSuffix.size(), BLevelSuffix
        ) == 0
    ) {
      PathStr = PathStr.substr(0, PathStr.size() - BLevelSuffix.size()) + ".BLevel.json";
    } else {
      PathStr += ".BLevel.json";
    }
    return SaveLevel(PathStr);
  }
  return SaveLevel(CurrentLevelPath);
}
std::string EditorMode::PendingLoadPath = "";
void EditorMode::Simulate() { GetWorld()->SetSimulating(!GetWorld()->IsSimulating()); }
EditorMode::EditorMode() {
  const auto& gameModes = ActorRegistry::GetInstance().GetGameModeClassNames();
  if (!gameModes.empty()) {
    SelectedGameModeClass = gameModes[0];
  }
  M_LOG(Log, "EditorMode initialized");
  bEditorActor = true;
  SetDefaultPawnClass(EditorPawn::StaticClassName());
  SetDefaultPlayerControllerClass(EditorController::StaticClassName());
}

void EditorMode::CopySelectedActor() {
  AActor* SelectedActor = GetSelectedActor();
  Clipboard.Copy(SelectedActor);
}

void EditorMode::PasteActor() {
  if (!Clipboard.HasData()) {
    M_LOG(Log, "Paste failed: Clipboard is empty.");
    return;
  }

  FVector2D PasteLocation;
  if (!TryGetMouseWorldPosition(PasteLocation)) {
    M_LOG(Log, "Paste failed: Mouse is outside the editor viewport.");
    return;
  }

  AActor* NewActor = Clipboard.Paste(GetWorld(), PasteLocation);
  if (NewActor != nullptr) {
    SetSelectedActor(NewActor);
  }
}

void EditorMode::CutSelectedActor() {
  AActor* SelectedActor = GetSelectedActor();
  if (SelectedActor == nullptr || SelectedActor->IsPendingDestroy()) {
    M_LOG(Log, "Cut failed: No actor selected.");
    return;
  }

  if (Clipboard.Copy(SelectedActor)) {
    SelectedActor->Destroy();
    SetSelectedActor(nullptr);
    M_LOG(Log, "Cut completed.");
  }
}

void EditorMode::DeleteSelectedActor() {
  AActor* SelectedActor = GetSelectedActor();
  if (SelectedActor == nullptr || SelectedActor->IsPendingDestroy()) {
    M_LOG(Log, "Delete failed: No actor selected.");
    return;
  }
  SelectedActor->Destroy();
  SetSelectedActor(nullptr);
  M_LOG(Log, "Selected actor destroyed.");
}

void EditorMode::OnUpdate(float DeltaTime) {
  (void)DeltaTime;
  static EditorUI ui;
  ui.UpdateAndDraw(this);
}

void EditorMode::BeginPlay() {
  GetWorld()->SetSimulating(false);

  if (PendingLoadPath != "") {
    FLevelMetaData meta;
    if (LevelSerializer::Load(GetWorld(), PendingLoadPath, false, &meta)) {
      if (!meta.GameModeClassName.empty()) {
        SelectedGameModeClass = meta.GameModeClassName;
      }
      CurrentLevelPath = PendingLoadPath;
    }
  }
  PendingLoadPath.clear();
}

bool EditorMode::IsViewportInputAvailable() const {
  const Vector2 MousePosition = GetMousePosition();
  const FVector2D ScreenPosition{MousePosition.x, MousePosition.y};
  return ViewportState.Hovered && ViewportState.ContainsScreenPoint(ScreenPosition);
}

bool EditorMode::TryGetViewportRenderTargetMousePosition(
    FVector2D& OutPosition, bool RequireInside
) const {
  const Vector2 MousePosition = GetMousePosition();
  return ViewportState.ScreenToRenderTarget(
      {MousePosition.x, MousePosition.y}, OutPosition, RequireInside
  );
}

bool EditorMode::TryGetMouseWorldPosition(FVector2D& OutPosition, bool RequireInside) const {
  FVector2D RenderTargetPosition;
  if (!TryGetViewportRenderTargetMousePosition(RenderTargetPosition, RequireInside)) return false;
  OutPosition = RenderSystem::GetInstance().ScreenToWorld(RenderTargetPosition);
  return true;
}
