#include "CameraComponent.h"
#include "RenderSystem.h"
MCameraComponent::~MCameraComponent()
{
	if (RenderSystem::GetInstance().GetCamera() == this) {
		RenderSystem::GetInstance().SetCameraView(nullptr);
	}
}
void MCameraComponent::SetActiveCamera()
{
	RenderSystem::GetInstance().SetCameraView(this);
}
