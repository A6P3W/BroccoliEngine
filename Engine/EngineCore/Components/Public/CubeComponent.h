#pragma once

#include "Color.h"
#include "SceneComponent.h"

class BROCCOLI_ENGINE_API MCubeComponent : public MSceneComponent {
 public:
  DEFINE_ACTOR_COMPONENT_CLASS(MCubeComponent)
  void Draw() override;
  void SetColor(const FColor& Value) { Color = Value; }

 private:
  FColor Color = FColor::White;
};
