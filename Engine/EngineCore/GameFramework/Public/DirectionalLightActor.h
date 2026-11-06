#pragma once

#include "Actor.h"
#include "Color.h"

class MDirectionalLightComponent;

class BROCCOLI_ENGINE_API ADirectionalLightActor : public AActor {
 public:
  DEFINE_ACTOR_CLASS(ADirectionalLightActor);
  ADirectionalLightActor();
  ~ADirectionalLightActor() override;

 private:
  void OnColorChanged(FColor OldValue);
  void OnIntensityChanged(float OldValue);
  void OnEnabledChanged(bool OldValue);
  MDirectionalLightComponent* LightComponent = nullptr;
  EDITOR_PROPERTY(.OnEditorChanged = ^^ADirectionalLightActor::OnColorChanged)
  FColor Color = FColor::White;
  EDITOR_PROPERTY(.OnEditorChanged = ^^ADirectionalLightActor::OnIntensityChanged, .Min = 0.0)
  float Intensity = 1.0f;
  EDITOR_PROPERTY(.OnEditorChanged = ^^ADirectionalLightActor::OnEnabledChanged)
  bool Enabled = true;
};
