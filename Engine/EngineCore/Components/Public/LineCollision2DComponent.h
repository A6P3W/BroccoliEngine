#pragma once
#include "BroccoliEngineAPI.h"
#include "Collision2DComponent.h"

class BROCCOLI_ENGINE_API MLineCollision2DComponent : public MCollision2DComponent {
 public:
  DEFINE_ACTOR_COMPONENT_CLASS(MLineCollision2DComponent)
  MLineCollision2DComponent() = default;

  ECollisionShape GetShapeType() const override { return ECollisionShape::Line; }

  FVector2D GetLocalStart() const { return LocalStart; }
  FVector2D GetLocalEnd() const { return LocalEnd; }
  void SetLine(const FVector2D& NewLocalStart, const FVector2D& NewLocalEnd) {
    LocalStart = NewLocalStart;
    LocalEnd = NewLocalEnd;
  }

  FVector2D GetWorldStart() const {
    return GetWorldLocation() + LocalStart.RotateVector(GetWorldRotation()) * GetWorldScale();
  }
  FVector2D GetWorldEnd() const {
    return GetWorldLocation() + LocalEnd.RotateVector(GetWorldRotation()) * GetWorldScale();
  }

  void Draw() override;
  FAABB GetAABB() const override;

 private:
  FVector2D LocalStart = FVector2D::ZeroVector();
  FVector2D LocalEnd = FVector2D(100.0f, 0.0f);
};
