#include "EditorSelection.h"

#include "Actor.h"
#include "ActorManager.h"
#include "EditorSelectPointComponent.h"
#include "World.h"

AActor* EditorSelection::HitTest2D(World* WorldPtr, const FVector2D& WorldPosition) const {
  if (WorldPtr == nullptr) {
    return nullptr;
  }

  constexpr float SelectionRadiusSquared = 15.0f * 15.0f;
  const auto& Actors = WorldPtr->GetActorManager()->GetAllActors();
  for (const auto& Actor : Actors) {
    if (Actor->IsPendingDestroy() || Actor->IsEditorActor()) {
      continue;
    }

    const FVector2D ActorPosition = Actor->GetActorLocation();
    const FVector2D Offset{
        ActorPosition.X - WorldPosition.X,
        ActorPosition.Y - WorldPosition.Y,
    };
    if (Offset.SizeSquared() <= SelectionRadiusSquared) {
      return Actor.get();
    }
  }

  return nullptr;
}

void EditorSelection::Select(AActor* Actor) {
  if (SelectedPointComponent != nullptr) {
    try {
      SelectedPointComponent->Selected(false);
    } catch (...) {
    }
  }

  SelectedActor = Actor;
  SelectedPointComponent = nullptr;
  if (SelectedActor == nullptr) {
    return;
  }

  const auto Components = SelectedActor->GetComponents<EditorSelectPointComponent>();
  if (Components.empty()) {
    return;
  }

  SelectedPointComponent = Components[0];
  SelectedPointComponent->Selected(true);
}

void EditorSelection::Clear() { Select(nullptr); }
