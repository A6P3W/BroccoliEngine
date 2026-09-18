#include "SingleAxisRotationTestActor.h"

#include "CubeComponent.h"

REGISTER_ACTOR(ASingleAxisRotationTestActor)

ASingleAxisRotationTestActor::ASingleAxisRotationTestActor() {
  Cube = NewObject<MCubeComponent>(this);
  if (Cube == nullptr) return;

  SetRootComponent(Cube);
  Cube->SetColor({255, 170, 70, 255});
  Cube->RegisterComponent();
}

void ASingleAxisRotationTestActor::OnUpdate(float DeltaTime) {
  AActor::OnUpdate(DeltaTime);

  const FQuaternion DeltaRotation =
      FQuaternion::FromRotator({0.0f, RotationSpeed * DeltaTime, 0.0f});
  SetActorRotation3D((GetActorRotation3D() * DeltaRotation).Normalize());
}
