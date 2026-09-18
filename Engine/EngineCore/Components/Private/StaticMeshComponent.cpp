#include "StaticMeshComponent.h"

#include "RenderSystem.h"

void MStaticMeshComponent::Draw() {
  if (!IsVisible() || ModelHandle == 0) return;
  RenderSystem::GetInstance().SubmitStaticMesh(GetWorldTransform3D(), ModelHandle, Tint);
}
