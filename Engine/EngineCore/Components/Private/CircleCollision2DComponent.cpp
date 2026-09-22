#include "CircleCollision2DComponent.h"

#include <RenderSystem.h>

#include "EngineDefine.h"
void MCircleCollision2DComponent::Draw() {
  if (!IsDebug) return;
  RenderSystem::GetInstance().SubmitCircle(
      GetWorldLocation(),
      Radius * GetWorldScale().Scale,
      FColor{0, 255, 0},
      0,
      RenderSpace::World,
      100
  );
  RenderSystem::GetInstance().SubmitCircle(
      GetWorldLocation(),
      Radius * GetWorldScale().Scale,
      FColor{169, 138, 199, 50},
      1,
      RenderSpace::World,
      100
  );
}

FAABB MCircleCollision2DComponent::GetAABB() const {
  FVector2D center = GetWorldLocation();
  float radius = Radius * GetWorldScale().Scale;

  FAABB aabb;
  aabb.MinX = center.X - radius;
  aabb.MinY = center.Y - radius;
  aabb.MaxX = center.X + radius;
  aabb.MaxY = center.Y + radius;
  return aabb;
}
