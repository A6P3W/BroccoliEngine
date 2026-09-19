#include "EditorPlacementTool.h"

#include "Actor.h"
#include "ActorRegistry.h"
#include "World.h"

AActor* EditorPlacementTool::Place2D(
    World* WorldPtr, const std::string& ClassName, const FVector2D& Position
) const {
  if (WorldPtr == nullptr || ClassName.empty()) {
    return nullptr;
  }

  return ActorRegistry::GetInstance().Spawn(WorldPtr, ClassName, Position);
}

AActor* EditorPlacementTool::Place3D(
    World* WorldPtr, const std::string& ClassName, const FVector3D& Position
) const {
  AActor* Actor = Place2D(WorldPtr, ClassName, FVector2D::ZeroVector());
  if (Actor != nullptr) {
    Actor->SetActorLocation3D(Position);
  }
  return Actor;
}
