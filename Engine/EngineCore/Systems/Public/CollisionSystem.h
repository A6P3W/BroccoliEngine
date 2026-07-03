#pragma once
#include <map>
#include <memory>
#include <unordered_map>
#include <utility>
#include <vector>

#include "UMath.h"

class MCollisionComponent;
class MCircleCollisionComponent;
class MRectangleCollisionComponent;
class MLineCollisionComponent;
class MMovementComponent;
class AActor;

struct pair_hash {
  inline std::size_t operator()(const std::pair<int, int>& v) const {
    return v.first * 31 + v.second;
  }
};
class FCollisionSystem {
 public:
  FCollisionSystem();
  ~FCollisionSystem();

  void RegisterCollision(MCollisionComponent* component);
  void UnRegisterCollision(MCollisionComponent* component);
  void RebuildStaticCollisionMap();
  void BeginSceneTransition();
  void EndSceneTransition();

  void UpdateCollisionMap();

  void CheckCollisions();

  float GetCollisionCellSize() { return CollisionCellSize; }

 private:
  void CircleAndCircle(MCircleCollisionComponent* a, MCircleCollisionComponent* b);
  void CircleAndRectangle(MCircleCollisionComponent* circle, MRectangleCollisionComponent* rect);
  void RectangleAndRectangle(MRectangleCollisionComponent* a, MRectangleCollisionComponent* b);
  void LineAndCircle(MLineCollisionComponent* line, MCircleCollisionComponent* circle);
  void LineAndRectangle(MLineCollisionComponent* line, MRectangleCollisionComponent* rect);
  void LineAndLine(MLineCollisionComponent* a, MLineCollisionComponent* b);

  static void CancelNormalVelocity(MMovementComponent* move, const FVector2D& normal);

  void CollisionResolution(
      AActor* ActorA, AActor* ActorB, const FVector2D& normal, float overlapDepth
  );

  void CheckCollisionPair(MCollisionComponent* A, MCollisionComponent* B);

  static std::unique_ptr<FCollisionSystem> s_Instance;

  std::vector<MCollisionComponent*> CollisionComponents;
  std::unordered_map<std::pair<int, int>, std::vector<MCollisionComponent*>, pair_hash>
      StaticCollisionMap;
  std::unordered_map<std::pair<int, int>, std::vector<MCollisionComponent*>, pair_hash>
      DynamicCollisionMap;
  float CollisionCellSize = 100;
  std::uint64_t FrameId = 0;
  bool bDeferStaticRebuild = false;
  bool bPendingStaticRebuild = false;

  std::vector<MCollisionComponent*> PendingStaticRegistrations;
  void RegisterToStaticMap(MCollisionComponent* component);
};
