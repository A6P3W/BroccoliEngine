#pragma once
#include <cstdint>
#include <vector>

#include "BroccoliEngineAPI.h"
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
class BROCCOLI_ENGINE_API MCollisionComponent : public MSceneComponent {
 public:
  MCollisionComponent();
  virtual ~MCollisionComponent();
  virtual ECollisionShape GetShapeType() const = 0;

  std::vector<AActor*> GetOverlappingActors() const;
  virtual FAABB GetAABB() const = 0;

  ECollisionType GetCollisionType() const;
  void SetCollisionType(ECollisionType NewType);

  bool IsOverlappingActor(AActor* OtherActor) const;
  bool ShouldProcessPair(MCollisionComponent* OtherComponent, std::uint64_t FrameId);
  void UpdateOverlapState(AActor* OtherActor, bool bIsIntersecting);
  void RemoveActorReference(AActor* Actor);
  void SetStatic(bool IsStatic);
  bool IsStatic() const;
  void MarkCheckedThisFrame(AActor* OtherActor);
  void FlushOverlapState();

  void OnRegister() override;
  void OnUnregister() override;

 protected:
  void OnComponentDestroy() override;

 private:
  struct Impl;
  Impl* ImplPtr = nullptr;
};
