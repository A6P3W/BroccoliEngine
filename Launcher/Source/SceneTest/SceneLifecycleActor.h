#pragma once

#include "Actor.h"
#include "TimerHandle.h"

// Place this actor beside AAttachmentRuleTestActor. It periodically creates
// and destroys a child actor so SceneTest shows lifecycle APIs.
class ASceneLifecycleActor : public AActor {
 public:
  DEFINE_ACTOR_CLASS(ASceneLifecycleActor)

  void BeginPlay() override;
  void OnUpdate(float DeltaTime) override;

 private:
  void ToggleSpawnedActor();
  AActor* SpawnedActor = nullptr;
  FTimerHandle LifecycleTimer;
};

class ASceneSpawnedMarkerActor : public AActor {
 public:
  DEFINE_ACTOR_CLASS(ASceneSpawnedMarkerActor)
  ASceneSpawnedMarkerActor();
  void OnUpdate(float DeltaTime) override;
};
