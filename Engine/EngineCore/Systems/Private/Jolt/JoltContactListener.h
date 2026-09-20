#pragma once

#include <cstdint>
#include <mutex>
#include <vector>

// Jolt headers deliberately stay in the implementation file. The backend only
// needs stable body identity values when it drains these events on the main thread.
struct FJoltContactEvent {
  enum class EType : uint8_t { Begin, End };

  uint32_t BodyIdA = 0;
  uint32_t BodyIdB = 0;
  EType Type = EType::Begin;
};

class FJoltContactEventQueue {
 public:
  void Push(uint32_t BodyIdA, uint32_t BodyIdB, FJoltContactEvent::EType Type);
  std::vector<FJoltContactEvent> Drain();

 private:
  std::mutex Mutex;
  std::vector<FJoltContactEvent> Events;
};
