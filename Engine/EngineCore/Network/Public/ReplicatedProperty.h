#pragma once

#include "NetBuffer.h"

#include <cassert>
#include <string>
#include <type_traits>
#include <utility>

class IReplicatedProperty
{
public:
	virtual ~IReplicatedProperty() = default;

	virtual bool HasChanged() const = 0;
	virtual void UpdateCache() = 0;
	virtual void Serialize(FNetBuffer& OutBuffer) const = 0;
	virtual bool DeserializeAndTriggerOnRep(FNetBuffer& InBuffer) = 0;
};

namespace ReplicatedPropertyDetail
{
	template<class T, class = void>
	struct THasNotEqual : std::false_type
	{
	};

	template<class T>
	struct THasNotEqual<T, std::void_t<decltype(std::declval<const T&>() != std::declval<const T&>())>>
		: std::is_convertible<decltype(std::declval<const T&>() != std::declval<const T&>()), bool>
	{
	};

	template<class T>
	void WriteReplicatedValue(FNetBuffer& OutBuffer, const T& Value)
	{
		if constexpr (std::is_same_v<T, std::string>) {
			OutBuffer.WriteString(Value);
		}
		else {
			static_assert(std::is_trivially_copyable_v<T>, "Replicated properties must be std::string or trivially copyable.");
			OutBuffer.Write(Value);
		}
	}

	template<class T>
	bool ReadReplicatedValue(FNetBuffer& InBuffer, T& OutValue)
	{
		if constexpr (std::is_same_v<T, std::string>) {
			return InBuffer.ReadString(OutValue);
		}
		else {
			static_assert(std::is_trivially_copyable_v<T>, "Replicated properties must be std::string or trivially copyable.");
			return InBuffer.Read(OutValue);
		}
	}

	template<class T>
	class TReplicatedPropertyBase : public IReplicatedProperty
	{
	public:
		explicit TReplicatedPropertyBase(T* InProperty)
			: Property(InProperty)
			, CachedValue(*InProperty)
		{
			assert(InProperty != nullptr);
			static_assert(THasNotEqual<T>::value, "Replicated properties require operator!=.");
		}

		bool HasChanged() const override
		{
			return Property && (*Property != CachedValue);
		}

		void UpdateCache() override
		{
			if (Property) {
				CachedValue = *Property;
			}
		}

		void Serialize(FNetBuffer& OutBuffer) const override
		{
			if (Property) {
				WriteReplicatedValue(OutBuffer, *Property);
			}
		}

		bool DeserializeAndTriggerOnRep(FNetBuffer& InBuffer) override
		{
			if (!Property) {
				return false;
			}

			const T OldValue = *Property;
			T NewValue = *Property;
			if (!ReadReplicatedValue(InBuffer, NewValue)) {
				return false;
			}

			*Property = std::move(NewValue);
			if (OldValue != *Property) {
				TriggerOnRep(OldValue);
			}

			return true;
		}

	protected:
		virtual void TriggerOnRep(const T& OldValue)
		{
		}

	private:
		T* Property = nullptr;
		T CachedValue;
	};
}

template<class T, class TClass = void>
class TReplicatedProperty : public ReplicatedPropertyDetail::TReplicatedPropertyBase<T>
{
public:
	using FOnRep = void (TClass::*)(T);

	TReplicatedProperty(T* InProperty, TClass* InObject, FOnRep InOnRep)
		: ReplicatedPropertyDetail::TReplicatedPropertyBase<T>(InProperty)
		, Object(InObject)
		, OnRep(InOnRep)
	{
	}

protected:
	void TriggerOnRep(const T& OldValue) override
	{
		if (Object && OnRep) {
			(Object->*OnRep)(OldValue);
		}
	}

private:
	TClass* Object = nullptr;
	FOnRep OnRep = nullptr;
};

template<class T>
class TReplicatedProperty<T, void> : public ReplicatedPropertyDetail::TReplicatedPropertyBase<T>
{
public:
	explicit TReplicatedProperty(T* InProperty)
		: ReplicatedPropertyDetail::TReplicatedPropertyBase<T>(InProperty)
	{
	}
};