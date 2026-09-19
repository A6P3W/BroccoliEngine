#pragma once

#include <cstdint>

#include "BroccoliEngineAPI.h"
#include "SceneComponent.h"

enum class ECollisionShape3D : uint8_t { Box, Sphere };
enum class ECollisionType3D : uint8_t { Block, Overlap };

class BROCCOLI_ENGINE_API MCollisionComponent3D : public MSceneComponent {
 public:
  DEFINE_ACTOR_COMPONENT_CLASS(MCollisionComponent3D)
  virtual ~MCollisionComponent3D() override = default;

  virtual ECollisionShape3D GetShapeType3D() const = 0;
  virtual FVector3D GetShapeDimensions3D() const = 0;

  ECollisionType3D GetCollisionType3D() const { return CollisionType; }
  void SetCollisionType3D(ECollisionType3D NewCollisionType) { CollisionType = NewCollisionType; }
  uint16_t GetCollisionLayer3D() const { return CollisionLayer; }
  void SetCollisionLayer3D(uint16_t NewCollisionLayer) { CollisionLayer = NewCollisionLayer; }
  uint16_t GetCollisionMask3D() const { return CollisionMask; }
  void SetCollisionMask3D(uint16_t NewCollisionMask) { CollisionMask = NewCollisionMask; }

 protected:
  void OnRegister() override;
  void OnUnregister() override;

 private:
  ECollisionType3D CollisionType = ECollisionType3D::Block;
  uint16_t CollisionLayer = 0;
  uint16_t CollisionMask = 0xffff;
};
