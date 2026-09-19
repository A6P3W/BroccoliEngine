#include "EditorClipboard.h"

#include "Actor.h"
#include "ActorRegistry.h"
#include "Log.h"
#include "SpriteActor.h"
#include "StaticMeshActor.h"
#include "World.h"

bool EditorClipboard::Copy(AActor* Actor) {
  if (Actor == nullptr || Actor->IsPendingDestroy()) {
    M_LOG(Log, "Copy failed: No actor selected.");
    return false;
  }

  ClipboardData.ClassName = Actor->GetActorClassName();
  ClipboardData.Transform = Actor->GetActorTransform3D();
  ClipboardData.CustomProperties.clear();

  if (auto* SpriteActor = dynamic_cast<ASpriteActor*>(Actor)) {
    ClipboardData.CustomProperties["ImagePath"] = SpriteActor->GetImagePath();
  }
  if (auto* StaticMeshActor = dynamic_cast<AStaticMeshActor*>(Actor)) {
    ClipboardData.CustomProperties["ModelPath"] = StaticMeshActor->GetModelPath();
  }

  bHasClipboard = true;
  M_LOG(
      Log,
      "Copied Actor: {} at ({}, {})",
      ClipboardData.ClassName,
      ClipboardData.Transform.Location.X,
      ClipboardData.Transform.Location.Y
  );
  return true;
}

AActor* EditorClipboard::Paste(World* WorldPtr, const FVector2D& PasteLocation) {
  return Paste(
      WorldPtr, FVector3D{PasteLocation.X, PasteLocation.Y, ClipboardData.Transform.Location.Z}
  );
}

AActor* EditorClipboard::Paste(World* WorldPtr, const FVector3D& PasteLocation) {
  if (!bHasClipboard) {
    M_LOG(Log, "Paste failed: Clipboard is empty.");
    return nullptr;
  }
  if (WorldPtr == nullptr) {
    M_LOG(Log, "Paste failed: World is null.");
    return nullptr;
  }

  AActor* NewActor = ActorRegistry::GetInstance().Spawn(WorldPtr, ClipboardData.ClassName);
  if (NewActor == nullptr) {
    M_LOG(Log, "Paste failed: Could not spawn actor '{}'.", ClipboardData.ClassName);
    return nullptr;
  }

  ClipboardData.Transform.Location = PasteLocation;
  NewActor->SetActorLocation3D(ClipboardData.Transform.Location);
  NewActor->SetActorRotation3D(ClipboardData.Transform.Rotation);
  NewActor->SetActorScale3D(ClipboardData.Transform.Scale);

  if (auto* SpriteActor = dynamic_cast<ASpriteActor*>(NewActor)) {
    auto It = ClipboardData.CustomProperties.find("ImagePath");
    if (It != ClipboardData.CustomProperties.end()) {
      SpriteActor->SetImagePath(It->second);
    }
  }
  if (auto* StaticMeshActor = dynamic_cast<AStaticMeshActor*>(NewActor)) {
    auto It = ClipboardData.CustomProperties.find("ModelPath");
    if (It != ClipboardData.CustomProperties.end()) {
      StaticMeshActor->SetModelPath(It->second);
    }
  }

  M_LOG(
      Log,
      "Pasted Actor: {} at ({}, {}, {})",
      ClipboardData.ClassName,
      PasteLocation.X,
      PasteLocation.Y,
      PasteLocation.Z
  );
  return NewActor;
}

void EditorClipboard::Clear() {
  ClipboardData = FActorSaveData{};
  bHasClipboard = false;
}
