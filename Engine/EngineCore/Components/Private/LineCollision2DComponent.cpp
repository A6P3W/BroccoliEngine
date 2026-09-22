#include "LineCollision2DComponent.h"

#include <RenderSystem.h>

#include "EngineDefine.h"

void MLineCollision2DComponent::Draw() {
  if (!IsDebug) return;

  RenderSystem::GetInstance().SubmitLine(
      GetWorldStart(), GetWorldEnd(), FColor{0, 255, 0, 120}, RenderSpace::World, 100
  );
}

FAABB MLineCollision2DComponent::GetAABB() const {
  FVector2D start = GetWorldStart();
  FVector2D end = GetWorldEnd();

  FAABB aabb;
  aabb.MinX = (start.X < end.X) ? start.X : end.X;
  aabb.MinY = (start.Y < end.Y) ? start.Y : end.Y;
  aabb.MaxX = (start.X > end.X) ? start.X : end.X;
  aabb.MaxY = (start.Y > end.Y) ? start.Y : end.Y;
  return aabb;
}
