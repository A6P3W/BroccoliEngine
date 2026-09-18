#include "ThreeDTestActor.h"

#include "Camera3DComponent.h"
#include "CubeComponent.h"
#include "ResourceManager.h"
#include "StaticMeshComponent.h"

REGISTER_ACTOR(AThreeDTestActor)

AThreeDTestActor::AThreeDTestActor() {
  Camera = NewObject<MCamera3DComponent>(this);
  Camera->SetRelativeLocation3D({5.0f, -8.0f, 5.0f});
  Camera->SetRelativeRotation3D(FQuaternion::FromRotator({-25.0f, 32.0f, 0.0f}));
  Camera->SetActiveCamera();
  Camera->RegisterComponent();

  auto* OriginCube = NewObject<MCubeComponent>(this);
  OriginCube->SetRelativeLocation3D({0.0f, 0.0f, 0.0f});
  OriginCube->SetColor({80, 170, 255, 255});
  OriginCube->RegisterComponent();

  RotatingCube = NewObject<MCubeComponent>(this);
  RotatingCube->SetRelativeLocation3D({2.0f, 0.0f, 1.0f});
  RotatingCube->SetRelativeRotation3D(FQuaternion::FromRotator({30.0f, 45.0f, 20.0f}));
  RotatingCube->SetRelativeScale3D({1.0f, 2.0f, 0.5f});
  RotatingCube->SetColor({255, 170, 70, 255});
  RotatingCube->RegisterComponent();

  auto* DepthCube = NewObject<MCubeComponent>(this);
  DepthCube->SetRelativeLocation3D({0.0f, 3.0f, 0.0f});
  DepthCube->SetRelativeScale3D({1.5f, 1.5f, 1.5f});
  DepthCube->SetColor({100, 220, 120, 255});
  DepthCube->RegisterComponent();

  const int GlbModel =
      ResourceManager::GetInstance().LoadResourceModel("/Engine/Model/SampleBox/Box.glb");
  const int GltfModel =
      ResourceManager::GetInstance().LoadResourceModel("/Engine/Model/SampleBox/BoxGLTF/box.gltf");

  StaticMeshA = NewObject<MStaticMeshComponent>(this);
  StaticMeshA->SetModel(GlbModel);
  StaticMeshA->SetRelativeLocation3D({0.0f, 3.0f, 0.0f});
  StaticMeshA->SetRelativeRotation3D(FQuaternion::FromRotator({0.0f, 45.0f, 0.0f}));
  StaticMeshA->RegisterComponent();

  StaticMeshB = NewObject<MStaticMeshComponent>(this);
  StaticMeshB->SetModel(GltfModel);
  StaticMeshB->SetRelativeLocation3D({3.0f, 3.0f, 1.0f});
  StaticMeshB->SetRelativeScale3D({1.0f, 2.0f, 0.5f});
  StaticMeshB->SetTint({255, 220, 120, 255});
  StaticMeshB->RegisterComponent();
}

void AThreeDTestActor::OnUpdate(float DeltaTime) {
  AActor::OnUpdate(DeltaTime);
  if (RotatingCube == nullptr) return;

  const FQuaternion DeltaRotation =
      FQuaternion::FromRotator({35.0f * DeltaTime, 50.0f * DeltaTime, 20.0f * DeltaTime});
  RotatingCube->SetRelativeRotation3D(
      (RotatingCube->GetRelativeRotation3D() * DeltaRotation).Normalize()
  );
}
