#include <Jolt/Jolt.h>

#include "JoltRuntime.h"

#include <Jolt/Core/Factory.h>
#include <Jolt/Core/Memory.h>
#include <Jolt/RegisterTypes.h>

#include "Log.h"

namespace {
bool Initialized = false;
}

bool FJoltRuntime::Initialize() {
  if (Initialized) {
    return true;
  }

  JPH::RegisterDefaultAllocator();
  JPH::Factory::sInstance = new JPH::Factory();
  JPH::RegisterTypes();
  Initialized = true;
  M_LOG(Log, "Jolt runtime initialized.");
  return true;
}

void FJoltRuntime::Shutdown() {
  if (!Initialized) {
    return;
  }

  JPH::UnregisterTypes();
  delete JPH::Factory::sInstance;
  JPH::Factory::sInstance = nullptr;
  Initialized = false;
  M_LOG(Log, "Jolt runtime shutdown completed.");
}

bool FJoltRuntime::IsInitialized() { return Initialized; }
