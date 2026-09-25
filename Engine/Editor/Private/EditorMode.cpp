#include "EditorMode.h"

#include <PlayerController.h>

#include <cmath>

#include "Actor.h"
#include "ActorManager.h"
#include "ActorRegistry.h"
#include "BroccoliRaylib.h"
#include "Camera3DComponent.h"
#include "EditorController.h"
#include "EditorPawn2D.h"
#include "EditorPawn3D.h"
#include "EditorUI.h"
#include "FileDialog.h"
#include "Log.h"
#include "PathResolver.h"
#include "PhysicsSystem3D.h"
#include "RenderSystem.h"
#include "ResourceManager.h"
#include "SceneManager.h"
#include "SpriteActor.h"
#include "StaticMeshActor.h"
#include "StaticMeshComponent.h"
#include "World.h"

namespace {
bool IsSamePickingProxy(const FEditorPickingProxy3D& Left, const FEditorPickingProxy3D& Right) {
  return Left.Shape == Right.Shape && Left.Center == Right.Center &&
         Left.HalfExtent == Right.HalfExtent && Left.Radius == Right.Radius;
}
}  // namespace

const std::vector<std::string>& EditorMode::GetClassList() const {
  return ActorRegistry::GetInstance().GetClassNames();
}

const std::vector<std::string>& EditorMode::GetGameModeClassList() const {
  return ActorRegistry::GetInstance().GetGameModeClassNames();
}

void EditorMode::SetViewportMode(EEditorViewportMode Mode) {
  if (IsThreeDCameraNavigationActive() || ViewportState.Mode == Mode ||
      EditorControllerPtr == nullptr) {
    return;
  }

  APawn* TargetPawn = Mode == EEditorViewportMode::TwoD ? static_cast<APawn*>(EditorPawn2DPtr)
                                                        : static_cast<APawn*>(EditorPawn3DPtr);
  if (TargetPawn == nullptr) {
    return;
  }

  ViewportState.Mode = Mode;
  SetPlayerPawn(TargetPawn);
  EditorControllerPtr->Possess(TargetPawn);
}

bool EditorMode::IsThreeDCameraNavigationActive() const {
  return EditorPawn3DPtr != nullptr && EditorPawn3DPtr->IsCameraNavigationActive();
}

void EditorMode::OnMousePress3D() {}

AActor* EditorMode::PlaceSelectedClassAtViewportCenter() {
  if (SelectedClass.empty()) {
    return nullptr;
  }

  return PlaceActorAtViewportCenter(SelectedClass);
}

AActor* EditorMode::PlaceActorAtViewportCenter(const std::string& ClassName) {
  if (ClassName.empty()) {
    return nullptr;
  }

  AActor* Actor = nullptr;
  if (ViewportState.Mode == EEditorViewportMode::TwoD) {
    if (ViewportState.RenderTargetSize.X <= 0.0f || ViewportState.RenderTargetSize.Y <= 0.0f) {
      return nullptr;
    }

    const FVector2D ViewportCenter = {
        ViewportState.RenderTargetSize.X * 0.5f,
        ViewportState.RenderTargetSize.Y * 0.5f,
    };
    const FVector2D Position = RenderSystem::GetInstance().ScreenToWorld(ViewportCenter);
    Actor = PlacementTool.Place2D(GetWorld(), ClassName, Position);
  } else {
    if (EditorPawn3DPtr == nullptr || EditorPawn3DPtr->GetEditorCamera3D() == nullptr) {
      return nullptr;
    }

    const FVector3D Position = EditorPawn3DPtr->GetActorLocation3D() +
                               EditorPawn3DPtr->GetEditorCamera3D()->GetForwardVector() * 5.0f;
    Actor = PlacementTool.Place3D(GetWorld(), ClassName, Position);
  }

  if (Actor != nullptr) {
    Selection.Select(Actor);
  }
  return Actor;
}

void EditorMode::OnMousePress(const FVector2D& WorldPos) {
  if (AActor* HitActor = Selection.HitTest2D(GetWorld(), WorldPos)) {
    Selection.Select(HitActor);
    SelectingActor = HitActor;
    TransformTool.Begin(SelectingActor, GetActorAction());
    M_LOG(Log, "Hit Actor: {}", HitActor->GetActorClassName());
    M_LOG(Log, "Current Actor Action: {}", static_cast<int>(GetActorAction()));
    State = EEditorState::Dragging;
    return;
  }
  Selection.Clear();

  if (SelectedClass.empty() || State == EEditorState::Dragging) {
    return;
  }

  SelectingActor = PlacementTool.Place2D(GetWorld(), SelectedClass, WorldPos);
  if (SelectingActor == nullptr) {
    return;
  }

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
  SetDefaultPawnClass(EditorPawn2D::StaticClassName());
  SetDefaultPlayerControllerClass(EditorController::StaticClassName());
}

