#include "TimerManager.h"

#include <algorithm>
#include <atomic>

TimerManager::TimerManager()
{
}

TimerManager::~TimerManager()
{
}

void TimerManager::SetTimerInternal(
	FTimerHandle& Handle,
	const void* OwnerObject,
	std::function<void()> Callback,
	float Rate,
	bool bLoop,
	float FirstDelay)
{
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

	if (m_isUpdating) {
		if (auto ExistingTimerIt = m_timers.find(TimerId); ExistingTimerIt != m_timers.end()) {
			ExistingTimerIt->second.bPendingRemove = true;
			m_pendingRemoveTimerIds.insert(TimerId);
		}

		m_pendingAddTimers[TimerId] = std::move(TimerData);
		return;
	}

	RemoveTimerInternal(TimerId);
	AddTimerInternal(TimerId, std::move(TimerData));
}

void TimerManager::ClearTimer(FTimerHandle& Handle)
{
	if (!Handle.IsValid()) {
		return;
	}

	const uint64_t TimerId = Handle.GetId();
	Handle.Invalidate();

	if (m_isUpdating) {
		if (auto ExistingTimerIt = m_timers.find(TimerId); ExistingTimerIt != m_timers.end()) {
			ExistingTimerIt->second.bPendingRemove = true;
		}
		m_pendingRemoveTimerIds.insert(TimerId);
		m_pendingAddTimers.erase(TimerId);
		return;
	}

	RemoveTimerInternal(TimerId);
}

void TimerManager::ClearAllTimersForObject(const void* OwnerObject)
{
	if (OwnerObject == nullptr) {
		return;
	}

	std::vector<uint64_t> TimerIds;
	if (const auto OwnerTimersIt = m_ownerToTimerIds.find(OwnerObject); OwnerTimersIt != m_ownerToTimerIds.end()) {
		TimerIds.insert(TimerIds.end(), OwnerTimersIt->second.begin(), OwnerTimersIt->second.end());
	}

	for (const auto& PendingAdd : m_pendingAddTimers) {
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
		if (m_isUpdating) {
			if (auto ExistingTimerIt = m_timers.find(TimerId); ExistingTimerIt != m_timers.end()) {
				ExistingTimerIt->second.bPendingRemove = true;
			}
			m_pendingRemoveTimerIds.insert(TimerId);
			m_pendingAddTimers.erase(TimerId);
		}
		else {
			RemoveTimerInternal(TimerId);
		}
	}
}

bool TimerManager::IsTimerActive(const FTimerHandle& Handle) const
{
	if (!Handle.IsValid()) {
		return false;
	}

	const uint64_t TimerId = Handle.GetId();
	const auto TimerIt = m_timers.find(TimerId);
	if (TimerIt == m_timers.end()) {
		return false;
	}

	if (TimerIt->second.bPendingRemove) {
		return false;
	}

	return m_pendingRemoveTimerIds.find(TimerId) == m_pendingRemoveTimerIds.end();
}

float TimerManager::GetTimerRemaining(const FTimerHandle& Handle) const
{
	if (!IsTimerActive(Handle)) {
		return -1.0f;
	}

	const auto TimerIt = m_timers.find(Handle.GetId());
	if (TimerIt == m_timers.end()) {
		return -1.0f;
	}

	return TimerIt->second.Remaining;
}

void TimerManager::Update(float DeltaTime)
{
	if (DeltaTime <= 0.0f || m_timers.empty()) {
		FlushPendingChanges();
		return;
	}

	m_isUpdating = true;

	std::vector<uint64_t> TimerIds;
	TimerIds.reserve(m_timers.size());
	for (const auto& TimerPair : m_timers) {
		TimerIds.push_back(TimerPair.first);
	}

	for (uint64_t TimerId : TimerIds) {
		auto TimerIt = m_timers.find(TimerId);
		if (TimerIt == m_timers.end()) {
			continue;
		}

		FTimerData& TimerData = TimerIt->second;
		if (TimerData.bPendingRemove || m_pendingRemoveTimerIds.find(TimerId) != m_pendingRemoveTimerIds.end()) {
			continue;
		}

		TimerData.Remaining -= DeltaTime;

		while (TimerData.Remaining <= 0.0f) {
			if (TimerData.bPendingRemove || m_pendingRemoveTimerIds.find(TimerId) != m_pendingRemoveTimerIds.end()) {
				break;
			}

			TimerData.Callback();

			if (TimerData.bPendingRemove || m_pendingRemoveTimerIds.find(TimerId) != m_pendingRemoveTimerIds.end()) {
				break;
			}

			if (!TimerData.bLoop) {
				TimerData.bPendingRemove = true;
				m_pendingRemoveTimerIds.insert(TimerId);
				break;
			}

			TimerData.Remaining += TimerData.Rate;
		}
	}

	m_isUpdating = false;
	FlushPendingChanges();
}

void TimerManager::AddTimerInternal(uint64_t TimerId, FTimerData&& TimerData)
{
	const void* OwnerObject = TimerData.OwnerObject;
	m_timers[TimerId] = std::move(TimerData);

	if (OwnerObject != nullptr) {
		m_ownerToTimerIds[OwnerObject].insert(TimerId);
	}
}

void TimerManager::RemoveTimerInternal(uint64_t TimerId)
{
	const auto TimerIt = m_timers.find(TimerId);
	if (TimerIt == m_timers.end()) {
		return;
	}

	const void* OwnerObject = TimerIt->second.OwnerObject;
	if (OwnerObject != nullptr) {
		if (auto OwnerTimersIt = m_ownerToTimerIds.find(OwnerObject); OwnerTimersIt != m_ownerToTimerIds.end()) {
			OwnerTimersIt->second.erase(TimerId);
			if (OwnerTimersIt->second.empty()) {
				m_ownerToTimerIds.erase(OwnerTimersIt);
			}
		}
	}

	m_timers.erase(TimerIt);
}

void TimerManager::FlushPendingChanges()
{
	if (m_isUpdating) {
		return;
	}

	if (!m_pendingRemoveTimerIds.empty()) {
		const std::vector<uint64_t> PendingRemoves(m_pendingRemoveTimerIds.begin(), m_pendingRemoveTimerIds.end());
		for (uint64_t TimerId : PendingRemoves) {
			RemoveTimerInternal(TimerId);
		}
		m_pendingRemoveTimerIds.clear();
	}

	if (!m_pendingAddTimers.empty()) {
		std::vector<std::pair<uint64_t, FTimerData>> PendingAdds;
		PendingAdds.reserve(m_pendingAddTimers.size());
		for (auto& PendingAdd : m_pendingAddTimers) {
			PendingAdds.emplace_back(PendingAdd.first, std::move(PendingAdd.second));
		}
		m_pendingAddTimers.clear();

		for (auto& PendingAdd : PendingAdds) {
			RemoveTimerInternal(PendingAdd.first);
			AddTimerInternal(PendingAdd.first, std::move(PendingAdd.second));
		}
	}
}

uint64_t TimerManager::AllocateTimerId()
{
	return m_nextTimerId++;
}
