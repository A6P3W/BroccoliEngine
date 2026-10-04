#include "StaticMeshComponent.h"

#include "ComponentRegistry.h"

REGISTER_COMPONENT(MStaticMeshComponent)

#include "RenderSystem.h"

void MStaticMeshComponent::Draw() {
  if (!IsVisible() || ModelHandle == 0) return;
  RenderSystem::GetInstance().SubmitStaticMesh(GetWorldTransform3D(), ModelHandle, Tint);
}
