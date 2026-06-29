#pragma once
#include <cstdint>
#include <unordered_map>
#include <unordered_set>

#include "Actor.h"
#include "SceneComponent.h"
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

  bool IsOverlappingActor(AActor* OtherActor) const {
    return OverlappingActors.contains(OtherActor);
  }
  bool ShouldProcessPair(MCollisionComponent* OtherComponent, std::uint64_t FrameId) {
    auto it = LastCheckedFrame.find(OtherComponent);
    if (it != LastCheckedFrame.end() && it->second == FrameId) {
      CheckedThisFrame.insert(OtherComponent->GetOwner());
      return false;
    }

    LastCheckedFrame[OtherComponent] = FrameId;

    auto& otherFrame = OtherComponent->LastCheckedFrame;
    otherFrame[this] = FrameId;

    return true;
  }
  void UpdateOverlapState(AActor* OtherActor, bool bIsIntersecting) {
    CheckedThisFrame.insert(OtherActor);
    if (bIsIntersecting) {
      IntersectingThisFrame.insert(OtherActor);
      if (OverlappingActors.insert(OtherActor).second) {
        GetOwner()->BeginOverlap(OtherActor);
      }
    }
  }
  void SetStatic(bool IsStatic);
  bool IsStatic() const { return bIsStatic; }
  void MarkCheckedThisFrame(AActor* OtherActor);
  void FlushOverlapState();

  virtual void RegisterComponent() override;
  virtual void UnRegisterComponent() override;

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
