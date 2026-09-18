#include "CubeComponent.h"

#include "RenderSystem.h"

void MCubeComponent::Draw() {
  RenderSystem::GetInstance().SubmitCube(GetWorldTransform3D(), Color);
}
