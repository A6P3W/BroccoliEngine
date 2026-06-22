#pragma once

#include <cstdint>

class FTimerHandle
{
public:
	FTimerHandle() = default;
	explicit FTimerHandle(uint64_t InId)
		: Id(InId)
	{}

	bool IsValid() const
	{
		return Id != 0;
	}

	void Invalidate()
	{
		Id = 0;
	}

	uint64_t GetId() const
	{
		return Id;
	}

	bool operator==(const FTimerHandle& Other) const
	{
		return Id == Other.Id;
	}

	bool operator!=(const FTimerHandle& Other) const
	{
		return !(*this == Other);
	}

private:
	uint64_t Id = 0;
};
