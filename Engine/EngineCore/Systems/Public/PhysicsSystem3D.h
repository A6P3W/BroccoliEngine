#pragma once

#include <cstdint>

#include "BroccoliEngineAPI.h"

class BROCCOLI_ENGINE_API FPhysicsSystem3D {
 public:
  FPhysicsSystem3D();
  ~FPhysicsSystem3D();
  FPhysicsSystem3D(const FPhysicsSystem3D&) = delete;
  FPhysicsSystem3D& operator=(const FPhysicsSystem3D&) = delete;

  void Step(float DeltaTime);
  void SetFixedTimeStep(float NewFixedTimeStep);

  bool IsInitialized() const;
  uint32_t GetBodyCount() const;
  float GetFixedTimeStep() const;

 private:
  struct Impl;
  Impl* ImplPtr = nullptr;
};
