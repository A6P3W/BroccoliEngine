#pragma once
#include "SceneComponent.h"

class MCameraComponent : public MSceneComponent {
 public:
  ~MCameraComponent();
  float GetFOV() const { return Fov; }
  void SetFOV(float fov) { Fov = fov; }
  void SetActiveCamera();

 private:
  float Fov = 1.0f;
};
