#pragma once
#include "BroccoliEngineAPI.h"
#include "UMath.h"

class MCollision2DComponent;
class MCircleCollision2DComponent;
class MRectangleCollision2DComponent;
class MLineCollision2DComponent;
class MMovementComponent;
class AActor;

/// @brief 2Dの当たり判定を処理するクラス。
class BROCCOLI_ENGINE_API FCollisionSystem {
 public:
  FCollisionSystem();
  ~FCollisionSystem();
  FCollisionSystem(const FCollisionSystem&) = delete;
  FCollisionSystem& operator=(const FCollisionSystem&) = delete;

  /// @brief CollisionSystemにCollisionを登録
  /// @param component
  void RegisterCollision(MCollision2DComponent* component);
  /// @brief CollisionSystemからCollisionを削除
  /// @param component
  void UnRegisterCollision(MCollision2DComponent* component);
  void RebuildStaticCollisionMap();

  void BeginSceneTransition();
  void EndSceneTransition();

  void UpdateCollisionMap();
  void CheckCollisions();

  void RemoveActorReferences(AActor* Actor);

  /// @brief Collision2D判定Cellサイズを取得。
  /// @return
  float GetCollisionCellSize() const;

 private:
  void CircleAndCircle(MCircleCollision2DComponent* a, MCircleCollision2DComponent* b);
  void CircleAndRectangle(
      MCircleCollision2DComponent* circle, MRectangleCollision2DComponent* rect
  );
  void RectangleAndRectangle(MRectangleCollision2DComponent* a, MRectangleCollision2DComponent* b);
  void LineAndCircle(MLineCollision2DComponent* line, MCircleCollision2DComponent* circle);
  void LineAndRectangle(MLineCollision2DComponent* line, MRectangleCollision2DComponent* rect);
  void LineAndLine(MLineCollision2DComponent* a, MLineCollision2DComponent* b);

  static void CancelNormalVelocity(MMovementComponent* move, const FVector2D& normal);
  /**
   * @brief 重なった2つのCollision2Dの解決
   * 
   * @param ActorA 
   * @param ActorB 
   * @param normal 方向
   * @param overlapDepth 重なりの深さ 
   */
  void CollisionResolution(
      AActor* ActorA, AActor* ActorB, const FVector2D& normal, float overlapDepth
  );

  /// @brief ２つのCollision2Dの当たり判定処理
  /// @param A
  /// @param B
  void CheckCollisionPair(MCollision2DComponent* A, MCollision2DComponent* B);
  void RegisterToStaticMap(MCollision2DComponent* component);

  struct Impl;
  Impl* ImplPtr = nullptr;
};
