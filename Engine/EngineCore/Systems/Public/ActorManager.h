#pragma once
#include <memory>
#include <string>
#include <type_traits>
#include <vector>

#include "ActorId.h"
#include "BroccoliEngineAPI.h"
#include "Log.h"
#include "UMath.h"
class AActor;
class World;
class BROCCOLI_ENGINE_API FActorManager {
 public:
  FActorManager();
  ~FActorManager();
  FActorManager(const FActorManager&) = delete;
  FActorManager& operator=(const FActorManager&) = delete;

  void Update(float DeltaTime);
  void Draw();

  void SetWorld(World* world);
  World* GetWorld() const;

  const std::vector<std::unique_ptr<AActor>>& GetAllActors() const;

  template <class T, std::enable_if_t<std::is_base_of_v<AActor, T>, int> = 0>
  T* SpawnObject(
      const FVector2D& location = FVector2D::ZeroVector(),
      FRotator rotation = FRotator(0),
      bool DeferBeginPlay = false
  ) {
    std::unique_ptr<T> obj;

    if constexpr (std::is_constructible_v<T, const FVector2D&, FRotator>) {
      obj = std::make_unique<T>(location, rotation);
    } else {
      obj = std::make_unique<T>();
    }
    T* ptr = obj.get();
    ptr->SetWorld(GetWorld());
    ptr->SetActorLocation(location);
    ptr->SetActorRotation(rotation);
    RegisterActorId(*ptr);

    if (!DeferBeginPlay) ptr->Spawned();
    AddPendingActor(std::move(obj));
    return ptr;
  }
  void RemovePendingDestroy();
  void FlushPendingActors();
  void ClearAllObjects();
  AActor* FindActorById(FActorId ActorId);
  const AActor* FindActorById(FActorId ActorId) const;

 private:
  void AddPendingActor(std::unique_ptr<AActor> Actor);
  void RegisterActorId(AActor& Actor);
  void UnregisterActorId(AActor& Actor);

  struct Impl;
  Impl* ImplPtr = nullptr;
};
