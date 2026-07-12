#pragma once
#include "BroccoliEngineAPI.h"
#include "SceneComponent.h"

class BROCCOLI_ENGINE_API MCameraComponent : public MSceneComponent {
 public:
  ~MCameraComponent();
  float GetFOV() const { return Fov; }
  void SetFOV(float fov) { Fov = fov; }
  void SetActiveCamera();

 private:
  float Fov = 1.0f;
};
