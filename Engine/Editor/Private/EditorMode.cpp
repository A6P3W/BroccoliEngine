#include "EditorMode.h"

#include <PlayerController.h>

#include "Actor.h"
#include "ActorManager.h"
#include "ActorRegistry.h"
#include "EditorController.h"
#include "EditorPawn.h"
#include "EditorSelectPointComponent.h"
#include "EditorUI.h"
#include "FileDialog.h"
#include "InputManager.h"
#include "Log.h"
#include "MouseDevice.h"
#include "PathResolver.h"
#include "RenderSystem.h"
#include "SceneManager.h"
#include "SpriteActor.h"
#include "UMath.h"
#include "World.h"
const std::vector<std::string>& EditorMode::GetClassList() const {
  return ActorRegistry::GetInstance().GetClassNames();
}

const std::vector<std::string>& EditorMode::GetGameModeClassList() const {
  return ActorRegistry::GetInstance().GetGameModeClassNames();
}

void EditorMode::SetSelectedActor(AActor* actor) {
  if (SelectedPointComponent != nullptr) {
    try {
      SelectedPointComponent->Selected(false);
    } catch (...) {
    }
  }

  SelectedActor = actor;
  SelectedPointComponent = nullptr;

  if (SelectedActor) {
    auto components = SelectedActor->GetComponents<EditorSelectPointComponent>();
    if (!components.empty()) {
      SelectedPointComponent = components[0];
      SelectedPointComponent->Selected(true);
    }
  }
}

void EditorMode::OnMousePress(const FVector2D& worldPos) {
  const auto& actors = GetWorld()->GetActorManager()->GetAllActors();
  AActor* hitActor = nullptr;

  for (auto& actor : actors) {
    FVector2D actorPos = actor->GetActorLocation();
    float distanceSq = FVector2D({actorPos.X - worldPos.X, actorPos.Y - worldPos.Y}).SizeSquared();
    const float selectionRadiusSq = 15.0f * 15.0f;
    if (distanceSq <= selectionRadiusSq) {
      hitActor = actor.get();
      SetSelectedActor(hitActor);
      if (SelectedPointComponent != nullptr) {
        SelectedPointComponent->Selected(false);
      }
      SelectingActor = hitActor;
      SelectedPointComponent = hitActor->GetComponents<EditorSelectPointComponent>()[0];
      SelectedPointComponent->Selected(true);
      M_LOG("Hit Actor: {}", hitActor->GetActorClassName());
      M_LOG("Current Actor Action: {}", (int)GetActorAction());
      State = EEditorState::Dragging;
      return;
    }
  }
  SetSelectedActor(nullptr);

  if (SelectedClass.empty()) return;
  if (State == EEditorState::Dragging) return;

  // プレビュー用アクタをスポーン
  SelectingActor = ActorRegistry::GetInstance().Spawn(GetWorld(), SelectedClass, worldPos);
  if (!SelectingActor) return;
  if (SelectedPointComponent != nullptr) {
    SelectedPointComponent->Selected(false);
  }
  SelectedPointComponent = SelectingActor->GetComponents<EditorSelectPointComponent>()[0];
  SelectedPointComponent->Selected(true);

  SetSelectedActor(SelectingActor);

  State = EEditorState::Dragging;
}

void EditorMode::OnMouseMove(const FVector2D& Delta) {
  if (State != EEditorState::Dragging) return;
  if (!SelectingActor) return;

  switch (GetActorAction()) {
    case EActorAction::Select:
      break;
    case EActorAction::Move:
      SelectingActor->SetActorLocation(GetMouseWorldPosition());
      break;
    case EActorAction::Rotate:
      SelectingActor->AddActorRotation(FRotator(Delta.X * 0.25f));
      break;
    case EActorAction::Scale:
      FScale NewScale = SelectingActor->GetActorScale() * (1 + Delta.X * 0.001f);
      SelectingActor->SetActorScale(NewScale);
      break;
  }
}

