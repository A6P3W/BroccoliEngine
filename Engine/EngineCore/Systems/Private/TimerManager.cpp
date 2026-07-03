#include "TimerManager.h"

#include <algorithm>
#include <atomic>

FTimerManager::FTimerManager() {}

FTimerManager::~FTimerManager() {}

void FTimerManager::SetTimerInternal(
    FTimerHandle& Handle,
    const void* OwnerObject,
    std::function<void()> Callback,
    float Rate,
    bool bLoop,
    float FirstDelay
) {
  if (Rate <= 0.0f || !Callback) {
    ClearTimer(Handle);
    return;
  }

  if (!Handle.IsValid()) {
    Handle = FTimerHandle(AllocateTimerId());
  }

  const uint64_t TimerId = Handle.GetId();
  const float InitialDelay = (FirstDelay >= 0.0f) ? FirstDelay : Rate;

  FTimerData TimerData;
  TimerData.Rate = Rate;
  TimerData.Remaining = InitialDelay;
  TimerData.bLoop = bLoop;
  TimerData.FirstDelay = FirstDelay;
  TimerData.Callback = std::move(Callback);
  TimerData.bPendingRemove = false;
  TimerData.OwnerObject = OwnerObject;

  if (bUpdating) {
    if (auto ExistingTimerIt = Timers.find(TimerId); ExistingTimerIt != Timers.end()) {
      ExistingTimerIt->second.bPendingRemove = true;
      PendingRemoveTimerIds.insert(TimerId);
    }

    PendingAddTimers[TimerId] = std::move(TimerData);
    return;
  }

  RemoveTimerInternal(TimerId);
  AddTimerInternal(TimerId, std::move(TimerData));
}

void FTimerManager::ClearTimer(FTimerHandle& Handle) {
  if (!Handle.IsValid()) {
    return;
  }

  const uint64_t TimerId = Handle.GetId();
  Handle.Invalidate();

  if (bUpdating) {
    if (auto ExistingTimerIt = Timers.find(TimerId); ExistingTimerIt != Timers.end()) {
      ExistingTimerIt->second.bPendingRemove = true;
    }
    PendingRemoveTimerIds.insert(TimerId);
    PendingAddTimers.erase(TimerId);
    return;
  }

  RemoveTimerInternal(TimerId);
}

void FTimerManager::ClearAllTimersForObject(const void* OwnerObject) {
  if (OwnerObject == nullptr) {
    return;
  }

  std::vector<uint64_t> TimerIds;
  if (const auto OwnerTimersIt = OwnerToTimerIds.find(OwnerObject);
      OwnerTimersIt != OwnerToTimerIds.end()) {
    TimerIds.insert(TimerIds.end(), OwnerTimersIt->second.begin(), OwnerTimersIt->second.end());
  }

  for (const auto& PendingAdd : PendingAddTimers) {
    if (PendingAdd.second.OwnerObject == OwnerObject) {
      TimerIds.push_back(PendingAdd.first);
    }
  }

  if (TimerIds.empty()) {
    return;
  }

  std::sort(TimerIds.begin(), TimerIds.end());
  TimerIds.erase(std::unique(TimerIds.begin(), TimerIds.end()), TimerIds.end());

  for (uint64_t TimerId : TimerIds) {
    if (bUpdating) {
      if (auto ExistingTimerIt = Timers.find(TimerId); ExistingTimerIt != Timers.end()) {
        ExistingTimerIt->second.bPendingRemove = true;
      }
      PendingRemoveTimerIds.insert(TimerId);
      PendingAddTimers.erase(TimerId);
    } else {
      RemoveTimerInternal(TimerId);
    }
  }
}

bool FTimerManager::IsTimerActive(const FTimerHandle& Handle) const {
  if (!Handle.IsValid()) {
    return false;
  }

  const uint64_t TimerId = Handle.GetId();
  const auto TimerIt = Timers.find(TimerId);
  if (TimerIt == Timers.end()) {
    return false;
  }

  if (TimerIt->second.bPendingRemove) {
    return false;
  }

  return PendingRemoveTimerIds.find(TimerId) == PendingRemoveTimerIds.end();
}

