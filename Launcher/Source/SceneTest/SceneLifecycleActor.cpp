#include "SceneLifecycleActor.h"

#include "Log.h"
#include "SpriteComponent.h"
#include "World.h"

REGISTER_ACTOR(ASceneLifecycleActor)
REGISTER_ACTOR(ASceneSpawnedMarkerActor)

void ASceneLifecycleActor::BeginPlay() {
  AActor::BeginPlay();
  GetWorldTimerManager().SetTimer(
      LifecycleTimer, this, &ASceneLifecycleActor::ToggleSpawnedActor, 3.0f, true
  );
  ToggleSpawnedActor();
  M_LOG("SceneLifecycleActor: a marker is spawned/destroyed every three seconds.");
}

void ASceneLifecycleActor::OnUpdate(float DeltaTime) {
  AActor::OnUpdate(DeltaTime);
  DRAW_SCREEN_LOG(
      "SceneHelp", 0.1f, "SceneTest: attachment rules + Spawn/Destroy marker / Escape LevelStarter"
  );
  DRAW_WORLD_LOG("SceneLifecycle", 0.1f, this, "Spawn/Destroy demonstration");
}

void ASceneLifecycleActor::ToggleSpawnedActor() {
  if (SpawnedActor && !SpawnedActor->IsPendingDestroy()) {
    M_LOG("SceneTest: destroy spawned marker.");
    SpawnedActor->Destroy();
    SpawnedActor = nullptr;
    return;
  }

  if (GetWorld()) {
    SpawnedActor = GetWorld()->SpawnActor<ASceneSpawnedMarkerActor>(
        GetActorLocation() + FVector2D{140.0f, 0.0f}
    );
    M_LOG("SceneTest: spawn marker actor.");
  }
}

ASceneSpawnedMarkerActor::ASceneSpawnedMarkerActor() {
  auto* Sprite = NewObject<MSpriteComponent>(this);
  Sprite->SetRenderSettings(20, RenderSpace::World);
  Sprite->SubmitCircle(18.0f, FColor{255, 180, 60}, true);
  Sprite->AttachToComponent(GetRootComponent());
  Sprite->RegisterComponent();
}

void ASceneSpawnedMarkerActor::OnUpdate(float DeltaTime) {
  AActor::OnUpdate(DeltaTime);
  AddActorRotation(FRotator(90.0f * DeltaTime));
  DRAW_WORLD_LOG("SceneSpawnedMarker", 0.1f, this, "Spawned actor");
}
