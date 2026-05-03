#include "CameraComponent.h"
#include <Systems/RenderSystem.h>
void MCameraComponent::SetActiveCamera()
{
	RenderSystem::GetInstance().SetCameraView(this);
}
