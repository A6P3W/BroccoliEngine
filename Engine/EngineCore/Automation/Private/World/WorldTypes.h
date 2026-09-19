#pragma once

#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "ActorId.h"
#include "ComponentId.h"
#include "UMath.h"

class AActor;
class MActorComponent;

struct FAutomationActorSnapshot {
  FActorId ActorId = InvalidActorId;
  std::string InstanceName;
  std::string ClassName;
  FVector3D Location;
  FRotator3D Rotation;
  FScale3D Scale;
};

struct FAutomationActorComponentSnapshot {
  FComponentId ComponentId = InvalidComponentId;
  std::string Name;
  std::string ClassName;
  bool Registered = false;
  bool PendingDestroy = false;
  bool Replicates = false;
  uint32_t NetworkId = 0;
};

struct FAutomationActorComponentListSnapshot {
  FActorId ActorId = InvalidActorId;
  std::string ClassName;
  std::vector<FAutomationActorComponentSnapshot> Components;
};

struct FAutomationActorListSnapshot {
  std::string SceneName;
  std::vector<FAutomationActorSnapshot> Actors;
};

struct FAutomationWorldStateSnapshot {
  std::string SceneName;
  float Fps = 0.0f;
  bool WorldAvailable = false;
  uint32_t ActorCount = 0;
  bool Physics3DAvailable = false;
  uint32_t Physics3DBodyCount = 0;
};

struct FAutomationActorQuery {
  std::optional<std::string> ClassName;
  std::optional<std::string> InstanceName;
};

enum class EAutomationWorldReadStatus : uint8_t {
  Success,
  WorldNotAvailable,
  ActorNotFound,
  InvalidState
};

enum class EAutomationActorResolveStatus : uint8_t {
  Success,
  WorldNotAvailable,
  ActorNotFound,
  ActorPendingDestroy,
  InvalidState
};

enum class EAutomationComponentResolveStatus : uint8_t {
  Success,
  WorldNotAvailable,
  ActorNotFound,
  ActorPendingDestroy,
  ComponentNotFound,
  ComponentPendingDestroy,
  InvalidState
};

enum class EAutomationWorldMutationStatus : uint8_t {
  Success,
  WorldNotAvailable,
  ClassNotRegistered,
  ActorNotFound,
  ActorPendingDestroy,
  InvalidState
};

struct FAutomationSpawnActorRequest {
  std::string ClassName;
  FVector3D Location = FVector3D::ZeroVector();
  FRotator3D Rotation = FRotator3D{};
  FScale3D Scale = FScale3D{1.0f, 1.0f, 1.0f};
  std::optional<std::string> InstanceName;
};

struct FOptionalVector3D {
  std::optional<float> X;
  std::optional<float> Y;
  std::optional<float> Z;

  bool HasAnyValue() const { return X.has_value() || Y.has_value() || Z.has_value(); }
};

struct FOptionalRotator3D {
  std::optional<float> Pitch;
  std::optional<float> Yaw;
  std::optional<float> Roll;

  bool HasAnyValue() const { return Pitch.has_value() || Yaw.has_value() || Roll.has_value(); }
};

struct FOptionalScale3D {
  std::optional<float> X;
  std::optional<float> Y;
  std::optional<float> Z;

  bool HasAnyValue() const { return X.has_value() || Y.has_value() || Z.has_value(); }
};

struct FAutomationTransformPatch {
  FOptionalVector3D Location;
  FOptionalRotator3D Rotation;
  FOptionalScale3D Scale;

  bool HasAnyValue() const {
    return Location.HasAnyValue() || Rotation.HasAnyValue() || Scale.HasAnyValue();
  }
};

using FAutomationActorListProvider = std::function<
    EAutomationWorldReadStatus(const FAutomationActorQuery&, FAutomationActorListSnapshot&)>;
using FAutomationWorldStateProvider = std::function<FAutomationWorldStateSnapshot()>;
using FAutomationActorProvider =
    std::function<EAutomationWorldReadStatus(FActorId, FAutomationActorSnapshot&)>;
using FAutomationActorComponentListProvider =
    std::function<EAutomationWorldReadStatus(FActorId, FAutomationActorComponentListSnapshot&)>;
using FAutomationActorResolver = std::function<EAutomationActorResolveStatus(FActorId, AActor*&)>;
using FAutomationComponentResolver =
    std::function<EAutomationComponentResolveStatus(FActorId, FComponentId, MActorComponent*&)>;
using FAutomationSpawnActorProvider = std::function<
    EAutomationWorldMutationStatus(const FAutomationSpawnActorRequest&, FAutomationActorSnapshot&)>;
using FAutomationDestroyActorProvider = std::function<EAutomationWorldMutationStatus(FActorId)>;
using FAutomationPatchActorTransformProvider = std::function<EAutomationWorldMutationStatus(
    FActorId, const FAutomationTransformPatch&, FAutomationActorSnapshot&
)>;
