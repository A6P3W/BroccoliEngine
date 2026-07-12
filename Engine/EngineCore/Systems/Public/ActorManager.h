#pragma once
#include "BroccoliEngineAPI.h"
#include <memory>
#include <string>
#include <type_traits>
#include <vector>

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

  void SetWorld(World* world) { World = world; }
  World* GetWorld() const { return World; }

  const std::vector<std::unique_ptr<AActor>>& GetAllActors() const { return Actors; }

  template <class T, std::enable_if_t<std::is_base_of_v<AActor, T>, int> = 0>
  T* SpawnObject(
      const FVector2D& location = FVector2D::ZeroVector,
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
    ptr->SetWorld(World);
    ptr->SetActorLocation(location);
    ptr->SetActorRotation(rotation);

    if (!DeferBeginPlay) ptr->Spawned();
    PendingActors.push_back(std::move(obj));
    return ptr;
  }
  void RemovePendingDestroy();
  void FlushPendingActors();
  void ClearAllObjects();

 private:
  std::vector<std::unique_ptr<AActor>> Actors;
  std::vector<std::unique_ptr<AActor>> PendingActors;
  World* World = nullptr;
};
