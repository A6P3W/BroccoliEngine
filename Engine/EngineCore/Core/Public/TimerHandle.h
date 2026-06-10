#pragma once

#include <cstdint>

class FTimerHandle
{
public:
	FTimerHandle() = default;
	explicit FTimerHandle(uint64_t InId)
		: m_Id(InId)
	{
	}

	bool IsValid() const
	{
		return m_Id != 0;
	}

	void Invalidate()
	{
		m_Id = 0;
	}

	uint64_t GetId() const
	{
		return m_Id;
	}

	bool operator==(const FTimerHandle& Other) const
	{
		return m_Id == Other.m_Id;
	}

	bool operator!=(const FTimerHandle& Other) const
	{
		return !(*this == Other);
	}

private:
	uint64_t m_Id = 0;
};
