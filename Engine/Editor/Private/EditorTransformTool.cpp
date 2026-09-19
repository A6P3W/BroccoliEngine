#include "EditorTransformTool.h"

#include "Actor.h"

void EditorTransformTool::Begin(AActor* Actor, EActorAction InAction) {
  TargetActor = Actor;
  Action = InAction;
}

void EditorTransformTool::Update(const FVector2D& Delta, const FVector2D& MouseWorldPosition) {
  if (TargetActor == nullptr || TargetActor->IsPendingDestroy()) {
    End();
    return;
  }

  switch (Action) {
    case EActorAction::Select:
      break;
    case EActorAction::Move:
      TargetActor->SetActorLocation(MouseWorldPosition);
      break;
    case EActorAction::Rotate:
      TargetActor->AddActorRotation(FRotator(Delta.X * 0.25f));
      break;
    case EActorAction::Scale:
      TargetActor->SetActorScale(TargetActor->GetActorScale() * (1.0f + Delta.X * 0.001f));
      break;
  }
}

void EditorTransformTool::End() {
  TargetActor = nullptr;
  Action = EActorAction::Select;
}
