#pragma once

#include "Color.h"
#include "SceneComponent.h"

class BROCCOLI_ENGINE_API MDirectionalLightComponent : public MSceneComponent {
 public:
  DEFINE_ACTOR_COMPONENT_CLASS(MDirectionalLightComponent)

  void SetColor(const FColor& NewColor) { Color = NewColor; }
  void SetIntensity(float NewIntensity) { Intensity = NewIntensity; }
  void SetEnabled(bool NewEnabled) { Enabled = NewEnabled; }
  const FColor& GetColor() const { return Color; }
  float GetIntensity() const { return Intensity; }
  bool IsEnabled() const { return Enabled; }
  FVector3D GetDirection() const;
  void Draw() override;

 private:
  FColor Color = FColor::White;
  float Intensity = 1.0f;
  bool Enabled = true;
};
