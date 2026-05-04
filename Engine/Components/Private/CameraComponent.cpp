#include "CameraComponent.h"
#include "RenderSystem.h"
void MCameraComponent::SetActiveCamera()
{
	RenderSystem::GetInstance().SetCameraView(this);
}
