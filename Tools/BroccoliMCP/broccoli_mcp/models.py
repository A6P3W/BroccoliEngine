"""MCP-independent models for the Automation API."""

from __future__ import annotations

import math
from collections.abc import Mapping
from dataclasses import dataclass
from typing import Any

from .errors import InvalidEngineResponse

MAX_ACTOR_ID = (1 << 64) - 1


@dataclass(frozen=True, slots=True)
class EngineState:
  """Validated state returned by GET /api/v1/state."""

  SceneName: str
  Fps: float
  Paused: bool
  WorldAvailable: bool
  ActorCount: int

  @classmethod
  def from_mapping(
    Class,
    Data: Mapping[str, Any],
    *,
    Operation: str,
  ) -> EngineState:
    RequiredFields = {
      "sceneName": str,
      "paused": bool,
      "worldAvailable": bool,
      "actorCount": int,
    }
    for FieldName, ExpectedType in RequiredFields.items():
      if FieldName not in Data:
        raise InvalidEngineResponse(
          f"State data is missing required field '{FieldName}'.",
          Operation=Operation,
        )
      FieldValue = Data[FieldName]
      if not isinstance(FieldValue, ExpectedType) or (
        ExpectedType is int and isinstance(FieldValue, bool)
      ):
        raise InvalidEngineResponse(
          f"State field '{FieldName}' has an invalid type.",
          Operation=Operation,
        )

    if "fps" not in Data:
      raise InvalidEngineResponse(
        "State data is missing required field 'fps'.",
        Operation=Operation,
      )
    FpsValue = Data["fps"]
    if isinstance(FpsValue, bool) or not isinstance(FpsValue, (int, float)):
      raise InvalidEngineResponse(
        "State field 'fps' has an invalid type.",
        Operation=Operation,
      )
    if Data["actorCount"] < 0:
      raise InvalidEngineResponse(
        "State field 'actorCount' must not be negative.",
        Operation=Operation,
      )

    return Class(
      SceneName=Data["sceneName"],
      Fps=float(FpsValue),
      Paused=Data["paused"],
      WorldAvailable=Data["worldAvailable"],
      ActorCount=Data["actorCount"],
    )

  def to_dict(Self) -> dict[str, Any]:
    return {
      "sceneName": Self.SceneName,
      "fps": Self.Fps,
      "paused": Self.Paused,
      "worldAvailable": Self.WorldAvailable,
      "actorCount": Self.ActorCount,
    }


def _required_mapping(
  Data: Mapping[str, Any],
  FieldName: str,
  *,
  Operation: str,
) -> Mapping[str, Any]:
  if FieldName not in Data:
    raise InvalidEngineResponse(
      f"Actor data is missing required field '{FieldName}'.",
      Operation=Operation,
    )
  Value = Data[FieldName]
  if not isinstance(Value, Mapping):
    raise InvalidEngineResponse(
      f"Actor field '{FieldName}' has an invalid type.",
      Operation=Operation,
    )
  return Value


def _finite_number(
  Data: Mapping[str, Any],
  FieldName: str,
  *,
  Operation: str,
) -> float:
  if FieldName not in Data:
    raise InvalidEngineResponse(
      f"Actor transform is missing required field '{FieldName}'.",
      Operation=Operation,
    )
  Value = Data[FieldName]
  if isinstance(Value, bool) or not isinstance(Value, (int, float)):
    raise InvalidEngineResponse(
      f"Actor transform field '{FieldName}' has an invalid type.",
      Operation=Operation,
    )
  Result = float(Value)
  if not math.isfinite(Result):
    raise InvalidEngineResponse(
      f"Actor transform field '{FieldName}' must be finite.",
      Operation=Operation,
    )
  return Result


@dataclass(frozen=True, slots=True)
class ActorTransform:
  """Validated two-dimensional actor transform."""

  LocationX: float
  LocationY: float
  Rotation: float
  Scale: float

  @classmethod
  def from_mapping(
    Class,
    Data: Mapping[str, Any],
    *,
    Operation: str,
  ) -> ActorTransform:
    Location = _required_mapping(Data, "location", Operation=Operation)
    return Class(
      LocationX=_finite_number(Location, "x", Operation=Operation),
      LocationY=_finite_number(Location, "y", Operation=Operation),
      Rotation=_finite_number(Data, "rotation", Operation=Operation),
      Scale=_finite_number(Data, "scale", Operation=Operation),
    )

  def to_dict(Self) -> dict[str, Any]:
    return {
      "location": {"x": Self.LocationX, "y": Self.LocationY},
      "rotation": Self.Rotation,
      "scale": Self.Scale,
    }


