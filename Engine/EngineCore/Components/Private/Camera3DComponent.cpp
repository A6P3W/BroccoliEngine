#include "Camera3DComponent.h"

#include "RenderSystem.h"

MCamera3DComponent::~MCamera3DComponent() {
  if (RenderSystem::GetInstance().GetCamera3D() == this)
    RenderSystem::GetInstance().SetCameraView3D(nullptr);
}
FVector3D MCamera3DComponent::GetForwardVector() const {
  return GetWorldRotation3D().RotateVector({0.0f, 1.0f, 0.0f});
}
FVector3D MCamera3DComponent::GetUpVector() const {
  return GetWorldRotation3D().RotateVector({0.0f, 0.0f, 1.0f});
}
FVector3D MCamera3DComponent::GetRightVector() const {
  return GetWorldRotation3D().RotateVector({1.0f, 0.0f, 0.0f});
}
void MCamera3DComponent::SetActiveCamera() { RenderSystem::GetInstance().SetCameraView3D(this); }