void EditorMode::OnPlayerSpawned(
    APlayerController* Controller, APawn* Pawn, FNetworkConnectionId ConnectionId
) {
  if (ConnectionId != 0) {
    return;
  }

  EditorControllerPtr = dynamic_cast<EditorController*>(Controller);
  EditorPawn2DPtr = dynamic_cast<EditorPawn2D*>(Pawn);
  EditorPawn3DPtr = GetWorld()->SpawnActor<EditorPawn3D>();
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
  RefreshPickingProxies();
  DrawPickingProxies();
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

FEditorPickingProxy3D EditorMode::ResolvePickingProxy(AActor* Actor) const {
  FEditorPickingProxy3D Proxy;
  if (Actor == nullptr) return Proxy;

  Proxy.Center = Actor->GetActorLocation3D();
  auto* StaticMeshActor = dynamic_cast<AStaticMeshActor*>(Actor);
  const auto Meshes = Actor->GetComponents<MStaticMeshComponent>();
  FBox3D Bounds;
  if (StaticMeshActor == nullptr || Meshes.size() != 1 ||
      !ResourceManager::GetInstance().GetModelBounds(Meshes.front()->GetModel(), Bounds)) {
    return Proxy;
  }

  const FVector3D LocalCenter = (Bounds.Min + Bounds.Max) * 0.5f;
  const FVector3D LocalHalfExtent = (Bounds.Max - Bounds.Min) * 0.5f;
  const FTransform3D Transform = Actor->GetActorTransform3D();
  Proxy.Shape = EEditorPickingShape3D::Box;
  Proxy.Center = Transform.TransformPosition(LocalCenter);
  Proxy.HalfExtent = {
      LocalHalfExtent.X * std::abs(Transform.Scale.X),
      LocalHalfExtent.Y * std::abs(Transform.Scale.Y),
      LocalHalfExtent.Z * std::abs(Transform.Scale.Z),
  };
  return Proxy;
}

void EditorMode::RefreshPickingProxies() {
  FPhysicsSystem3D* Physics = GetWorld()->GetPhysicsSystem3D();
  if (Physics == nullptr) return;

  std::unordered_map<AActor*, FEditorPickingProxy3D> CurrentProxies;
  for (const std::unique_ptr<AActor>& ActorOwner : GetWorld()->GetActorManager()->GetAllActors()) {
    AActor* Actor = ActorOwner.get();
    if (Actor == nullptr || Actor->IsPendingDestroy() || Actor->IsEditorActor()) continue;
    FEditorPickingProxy3D Proxy = ResolvePickingProxy(Actor);
    const auto Existing = PickingProxies.find(Actor);
    if (Existing == PickingProxies.end() || !IsSamePickingProxy(Existing->second, Proxy)) {
      Physics->RefreshEditorPickingBody(Actor, Proxy);
    }
    CurrentProxies.emplace(Actor, Proxy);
  }
  for (const auto& [Actor, Proxy] : PickingProxies) {
    if (!CurrentProxies.contains(Actor)) Physics->UnregisterEditorPickingBody(Actor);
  }
  PickingProxies = std::move(CurrentProxies);
}

void EditorMode::DrawPickingProxies() const {
  if (ViewportState.Mode != EEditorViewportMode::ThreeD) return;
  constexpr FColor PickingColor{255, 196, 0, 255};
  for (const auto& [Actor, Proxy] : PickingProxies) {
    if (Proxy.Shape == EEditorPickingShape3D::Sphere) {
      RenderSystem::GetInstance().SubmitSphere(Proxy.Center, Proxy.Radius, PickingColor, false);
    } else if (Actor == HoveredActor || Actor == GetSelectedActor()) {
      RenderSystem::GetInstance().SubmitCube(
          {Proxy.Center,
           Actor->GetActorRotation3D(),
           {Proxy.HalfExtent.X * 2.0f, Proxy.HalfExtent.Y * 2.0f, Proxy.HalfExtent.Z * 2.0f}},
          PickingColor,
          false
      );
    }
  }
}

bool EditorMode::BuildViewportRay(FPhysicsRay3D& OutRay) const {
  if (EditorPawn3DPtr == nullptr || EditorPawn3DPtr->GetEditorCamera3D() == nullptr ||
      ViewportState.RenderTargetSize.X <= 0.0f || ViewportState.RenderTargetSize.Y <= 0.0f) {
    return false;
  }
  FVector2D Mouse;
  if (!TryGetViewportRenderTargetMousePosition(Mouse)) return false;

  MCamera3DComponent* Camera = EditorPawn3DPtr->GetEditorCamera3D();
  const float NormalizedX = Mouse.X * 2.0f / ViewportState.RenderTargetSize.X - 1.0f;
  const float NormalizedY = 1.0f - Mouse.Y * 2.0f / ViewportState.RenderTargetSize.Y;
  const float Aspect = ViewportState.RenderTargetSize.X / ViewportState.RenderTargetSize.Y;
  const FVector3D Forward = Camera->GetForwardVector();
  const FVector3D Right = Camera->GetRightVector();
  const FVector3D Up = Camera->GetUpVector();
  OutRay.Origin = Camera->GetWorldLocation3D();
  if (Camera->GetProjection() == ECameraProjection3D::Perspective) {
    const float HalfHeight = std::tan(UMath::DegToRad(Camera->GetFOV()) * 0.5f);
    OutRay.Direction =
        (Forward + Right * (NormalizedX * HalfHeight * Aspect) + Up * (NormalizedY * HalfHeight))
            .Normalize();
  } else {
    const float HalfHeight = Camera->GetFOV() * 0.5f;
    OutRay.Origin += Right * (NormalizedX * HalfHeight * Aspect) + Up * (NormalizedY * HalfHeight);
    OutRay.Direction = Forward;
  }
  OutRay.MaxDistance = 1000.0f;
  return true;
}