@dataclass(frozen=True, slots=True)
class ActorInfo:
  """Validated common actor data returned by actor APIs."""

  ActorId: int
  InstanceName: str
  ClassName: str
  Transform: ActorTransform

  @classmethod
  def from_mapping(
    Class,
    Data: Mapping[str, Any],
    *,
    Operation: str,
  ) -> ActorInfo:
    for FieldName in ("actorId", "instanceName", "className", "transform"):
      if FieldName not in Data:
        raise InvalidEngineResponse(
          f"Actor data is missing required field '{FieldName}'.",
          Operation=Operation,
        )

    ActorId = Data["actorId"]
    if (
      isinstance(ActorId, bool) or not isinstance(ActorId, int) or not 1 <= ActorId <= MAX_ACTOR_ID
    ):
      raise InvalidEngineResponse(
        "Actor field 'actorId' must be an unsigned 64-bit integer greater than zero.",
        Operation=Operation,
      )

    InstanceName = Data["instanceName"]
    ClassName = Data["className"]
    if not isinstance(InstanceName, str) or not InstanceName:
      raise InvalidEngineResponse(
        "Actor field 'instanceName' must be a non-empty string.",
        Operation=Operation,
      )
    if not isinstance(ClassName, str) or not ClassName:
      raise InvalidEngineResponse(
        "Actor field 'className' must be a non-empty string.",
        Operation=Operation,
      )

    Transform = _required_mapping(Data, "transform", Operation=Operation)
    return Class(
      ActorId=ActorId,
      InstanceName=InstanceName,
      ClassName=ClassName,
      Transform=ActorTransform.from_mapping(Transform, Operation=Operation),
    )

  def to_dict(Self) -> dict[str, Any]:
    return {
      "actorId": Self.ActorId,
      "instanceName": Self.InstanceName,
      "className": Self.ClassName,
      "transform": Self.Transform.to_dict(),
    }


@dataclass(frozen=True, slots=True)
class ActorList:
  """Validated actor collection returned by GET /api/v1/world/actors."""

  SceneName: str
  ActorCount: int
  Actors: tuple[ActorInfo, ...]

  @classmethod
  def from_mapping(
    Class,
    Data: Mapping[str, Any],
    *,
    Operation: str,
  ) -> ActorList:
    RequiredFields = ("sceneName", "actorCount", "actors")
    for FieldName in RequiredFields:
      if FieldName not in Data:
        raise InvalidEngineResponse(
          f"Actor list is missing required field '{FieldName}'.",
          Operation=Operation,
        )

    SceneName = Data["sceneName"]
    ActorCount = Data["actorCount"]
    ActorData = Data["actors"]
    if not isinstance(SceneName, str):
      raise InvalidEngineResponse(
        "Actor list field 'sceneName' has an invalid type.",
        Operation=Operation,
      )
    if isinstance(ActorCount, bool) or not isinstance(ActorCount, int) or ActorCount < 0:
      raise InvalidEngineResponse(
        "Actor list field 'actorCount' must be a non-negative integer.",
        Operation=Operation,
      )
    if not isinstance(ActorData, list):
      raise InvalidEngineResponse(
        "Actor list field 'actors' has an invalid type.",
        Operation=Operation,
      )

    Actors = []
    ActorIds = set()
    for Item in ActorData:
      if not isinstance(Item, Mapping):
        raise InvalidEngineResponse(
          "Actor list contains an invalid actor item.",
          Operation=Operation,
        )
      Actor = ActorInfo.from_mapping(Item, Operation=Operation)
      if Actor.ActorId in ActorIds:
        raise InvalidEngineResponse(
          "Actor list contains duplicate actorId values.",
          Operation=Operation,
        )
      ActorIds.add(Actor.ActorId)
      Actors.append(Actor)

    if ActorCount != len(Actors):
      raise InvalidEngineResponse(
        "Actor list field 'actorCount' does not match the actors array.",
        Operation=Operation,
      )
    return Class(SceneName=SceneName, ActorCount=ActorCount, Actors=tuple(Actors))

  def to_dict(Self) -> dict[str, Any]:
    return {
      "sceneName": Self.SceneName,
      "actorCount": Self.ActorCount,
      "actors": [Actor.to_dict() for Actor in Self.Actors],
    }
