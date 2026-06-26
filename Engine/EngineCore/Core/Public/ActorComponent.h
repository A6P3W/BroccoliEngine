#pragma once
#include "BaseObject.h"
#include "NetBuffer.h"
#include "NetworkManager.h"
#include "NetworkTypes.h"
#include "ReplicatedProperty.h"
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

class AActor;

class MActorComponent : public MBaseObject {
public:
	virtual ~MActorComponent() override;
	virtual void BeginPlay() {}
	bool Update(float DeltaTime);
	virtual void Draw() {}

	void SetOwner(AActor* owner) { Owner = owner; }
	AActor* GetOwner() const { return Owner; }

	void DestroyComponent();
	bool IsPendingDestroy() const { return bPendingDestroy; }

	virtual void RegisterComponent(){}
	virtual void UnRegisterComponent() {}

	FNetworkComponentId ComponentNetworkId = 0;
	bool bReplicates = false;

	void SetNetComponentName(std::string Name);
	const std::string& GetNetComponentName() const { return NetComponentName; }

	virtual void SerializeNetworkState(FNetBuffer& OutBuffer);
	virtual bool DeserializeNetworkState(FNetBuffer& InBuffer);
	virtual void SerializeNetworkSpawn(FNetBuffer& OutBuffer);
	virtual bool DeserializeNetworkSpawn(FNetBuffer& InBuffer);

	bool HasReplicatedStateChanged() const;
	void UpdateReplicatedStateCache();
	void MarkReplicatedStateDirty();
	uint32_t IncrementReplicationSequence();
	uint32_t GetReplicationSequence() const { return ReplicationSequence; }
	uint32_t GetLastReceivedReplicationSequence() const { return LastReceivedReplicationSequence; }
	void SetLastReceivedReplicationSequence(uint32_t Sequence);

	template<class T>
	void RegisterReplicatedProperty(T* Property)
	{
		if (!Property) {
			return;
		}

		ReplicatedProperties.push_back(std::make_unique<TReplicatedProperty<T>>(Property));
		MarkReplicatedStateDirty();
	}

	template<class T, class TObject>
	void RegisterReplicatedProperty(T* Property, TObject* Object, void (TObject::* OnRep)(T))
	{
		if (!Property || !Object || !OnRep) {
			return;
		}

		ReplicatedProperties.push_back(std::make_unique<TReplicatedProperty<T, TObject>>(Property, Object, OnRep));
		MarkReplicatedStateDirty();
	}

	using FRPCHandler = std::function<void(FNetBuffer&)>;
	void RegisterRPC(FNetworkRPCId RPCId, ENetRPCType RPCType, FRPCHandler Handler);
	bool DispatchRPC(FNetworkRPCId RPCId, ENetRPCType RPCType, FNetBuffer& Payload);
	bool InvokeRPCWithPayload(FNetworkRPCId RPCId, ENetRPCType RPCType, ENetPacketReliability Reliability, const FNetBuffer& Payload);

	template<class TObject, class... TArgs>
	void RegisterRPC(FNetworkRPCId RPCId, ENetRPCType RPCType, TObject* Object, void (TObject::* Method)(TArgs...))
	{
		RegisterRPC(RPCId, RPCType, [Object, Method](FNetBuffer& Payload) {
			std::tuple<std::decay_t<TArgs>...> args;
			if (!ReadRPCTuple(Payload, args)) {
				return;
			}

			std::apply([Object, Method](auto&&... Values) {
				(Object->*Method)(std::forward<decltype(Values)>(Values)...);
			}, args);
		});
	}

	template<class TObject, class... TArgs>
	void RegisterRPC(FNetworkRPCId RPCId, ENetRPCType RPCType, TObject* Object, void (TObject::* Method)(TArgs...) const)
	{
		RegisterRPC(RPCId, RPCType, [Object, Method](FNetBuffer& Payload) {
			std::tuple<std::decay_t<TArgs>...> args;
			if (!ReadRPCTuple(Payload, args)) {
				return;
			}

			std::apply([Object, Method](auto&&... Values) {
				(Object->*Method)(std::forward<decltype(Values)>(Values)...);
			}, args);
		});
	}

	template<class... TArgs>
	bool InvokeRPC(FNetworkRPCId RPCId, ENetRPCType RPCType, ENetPacketReliability Reliability, const TArgs&... Args)
	{
		FNetBuffer payload;
		(WriteRPCArgument(payload, Args), ...);
		return InvokeRPCWithPayload(RPCId, RPCType, Reliability, payload);
	}

protected:
	virtual void OnComponentDestroy() {}

	AActor* Owner = nullptr;
	virtual void OnUpdate(float DeltaTime) {}

private:
	bool bPendingDestroy = false;
	bool bReplicatedStateDirty = false;
	std::string NetComponentName;
	std::vector<std::unique_ptr<IReplicatedProperty>> ReplicatedProperties;
	uint32_t ReplicationSequence = 0;
	uint32_t LastReceivedReplicationSequence = 0;
	struct FRPCEntry
	{
		ENetRPCType Type = ENetRPCType::Server;
		FRPCHandler Handler;
	};
	std::unordered_map<FNetworkRPCId, FRPCEntry> RPCHandlers;

	template<class T>
	static void WriteRPCArgument(FNetBuffer& Payload, const T& Value)
	{
		static_assert(std::is_trivially_copyable_v<T>, "MActorComponent::InvokeRPC supports trivially copyable RPC arguments.");
		Payload.Write(Value);
	}

	template<size_t Index = 0, class... TArgs>
	static bool ReadRPCTuple(FNetBuffer& Payload, std::tuple<TArgs...>& Args)
	{
		if constexpr (Index == sizeof...(TArgs)) {
			return true;
		}
		else {
			if (!Payload.Read(std::get<Index>(Args))) {
				return false;
			}
			return ReadRPCTuple<Index + 1>(Payload, Args);
		}
	}
};
