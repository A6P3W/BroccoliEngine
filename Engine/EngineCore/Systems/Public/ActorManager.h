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

/// @brief 各ワールド内のActorを管理するクラス
class BROCCOLI_ENGINE_API FActorManager {
 public:
  FActorManager();
  ~FActorManager();
  FActorManager(const FActorManager&) = delete;
  FActorManager& operator=(const FActorManager&) = delete;

  /// @brief ワールドのActorの更新関数。
  /// @param DeltaTime
  void Update(float DeltaTime);
  /// @brief ワールドのActorの描画関数。
  void Draw();

  void SetWorld(World* world);
  World* GetWorld() const;

  /// @brief ワールドに生成されたすべてのアクタの配列を取得します。
  /// @return
  const std::vector<std::unique_ptr<AActor>>& GetAllActors() const;
  size_t GetActiveActorCount() const;

  /// @brief Actorをワールドに生成します。
  /// @tparam T Actorのクラス
  /// @tparam
  /// @param location 生成する座標
  /// @param rotation 初期回転
  /// @param DeferBeginPlay 内部用(BeginPlayの実行を行わない)
  /// @return Actorのポインタ
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
    ptr->SetActorLocation2D(location);
    ptr->SetActorRotation2D(rotation);
    RegisterActorId(*ptr);
    RegisterInstanceName(*ptr);

    if (!DeferBeginPlay) ptr->Spawned();
    AddPendingActor(std::move(obj));
    return ptr;
  }
  void RemovePendingDestroy();
  void FlushPendingActors();
  void ClearAllObjects();
  AActor* FindActorById(FActorId ActorId);
  const AActor* FindActorById(FActorId ActorId) const;
  AActor* FindActorByIdIncludingPendingDestroy(FActorId ActorId);
  const AActor* FindActorByIdIncludingPendingDestroy(FActorId ActorId) const;
  bool AssignInstanceName(AActor& Actor, const std::string& RequestedName);

 private:
  void AddPendingActor(std::unique_ptr<AActor> Actor);
  void RegisterActorId(AActor& Actor);
  void UnregisterActorId(AActor& Actor);
  void RegisterInstanceName(AActor& Actor);
  void UnregisterInstanceName(AActor& Actor);
  std::string AllocateUniqueInstanceName(
      const std::string& RequestedName, const std::string& ClassName
  );

  struct Impl;
  Impl* ImplPtr = nullptr;
};
