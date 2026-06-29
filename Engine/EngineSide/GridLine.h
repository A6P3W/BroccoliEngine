#pragma once
#include "Actor.h"
class AGridLine : public AActor {
 public:
  DEFINE_ACTOR_CLASS(AGridLine)
  AGridLine();
  void BeginPlay() override;
  void OnUpdate(float DeltaTime) override;
  void SimpleDraw(float cellSize, int color);

 private:
  float CollisionCellSize;
};
