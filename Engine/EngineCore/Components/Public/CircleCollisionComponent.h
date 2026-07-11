#pragma once
#include "CollisionComponent.h"

class MCircleCollisionComponent : public MCollisionComponent {
 public:
  MCircleCollisionComponent() = default;
  ECollisionShape GetShapeType() const override { return ECollisionShape::Circle; }
  float GetRadius() const { return Radius; };
  void SetRadius(float NewRadius) { Radius = NewRadius; }
  void Draw() override;

  FAABB GetAABB() const override;

 private:
  float Radius = 50.0f;
};
