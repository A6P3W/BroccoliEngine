#pragma once

#include "SceneComponent.h"

enum class ECameraProjection3D { Perspective, Orthographic };

class BROCCOLI_ENGINE_API MCamera3DComponent : public MSceneComponent {
 public:
  DEFINE_ACTOR_COMPONENT_CLASS(MCamera3DComponent)
  ~MCamera3DComponent();
  float GetFOV() const { return Fov; }
  void SetFOV(float Value) { Fov = Value; }
  void SetProjection(ECameraProjection3D Value) { Projection = Value; }
  ECameraProjection3D GetProjection() const { return Projection; }
  FVector3D GetForwardVector() const;
  FVector3D GetUpVector() const;
  FVector3D GetRightVector() const;
  void SetActiveCamera();

 private:
  float Fov = 60.0f;
  ECameraProjection3D Projection = ECameraProjection3D::Perspective;
};
