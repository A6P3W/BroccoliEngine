#include "Camera2DComponent.h"

REGISTER_COMPONENT(MCamera2DComponent)

#include "RenderSystem.h"
MCamera2DComponent::~MCamera2DComponent() {
  if (RenderSystem::GetInstance().GetCamera() == this) {
    RenderSystem::GetInstance().SetCameraView(nullptr);
  }
}
void MCamera2DComponent::SetActiveCamera() { RenderSystem::GetInstance().SetCameraView(this); }
