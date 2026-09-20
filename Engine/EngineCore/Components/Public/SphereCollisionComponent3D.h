#pragma once

#include "CollisionComponent3D.h"

class BROCCOLI_ENGINE_API MSphereCollisionComponent3D : public MCollisionComponent3D {
 public:
  DEFINE_ACTOR_COMPONENT_CLASS(MSphereCollisionComponent3D)

  ECollisionShape3D GetShapeType3D() const override { return ECollisionShape3D::Sphere; }
  FVector3D GetShapeDimensions3D() const override;
  float GetRadius() const { return Radius; }
  void SetRadius(float NewRadius);

 private:
  float Radius = 0.5f;
};
