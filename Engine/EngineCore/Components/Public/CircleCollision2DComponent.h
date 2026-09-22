#pragma once
#include "BroccoliEngineAPI.h"
#include "Collision2DComponent.h"

class BROCCOLI_ENGINE_API MCircleCollision2DComponent : public MCollision2DComponent {
 public:
  DEFINE_ACTOR_COMPONENT_CLASS(MCircleCollision2DComponent)
  MCircleCollision2DComponent() = default;
  ECollisionShape GetShapeType() const override { return ECollisionShape::Circle; }
  float GetRadius() const { return Radius; };
  void SetRadius(float NewRadius) { Radius = NewRadius; }
  void Draw() override;

  FAABB GetAABB() const override;

 private:
  float Radius = 50.0f;
};
