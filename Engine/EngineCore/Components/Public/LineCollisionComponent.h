#pragma once
#include "CollisionComponent.h"

class MLineCollisionComponent : public MCollisionComponent {
 public:
  MLineCollisionComponent(
      const FVector2D& localStart = FVector2D::ZeroVector,
      const FVector2D& localEnd = {100.0f, 0.0f}
  )
      : LocalStart(localStart), LocalEnd(localEnd) {}

  ECollisionShape GetShapeType() const override { return ECollisionShape::Line; }

  FVector2D GetLocalStart() const { return LocalStart; }
  FVector2D GetLocalEnd() const { return LocalEnd; }

  FVector2D GetWorldStart() const {
    return GetWorldLocation() + LocalStart.RotateVector(GetWorldRotation()) * GetWorldScale();
  }
  FVector2D GetWorldEnd() const {
    return GetWorldLocation() + LocalEnd.RotateVector(GetWorldRotation()) * GetWorldScale();
  }

  void Draw() override;
  FAABB GetAABB() const override;

 private:
  FVector2D LocalStart;
  FVector2D LocalEnd;
};
