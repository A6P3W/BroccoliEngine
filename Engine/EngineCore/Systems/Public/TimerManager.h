#pragma once

#include "TimerHandle.h"

#include <cstdint>
#include <functional>
#include <unordered_map>
#include <unordered_set>
#include <vector>

class MTimerManager
{
public:

	MTimerManager();
	~MTimerManager();

	template<class UserClass>
	void SetTimer(
		FTimerHandle& Handle,
		UserClass* Object,
		void (UserClass::* MemberFunc)(),
		float Rate,
		bool bLoop,
		float FirstDelay = -1.0f)
	{
		if (Object == nullptr || MemberFunc == nullptr) {
			ClearTimer(Handle);
			return;
		}

		SetTimerInternal(
			Handle,
			static_cast<const void*>(Object),
			[Object, MemberFunc]() { (Object->*MemberFunc)(); },
			Rate,
			bLoop,
			FirstDelay);
	}

	void ClearTimer(FTimerHandle& Handle);
	void ClearAllTimersForObject(const void* OwnerObject);
	bool IsTimerActive(const FTimerHandle& Handle) const;
	float GetTimerRemaining(const FTimerHandle& Handle) const;
	void Update(float DeltaTime);

private:
	MTimerManager(const MTimerManager&) = delete;
	MTimerManager& operator=(const MTimerManager&) = delete;

	struct FTimerData
	{
		float Rate = 0.0f;
		float Remaining = 0.0f;
		bool bLoop = false;
		float FirstDelay = -1.0f;
		std::function<void()> Callback;
		bool bPendingRemove = false;
		const void* OwnerObject = nullptr;
	};

	void SetTimerInternal(
		FTimerHandle& Handle,
		const void* OwnerObject,
		std::function<void()> Callback,
		float Rate,
		bool bLoop,
		float FirstDelay);

	void AddTimerInternal(uint64_t TimerId, FTimerData&& TimerData);
	void RemoveTimerInternal(uint64_t TimerId);
	void FlushPendingChanges();
	uint64_t AllocateTimerId();

	std::unordered_map<uint64_t, FTimerData> Timers;
	std::unordered_map<const void*, std::unordered_set<uint64_t>> OwnerToTimerIds;
	std::unordered_map<uint64_t, FTimerData> PendingAddTimers;
	std::unordered_set<uint64_t> PendingRemoveTimerIds;
	uint64_t NextTimerId = 1;
	bool bUpdating = false;
};
