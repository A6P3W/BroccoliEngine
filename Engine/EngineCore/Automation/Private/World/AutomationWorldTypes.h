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
  FVector2D Location;
  FRotator Rotation;
  FScale Scale;
};

struct FAutomationActorComponentSnapshot {
  FComponentId ComponentId = InvalidComponentId;
  std::string Name;
  std::string ClassName;
  bool bRegistered = false;
  bool bPendingDestroy = false;
  bool bReplicates = false;
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
  FVector2D Location = FVector2D::ZeroVector();
  FRotator Rotation = FRotator(0.0f);
  FScale Scale = FScale(1.0f);
  std::optional<std::string> InstanceName;
};

struct FAutomationTransformPatch {
  std::optional<FVector2D> Location;
  std::optional<FRotator> Rotation;
  std::optional<FScale> Scale;

  bool HasAnyValue() const {
    return Location.has_value() || Rotation.has_value() || Scale.has_value();
  }
};

using FAutomationActorListProvider = std::function<
    EAutomationWorldReadStatus(const FAutomationActorQuery&, FAutomationActorListSnapshot&)>;
using FAutomationActorProvider =
    std::function<EAutomationWorldReadStatus(FActorId, FAutomationActorSnapshot&)>;
using FAutomationActorComponentListProvider =
    std::function<EAutomationWorldReadStatus(FActorId, FAutomationActorComponentListSnapshot&)>;
using FAutomationActorResolver = std::function<EAutomationActorResolveStatus(FActorId, AActor*&)>;
using FAutomationComponentResolver = std::function<EAutomationComponentResolveStatus(
    FActorId, FComponentId, MActorComponent*&
)>;
using FAutomationSpawnActorProvider = std::function<
    EAutomationWorldMutationStatus(const FAutomationSpawnActorRequest&, FAutomationActorSnapshot&)>;
using FAutomationDestroyActorProvider = std::function<EAutomationWorldMutationStatus(FActorId)>;
using FAutomationPatchActorTransformProvider = std::function<EAutomationWorldMutationStatus(
    FActorId, const FAutomationTransformPatch&, FAutomationActorSnapshot&
)>;
