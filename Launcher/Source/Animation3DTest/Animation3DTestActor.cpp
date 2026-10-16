#include "Animation3DTestActor.h"

#include "Camera3DComponent.h"
#include "SkeletalMeshComponent.h"

REGISTER_ACTOR(AAnimation3DTestActor)
REGISTER_ACTOR(AAnimation3DTestCameraActor)

AAnimation3DTestActor::AAnimation3DTestActor() {
  Mesh = NewObject<MSkeletalMeshComponent>(this, "FoxMesh");
  SetRootComponent(Mesh);
  Mesh->SetModelPath(FPath("/Engine/Model/Fox.glb"));
  Mesh->RegisterComponent();
}

void AAnimation3DTestActor::BeginPlay() {
  AActor::BeginPlay();
  Mesh->SetPlaybackSpeed(Speed);
  Mesh->Play(AnimationName, true);
}

AAnimation3DTestCameraActor::AAnimation3DTestCameraActor() {
  auto* Camera = NewObject<MCamera3DComponent>(this, "AnimationCamera");
  SetRootComponent(Camera);
  Camera->SetActiveCamera();
  Camera->RegisterComponent();
}
