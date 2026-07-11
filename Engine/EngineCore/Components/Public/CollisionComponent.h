#pragma once
#include <cstdint>
#include <unordered_map>
#include <unordered_set>

#include "SceneComponent.h"
class AActor;
enum class ECollisionShape {
  Circle,
  Rectangle,
  Line,
};
enum class ECollisionType { Overlap, Block };
struct FAABB {
  float MinX, MinY, MaxX, MaxY;
};
class MCollisionComponent : public MSceneComponent {
 public:
  MCollisionComponent();
  virtual ~MCollisionComponent();
  virtual ECollisionShape GetShapeType() const = 0;

  virtual FAABB GetAABB() const = 0;

  ECollisionType GetCollisionType() { return CollisionType; }
  void SetCollisionType(ECollisionType NewType) { CollisionType = NewType; }

  bool IsOverlappingActor(AActor* OtherActor) const;
  bool ShouldProcessPair(MCollisionComponent* OtherComponent, std::uint64_t FrameId);
  void UpdateOverlapState(AActor* OtherActor, bool bIsIntersecting);
  void SetStatic(bool IsStatic);
  bool IsStatic() const { return bIsStatic; }
  void MarkCheckedThisFrame(AActor* OtherActor);
  void FlushOverlapState();

  void OnRegister() override;
  void OnUnregister() override;

 protected:
  void OnComponentDestroy() override;

 private:
  std::unordered_set<AActor*> CheckedThisFrame;
  std::unordered_set<AActor*> OverlappingActors;
  std::unordered_map<MCollisionComponent*, std::uint64_t> LastCheckedFrame;
  std::unordered_set<AActor*> IntersectingThisFrame;
  ECollisionType CollisionType = ECollisionType::Block;
  bool bIsStatic = true;
};
