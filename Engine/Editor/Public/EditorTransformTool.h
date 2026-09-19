#pragma once

#include "UMath.h"

class AActor;

enum class EActorAction { Select, Move, Rotate, Scale };

class EditorTransformTool {
 public:
  void Begin(AActor* Actor, EActorAction Action);
  void Update(const FVector2D& Delta, const FVector2D& MouseWorldPosition);
  void End();

  bool IsActive() const { return TargetActor != nullptr; }

 private:
  AActor* TargetActor = nullptr;
  EActorAction Action = EActorAction::Select;
};