float FTimerManager::GetTimerRemaining(const FTimerHandle& Handle) const {
  if (!IsTimerActive(Handle)) {
    return -1.0f;
  }

  const auto TimerIt = Timers.find(Handle.GetId());
  if (TimerIt == Timers.end()) {
    return -1.0f;
  }

  return TimerIt->second.Remaining;
}

void FTimerManager::Update(float DeltaTime) {
  if (DeltaTime <= 0.0f || Timers.empty()) {
    FlushPendingChanges();
    return;
  }

  bUpdating = true;

  std::vector<uint64_t> TimerIds;
  TimerIds.reserve(Timers.size());
  for (const auto& TimerPair : Timers) {
    TimerIds.push_back(TimerPair.first);
  }

  for (uint64_t TimerId : TimerIds) {
    auto TimerIt = Timers.find(TimerId);
    if (TimerIt == Timers.end()) {
      continue;
    }

    FTimerData& TimerData = TimerIt->second;
    if (TimerData.bPendingRemove ||
        PendingRemoveTimerIds.find(TimerId) != PendingRemoveTimerIds.end()) {
      continue;
    }

    TimerData.Remaining -= DeltaTime;

    while (TimerData.Remaining <= 0.0f) {
      if (TimerData.bPendingRemove ||
          PendingRemoveTimerIds.find(TimerId) != PendingRemoveTimerIds.end()) {
        break;
      }

      TimerData.Callback();

      if (TimerData.bPendingRemove ||
          PendingRemoveTimerIds.find(TimerId) != PendingRemoveTimerIds.end()) {
        break;
      }

      if (!TimerData.bLoop) {
        TimerData.bPendingRemove = true;
        PendingRemoveTimerIds.insert(TimerId);
        break;
      }

      TimerData.Remaining += TimerData.Rate;
    }
  }

  bUpdating = false;
  FlushPendingChanges();
}

void FTimerManager::AddTimerInternal(uint64_t TimerId, FTimerData&& TimerData) {
  const void* OwnerObject = TimerData.OwnerObject;
  Timers[TimerId] = std::move(TimerData);

  if (OwnerObject != nullptr) {
    OwnerToTimerIds[OwnerObject].insert(TimerId);
  }
}

void FTimerManager::RemoveTimerInternal(uint64_t TimerId) {
  const auto TimerIt = Timers.find(TimerId);
  if (TimerIt == Timers.end()) {
    return;
  }

  const void* OwnerObject = TimerIt->second.OwnerObject;
  if (OwnerObject != nullptr) {
    if (auto OwnerTimersIt = OwnerToTimerIds.find(OwnerObject);
        OwnerTimersIt != OwnerToTimerIds.end()) {
      OwnerTimersIt->second.erase(TimerId);
      if (OwnerTimersIt->second.empty()) {
        OwnerToTimerIds.erase(OwnerTimersIt);
      }
    }
  }

  Timers.erase(TimerIt);
}

void FTimerManager::FlushPendingChanges() {
  if (bUpdating) {
    return;
  }

  if (!PendingRemoveTimerIds.empty()) {
    const std::vector<uint64_t> PendingRemoves(
        PendingRemoveTimerIds.begin(), PendingRemoveTimerIds.end()
    );
    for (uint64_t TimerId : PendingRemoves) {
      RemoveTimerInternal(TimerId);
    }
    PendingRemoveTimerIds.clear();
  }

  if (!PendingAddTimers.empty()) {
    std::vector<std::pair<uint64_t, FTimerData>> PendingAdds;
    PendingAdds.reserve(PendingAddTimers.size());
    for (auto& PendingAdd : PendingAddTimers) {
      PendingAdds.emplace_back(PendingAdd.first, std::move(PendingAdd.second));
    }
    PendingAddTimers.clear();

    for (auto& PendingAdd : PendingAdds) {
      RemoveTimerInternal(PendingAdd.first);
      AddTimerInternal(PendingAdd.first, std::move(PendingAdd.second));
    }
  }
}

uint64_t FTimerManager::AllocateTimerId() { return NextTimerId++; }
