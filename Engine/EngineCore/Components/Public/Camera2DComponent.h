#pragma once
#include "BroccoliEngineAPI.h"
#include "SceneComponent.h"

class BROCCOLI_ENGINE_API MCamera2DComponent : public MSceneComponent {
 public:
  DEFINE_ACTOR_COMPONENT_CLASS(MCamera2DComponent)
  ~MCamera2DComponent();
  float GetFOV() const { return Fov; }
  void SetFOV(float fov) { Fov = fov; }
  void SetActiveCamera();

 private:
  float Fov = 1.0f;
};
