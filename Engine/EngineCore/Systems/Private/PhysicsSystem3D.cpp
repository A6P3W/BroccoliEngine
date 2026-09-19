#include "PhysicsSystem3D.h"

#include <algorithm>
#include <cmath>
#include <memory>

#include "Jolt/JoltPhysicsBackend.h"

namespace {
constexpr float DefaultFixedTimeStep = 1.0f / 60.0f;
constexpr float MaximumFrameDeltaTime = 0.25f;
constexpr uint32_t MaximumStepsPerFrame = 8;
}  // namespace

struct FPhysicsSystem3D::Impl {
  std::unique_ptr<FJoltPhysicsBackend> Backend = std::make_unique<FJoltPhysicsBackend>();
  float FixedTimeStep = DefaultFixedTimeStep;
  float Accumulator = 0.0f;
};

FPhysicsSystem3D::FPhysicsSystem3D() : ImplPtr(new Impl()) {}

FPhysicsSystem3D::~FPhysicsSystem3D() { delete ImplPtr; }

void FPhysicsSystem3D::Step(float DeltaTime) {
  if (!IsInitialized() || !std::isfinite(DeltaTime) || DeltaTime <= 0.0f) {
    return;
  }

  ImplPtr->Accumulator += (std::min)(DeltaTime, MaximumFrameDeltaTime);
  uint32_t StepCount = 0;
  while (ImplPtr->Accumulator >= ImplPtr->FixedTimeStep && StepCount < MaximumStepsPerFrame) {
    ImplPtr->Backend->Step(ImplPtr->FixedTimeStep);
    ImplPtr->Accumulator -= ImplPtr->FixedTimeStep;
    ++StepCount;
  }
  if (StepCount == MaximumStepsPerFrame) {
    ImplPtr->Accumulator = 0.0f;
  }
}

void FPhysicsSystem3D::SetFixedTimeStep(float NewFixedTimeStep) {
  if (std::isfinite(NewFixedTimeStep) && NewFixedTimeStep > 0.0f) {
    ImplPtr->FixedTimeStep = NewFixedTimeStep;
  }
}

bool FPhysicsSystem3D::IsInitialized() const {
  return ImplPtr != nullptr && ImplPtr->Backend != nullptr && ImplPtr->Backend->IsInitialized();
}

uint32_t FPhysicsSystem3D::GetBodyCount() const {
  return IsInitialized() ? ImplPtr->Backend->GetBodyCount() : 0;
}

float FPhysicsSystem3D::GetFixedTimeStep() const { return ImplPtr->FixedTimeStep; }
