// clang-format off
#include <Jolt/Jolt.h>
#include <Jolt/Physics/Collision/ContactListener.h>
// clang-format on

#include "JoltContactListener.h"

void FJoltContactEventQueue::Push(
    uint32_t BodyIdA, uint32_t BodyIdB, FJoltContactEvent::EType Type
) {
  if (BodyIdA > BodyIdB) {
    std::swap(BodyIdA, BodyIdB);
  }
  std::scoped_lock Lock(Mutex);
  Events.push_back({BodyIdA, BodyIdB, Type});
}

std::vector<FJoltContactEvent> FJoltContactEventQueue::Drain() {
  std::scoped_lock Lock(Mutex);
  std::vector<FJoltContactEvent> Result;
  Result.swap(Events);
  return Result;
}
