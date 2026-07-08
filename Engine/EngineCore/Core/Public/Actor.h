#pragma once
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

#include "ActorComponent.h"
#include "ActorRegistry.h"
#include "BaseObject.h"
#include "NetBuffer.h"
#include "NetworkManager.h"
#include "NetworkTypes.h"
#include "ReplicatedProperty.h"
#include "SceneComponent.h"
#include "UMath.h"

template <class T>
struct TActorAutoRegister {
  TActorAutoRegister(bool bIsGameMode = false) {
    ActorRegistry::GetInstance().Register<T>(bIsGameMode);
  }
};

#define REGISTER_ACTOR(ClassName) static TActorAutoRegister<ClassName> AutoRegister_##ClassName;

#define DEFINE_ACTOR_CLASS(ClassName)                         \
 public:                                                      \
  static std::string StaticClassName() { return #ClassName; } \
  virtual std::string GetActorClassName() const override { return #ClassName; }

class MSceneComponent;
class FTimerManager;
class World;
class AActor : public MBaseObject

{
 public:
  AActor();
  virtual ~AActor() override;

  void Spawned();

  virtual std::string GetActorClassName() const = 0;

  std::vector<std::string> Tags;
  FNetworkActorId NetworkId = 0;
  bool bReplicates = false;

  bool bHasAuthority = true;
  bool bIsLocallyControlled = false;
  FNetworkConnectionId OwnerConnectionId = 0;
  bool HasTag(std::string_view Tag) const;

  virtual void Update(float DeltaTime) final;
  virtual void Draw();
  MSceneComponent* GetRootComponent() const { return RootComponent; };
  FTimerManager& GetWorldTimerManager();

  const std::vector<std::unique_ptr<MActorComponent>>& GetComponents() const;

  void AddComponent(std::unique_ptr<MActorComponent> comp);

  MActorComponent* FindReplicatedComponent(FNetworkComponentId ComponentNetworkId) const;
  MActorComponent* FindReplicatedComponentByName(std::string_view NetComponentName) const;
  void AssignNetworkComponentIds();

  FVector2D GetActorLocation() const;
  bool SetActorLocation(const FVector2D& NewLocation);

  FRotator GetActorRotation() const;
  bool SetActorRotation(const FRotator& NewRotation);

  FScale GetActorScale() const;
  bool SetActorScale(FScale NewScale);

  void AddActorWorldOffset(const FVector2D& Offset);
  void AddActorLocalOffset(const FVector2D& Offset);
  void AddActorRotation(const FRotator& DeltaRotation);

  void Destroy();
  bool IsPendingDestroy() const;

  virtual void BeginOverlap(AActor* OtherActor) {}
  virtual void EndOverlap(AActor* OtherActor) {}

  virtual bool SerializeNetworkState(FNetBuffer& OutBuffer);
  virtual bool DeserializeNetworkState(FNetBuffer& InBuffer);

  virtual bool SerializeNetworkSpawn(FNetBuffer& OutBuffer);
  virtual bool DeserializeNetworkSpawn(FNetBuffer& InBuffer);

  bool HasReplicatedStateChanged(float Tolerance = 0.001f) const;
  void UpdateReplicatedStateCache();
  void MarkReplicatedStateDirty();
  uint32_t IncrementReplicationSequence();
  uint32_t GetReplicationSequence() const { return ReplicationSequence; }
  uint32_t GetLastReceivedReplicationSequence() const { return LastReceivedReplicationSequence; }
  void SetLastReceivedReplicationSequence(uint32_t Sequence);

  template <class T>
  void RegisterReplicatedProperty(T* Property) {
    if (!Property) {
      return;
    }

    ReplicatedProperties.push_back(std::make_unique<TReplicatedProperty<T>>(Property));
    MarkReplicatedStateDirty();
  }

  template <class T, class TObject>
  void RegisterReplicatedProperty(T* Property, TObject* Object, void (TObject::*OnRep)(T)) {
    if (!Property || !Object || !OnRep) {
      return;
    }

    ReplicatedProperties.push_back(
        std::make_unique<TReplicatedProperty<T, TObject>>(Property, Object, OnRep)
    );
    MarkReplicatedStateDirty();
  }

  using FRPCHandler = std::function<void(FNetBuffer&)>;
  void RegisterRPC(FNetworkRPCId RPCId, ENetRPCType RPCType, FRPCHandler Handler);
  bool DispatchRPC(FNetworkRPCId RPCId, ENetRPCType RPCType, FNetBuffer& Payload);
  bool DispatchRPC(
      FNetworkComponentId ComponentNetworkId,
      FNetworkRPCId RPCId,
      ENetRPCType RPCType,
      FNetBuffer& Payload
  );
  bool InvokeRPCWithPayload(
      FNetworkRPCId RPCId,
      ENetRPCType RPCType,
      ENetPacketReliability Reliability,
      const FNetBuffer& Payload
  );
  bool InvokeComponentRPCWithPayload(
      MActorComponent* Component,
      FNetworkRPCId RPCId,
      ENetRPCType RPCType,
      ENetPacketReliability Reliability,
      const FNetBuffer& Payload
  );

  template <class TObject, class... TArgs>
  void RegisterRPC(
      FNetworkRPCId RPCId, ENetRPCType RPCType, TObject* Object, void (TObject::*Method)(TArgs...)
  ) {
    RegisterRPC(RPCId, RPCType, [Object, Method](FNetBuffer& Payload) {
      std::tuple<std::decay_t<TArgs>...> args;
      if (!ReadRPCTuple(Payload, args)) {
        return;
      }

      std::apply(
          [Object, Method](auto&&... Values) {
            (Object->*Method)(std::forward<decltype(Values)>(Values)...);
          },
          args
      );
    });
  }

  template <class TObject, class... TArgs>
  void RegisterRPC(
      FNetworkRPCId RPCId,
      ENetRPCType RPCType,
      TObject* Object,
      void (TObject::*Method)(TArgs...) const
  ) {
    RegisterRPC(RPCId, RPCType, [Object, Method](FNetBuffer& Payload) {
      std::tuple<std::decay_t<TArgs>...> args;
      if (!ReadRPCTuple(Payload, args)) {
        return;
      }

      std::apply(
          [Object, Method](auto&&... Values) {
            (Object->*Method)(std::forward<decltype(Values)>(Values)...);
          },
          args
      );
    });
  }

  template <class... TArgs>
  bool InvokeRPC(
      FNetworkRPCId RPCId,
      ENetRPCType RPCType,
      ENetPacketReliability Reliability,
      const TArgs&... Args
  ) {
    FNetBuffer payload;
    (WriteRPCArgument(payload, Args), ...);
    return InvokeRPCWithPayload(RPCId, RPCType, Reliability, payload);
  }

  template <class T>
  std::vector<T*> GetComponents() const {
    std::vector<T*> results;
    for (const auto& comp : Components) {
      if (auto casted = dynamic_cast<T*>(comp.get())) {
        results.push_back(casted);
      }
    }
    return results;
  }
  World* GetWorld() { return OwnerWorld; }
  void SetWorld(World* world);

  bool IsEditorActor() const { return bEditorActor; }
  bool CanUpdateAnytime() const { return bUpdateableAnytime; }
  void SetUpdateableAnytime(bool bTick) { bUpdateableAnytime = bTick; }

 protected:
  virtual void BeginPlay() {}
  virtual void OnUpdate(float DeltaTime);
  MSceneComponent* RootComponent = nullptr;
  void SetRootComponent(MSceneComponent* Component);

  bool bEditorActor = false;
  bool bUpdateableAnytime = false;

 private:
  std::vector<AActor*> ChildObjects;
  bool bPendingDestroy = false;
  std::vector<std::unique_ptr<MActorComponent>> Components;
  World* OwnerWorld = nullptr;
  FVector2D LastReplicatedLocation = FVector2D::ZeroVector;
  FRotator LastReplicatedRotation;
  FScale LastReplicatedScale;
  bool bHasReplicatedStateCache = false;
  bool bReplicatedStateDirty = false;
  std::vector<std::unique_ptr<IReplicatedProperty>> ReplicatedProperties;
  uint32_t ReplicationSequence = 0;
  uint32_t LastReceivedReplicationSequence = 0;
  FNetworkComponentId NextNetworkComponentId = 1;
  std::unordered_map<FNetworkComponentId, MActorComponent*> ComponentsByNetworkId;
  struct FRPCEntry {
    ENetRPCType Type = ENetRPCType::Server;
    FRPCHandler Handler;
  };
  std::unordered_map<FNetworkRPCId, FRPCEntry> RPCHandlers;

  void EnsureNetComponentName(MActorComponent* Component);
  std::vector<MActorComponent*> GetReplicatedNetworkComponents() const;
  void SerializeActorNetworkState(FNetBuffer& OutBuffer);
  bool DeserializeActorNetworkState(FNetBuffer& InBuffer, bool bApplyTransform = true);
  bool SerializeReplicatedComponentStates(FNetBuffer& OutBuffer);
  bool DeserializeReplicatedComponentStates(FNetBuffer& InBuffer);
  bool SerializeReplicatedComponentSpawns(FNetBuffer& OutBuffer);
  bool DeserializeReplicatedComponentSpawns(FNetBuffer& InBuffer);

  template <class T>
  static void WriteRPCArgument(FNetBuffer& Payload, const T& Value) {
    if constexpr (std::is_same_v<std::decay_t<T>, std::string>) {
      Payload.WriteString(Value);
    } else {
      static_assert(
          std::is_trivially_copyable_v<T>,
          "AActor::InvokeRPC supports trivially copyable RPC arguments."
      );
      Payload.Write(Value);
    }
  }

  template <class T>
  static bool ReadRPCArgument(FNetBuffer& Payload, T& OutValue) {
    if constexpr (std::is_same_v<std::decay_t<T>, std::string>) {
      return Payload.ReadString(OutValue);
    } else {
      static_assert(
          std::is_trivially_copyable_v<T>,
          "AActor::RegisterRPC supports trivially copyable RPC arguments."
      );
      return Payload.Read(OutValue);
    }
  }

  template <size_t Index = 0, class... TArgs>
  static bool ReadRPCTuple(FNetBuffer& Payload, std::tuple<TArgs...>& Args) {
    if constexpr (Index == sizeof...(TArgs)) {
      return true;
    } else {
      if (!ReadRPCArgument(Payload, std::get<Index>(Args))) {
        return false;
      }
      return ReadRPCTuple<Index + 1>(Payload, Args);
    }
  }
};
