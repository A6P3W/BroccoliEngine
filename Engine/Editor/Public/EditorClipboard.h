#pragma once

#include "BroccoliEngineAPI.h"
#include "LevelSerializer.h"
#include "UMath.h"

class AActor;
class World;

class BROCCOLI_ENGINE_API EditorClipboard {
 public:
  bool Copy(AActor* Actor);
  AActor* Paste(World* WorldPtr, const FVector2D& PasteLocation);
  AActor* Paste(World* WorldPtr, const FVector3D& PasteLocation);
  void Clear();

  bool HasData() const { return bHasClipboard; }
  const FActorSaveData& GetData() const { return ClipboardData; }

 private:
  FActorSaveData ClipboardData;
  bool bHasClipboard = false;
};
