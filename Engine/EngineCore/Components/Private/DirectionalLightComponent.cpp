#include "DirectionalLightComponent.h"

#include "RenderSystem.h"

FVector3D MDirectionalLightComponent::GetDirection() const {
  return GetWorldRotation3D().RotateVector({0.0f, 0.0f, 1.0f}).Normalize();
}

void MDirectionalLightComponent::Draw() {
  if (!IsVisible() || !Enabled) return;
  RenderSystem::GetInstance().SubmitDirectionalLight({GetDirection(), Color, Intensity});
}