void EditorMode::OnMouseRelease(const FVector2D& worldPos) {
  if (State != EEditorState::Dragging) return;

  if (!SelectingActor) return;

  SelectingActor = nullptr;

  State = EEditorState::Idle;
}

bool EditorMode::SaveLevel(const std::string& filePath) {
  if (LevelSerializer::Save(GetWorld(), filePath, SelectedGameModeClass)) {
    CurrentLevelPath = filePath;
    M_LOG("Level saved to '{}'", filePath);

  } else {
    M_LOG("Failed to save level to '{}'", filePath);
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
  M_LOG("EditorMode initialized");
  bEditorActor = true;
  SetDefaultPawnClass(EditorPawn::StaticClassName());
  SetDefaultPlayerControllerClass(EditorController::StaticClassName());
}

void EditorMode::CopySelectedActor() {
  if (!SelectedActor || SelectedActor->IsPendingDestroy()) {
    M_LOG("Copy failed: No actor selected.");
    return;
  }

  ClipboardData.ClassName = SelectedActor->GetActorClassName();
  ClipboardData.Location = SelectedActor->GetActorLocation();
  ClipboardData.Rotation = SelectedActor->GetActorRotation();
  ClipboardData.Scale = SelectedActor->GetActorScale();
  ClipboardData.CustomProperties.clear();

  if (auto spriteActor = dynamic_cast<ASpriteActor*>(SelectedActor)) {
    ClipboardData.CustomProperties["ImagePath"] = spriteActor->GetImagePath();
  }

  bHasClipboard = true;
  M_LOG(
      "Copied Actor: {} at ({}, {})",
      ClipboardData.ClassName,
      ClipboardData.Location.X,
      ClipboardData.Location.Y
  );
}

void EditorMode::PasteActor() {
  if (!bHasClipboard) {
    M_LOG("Paste failed: Clipboard is empty.");
    return;
  }

  FVector2D pasteLocation = GetMouseWorldPosition();

  AActor* newActor = ActorRegistry::GetInstance().Spawn(
      GetWorld(), ClipboardData.ClassName, pasteLocation, ClipboardData.Rotation
  );

  if (!newActor) {
    M_LOG("Paste failed: Could not spawn actor '{}'.", ClipboardData.ClassName);
    return;
  }

  newActor->SetActorScale(ClipboardData.Scale);

  if (auto spriteActor = dynamic_cast<ASpriteActor*>(newActor)) {
    auto it = ClipboardData.CustomProperties.find("ImagePath");
    if (it != ClipboardData.CustomProperties.end()) {
      spriteActor->SetImagePath(it->second);
    }
  }

  SetSelectedActor(newActor);

  M_LOG("Pasted Actor: {} at ({}, {})", ClipboardData.ClassName, pasteLocation.X, pasteLocation.Y);
}

void EditorMode::CutSelectedActor() {
  if (!SelectedActor || SelectedActor->IsPendingDestroy()) {
    M_LOG("Cut failed: No actor selected.");
    return;
  }

  CopySelectedActor();
  DeleteSelectedActor();

  M_LOG("Cut completed.");
}

void EditorMode::DeleteSelectedActor() {
  if (!SelectedActor || SelectedActor->IsPendingDestroy()) {
    M_LOG("Delete failed: No actor selected.");
    return;
  }
  SelectedActor->Destroy();
  SetSelectedActor(nullptr);
  M_LOG("Selected actor destroyed.");
}

void EditorMode::OnUpdate(float DeltaTime) {
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

FVector2D EditorMode::GetMouseWorldPosition() const {
  const MouseDevice* Mouse = InputManager::GetInstance().GetDevice<MouseDevice>();
  if (Mouse == nullptr) return FVector2D::ZeroVector();
  return RenderSystem::GetInstance().ScreenToWorld(
      {static_cast<float>(Mouse->GetMouseX()), static_cast<float>(Mouse->GetMouseY())}
  );
}
