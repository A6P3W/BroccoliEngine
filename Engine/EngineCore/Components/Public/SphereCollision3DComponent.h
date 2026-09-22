#pragma once

#include "Collision3DComponent.h"

class BROCCOLI_ENGINE_API MSphereCollision3DComponent : public MCollision3DComponent {
 public:
  DEFINE_ACTOR_COMPONENT_CLASS(MSphereCollision3DComponent)

  ECollisionShape3D GetShapeType3D() const override { return ECollisionShape3D::Sphere; }
  FVector3D GetShapeDimensions3D() const override;
  float GetRadius() const { return Radius; }
  void SetRadius(float NewRadius);

 private:
  float Radius = 0.5f;
};
