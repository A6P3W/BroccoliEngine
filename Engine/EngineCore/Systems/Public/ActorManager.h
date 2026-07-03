#pragma once
#include <memory>
#include <string>
#include <type_traits>
#include <vector>

#include "Log.h"
#include "UMath.h"
class AActor;
class World;
class FActorManager {
 public:
  FActorManager();
  ~FActorManager();

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
    Actors.push_back(std::move(obj));
    ptr->SetActorLocation(location);
    ptr->SetActorRotation(rotation);

    if (!DeferBeginPlay) ptr->Spawned();
    return ptr;
  }
  void RemovePendingDestroy();
  void ClearAllObjects();

 private:
  std::vector<std::unique_ptr<AActor>> Actors;
  World* World = nullptr;
};
