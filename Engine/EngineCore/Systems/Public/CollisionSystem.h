#pragma once
#include "BroccoliEngineAPI.h"
#include "UMath.h"

class MCollisionComponent;
class MCircleCollisionComponent;
class MRectangleCollisionComponent;
class MLineCollisionComponent;
class MMovementComponent;
class AActor;

class BROCCOLI_ENGINE_API FCollisionSystem {
 public:
  FCollisionSystem();
  ~FCollisionSystem();
  FCollisionSystem(const FCollisionSystem&) = delete;
  FCollisionSystem& operator=(const FCollisionSystem&) = delete;

  void RegisterCollision(MCollisionComponent* component);
  void UnRegisterCollision(MCollisionComponent* component);
  void RebuildStaticCollisionMap();

  void BeginSceneTransition();
  void EndSceneTransition();

  void UpdateCollisionMap();
  void CheckCollisions();

  void RemoveActorReferences(AActor* Actor);

  float GetCollisionCellSize() const;

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
  void RegisterToStaticMap(MCollisionComponent* component);

  struct Impl;
  Impl* ImplPtr = nullptr;
};
