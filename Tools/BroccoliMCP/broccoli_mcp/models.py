"""MCP-independent models for the Automation API."""

from __future__ import annotations

import math
from collections.abc import Mapping
from dataclasses import dataclass
from typing import Any

from .errors import InvalidEngineResponse

MAX_ACTOR_ID = (1 << 64) - 1
LOG_LEVELS = frozenset({"debug", "info", "warning", "error"})


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


def _optional_finite_number(Value: object, FieldName: str) -> float | None:
  if Value is None:
    return None
  if isinstance(Value, bool) or not isinstance(Value, (int, float)):
    raise ValueError(f"{FieldName} must be a number.")
  Result = float(Value)
  if not math.isfinite(Result):
    raise ValueError(f"{FieldName} must be finite.")
  return Result


@dataclass(frozen=True, slots=True)
class TransformPatch:
  """Validated transform values for PATCH /world/actors/{actorId}/transform."""

  LocationX: float | None = None
  LocationY: float | None = None
  Rotation: float | None = None
  Scale: float | None = None

  def __post_init__(Self) -> None:
    LocationX = _optional_finite_number(Self.LocationX, "location_x")
    LocationY = _optional_finite_number(Self.LocationY, "location_y")
    Rotation = _optional_finite_number(Self.Rotation, "rotation")
    Scale = _optional_finite_number(Self.Scale, "scale")
    if (LocationX is None) != (LocationY is None):
      raise ValueError("location_x and location_y must be provided together.")
    if LocationX is None and Rotation is None and Scale is None:
      raise ValueError("At least one transform value must be provided.")
    if Scale is not None and Scale <= 0.0:
      raise ValueError("scale must be greater than zero.")

    object.__setattr__(Self, "LocationX", LocationX)
    object.__setattr__(Self, "LocationY", LocationY)
    object.__setattr__(Self, "Rotation", Rotation)
    object.__setattr__(Self, "Scale", Scale)

  def to_dict(Self) -> dict[str, Any]:
    Result: dict[str, Any] = {}
    if Self.LocationX is not None and Self.LocationY is not None:
      Result["location"] = {"x": Self.LocationX, "y": Self.LocationY}
    if Self.Rotation is not None:
      Result["rotation"] = Self.Rotation
    if Self.Scale is not None:
      Result["scale"] = Self.Scale
    return Result


@dataclass(frozen=True, slots=True)
class DestroyActorResult:
  """Validated result returned by DELETE /world/actors/{actorId}."""

  ActorId: int
  PendingDestroy: bool

  @classmethod
  def from_mapping(
    Class,
    Data: Mapping[str, Any],
    *,
    Operation: str,
  ) -> DestroyActorResult:
    ActorId = Data.get("actorId")
    PendingDestroy = Data.get("pendingDestroy")
    if (
      isinstance(ActorId, bool) or not isinstance(ActorId, int) or not 1 <= ActorId <= MAX_ACTOR_ID
    ):
      raise InvalidEngineResponse(
        "Destroy result field 'actorId' must be an unsigned 64-bit integer greater than zero.",
        Operation=Operation,
      )
    if PendingDestroy is not True:
      raise InvalidEngineResponse(
        "Destroy result field 'pendingDestroy' must be true.",
        Operation=Operation,
      )
    return Class(ActorId=ActorId, PendingDestroy=PendingDestroy)

  def to_dict(Self) -> dict[str, Any]:
    return {
      "actorId": Self.ActorId,
      "pendingDestroy": Self.PendingDestroy,
    }


@dataclass(frozen=True, slots=True)
class LogEntry:
  """Validated in-memory engine log entry."""

  Sequence: int
  Timestamp: str
  Level: str
  Category: str
  Message: str

  @classmethod
  def from_mapping(
    Class,
    Data: Mapping[str, Any],
    *,
    Operation: str,
  ) -> LogEntry:
    RequiredFields = ("sequence", "timestamp", "level", "category", "message")
    for FieldName in RequiredFields:
      if FieldName not in Data:
        raise InvalidEngineResponse(
          f"Log entry is missing required field '{FieldName}'.",
          Operation=Operation,
        )

    Sequence = Data["sequence"]
    if (
      isinstance(Sequence, bool)
      or not isinstance(Sequence, int)
      or not 1 <= Sequence <= MAX_ACTOR_ID
    ):
      raise InvalidEngineResponse(
        "Log entry field 'sequence' must be an unsigned 64-bit integer greater than zero.",
        Operation=Operation,
      )

    Timestamp = Data["timestamp"]
    Level = Data["level"]
    Category = Data["category"]
    Message = Data["message"]
    if not isinstance(Timestamp, str) or not Timestamp:
      raise InvalidEngineResponse(
        "Log entry field 'timestamp' must be a non-empty string.",
        Operation=Operation,
      )
    if not isinstance(Level, str) or Level not in LOG_LEVELS:
      raise InvalidEngineResponse(
        "Log entry field 'level' is invalid.",
        Operation=Operation,
      )
    if not isinstance(Category, str):
      raise InvalidEngineResponse(
        "Log entry field 'category' must be a string.",
        Operation=Operation,
      )
    if not isinstance(Message, str):
      raise InvalidEngineResponse(
        "Log entry field 'message' must be a string.",
        Operation=Operation,
      )
    return Class(Sequence, Timestamp, Level, Category, Message)

  def to_dict(Self) -> dict[str, Any]:
    return {
      "sequence": Self.Sequence,
      "timestamp": Self.Timestamp,
      "level": Self.Level,
      "category": Self.Category,
      "message": Self.Message,
    }


def _uint64_field(Data: Mapping[str, Any], FieldName: str, Operation: str) -> int:
  Value = Data.get(FieldName)
  if isinstance(Value, bool) or not isinstance(Value, int) or not 0 <= Value <= MAX_ACTOR_ID:
    raise InvalidEngineResponse(
      f"Recent logs field '{FieldName}' must be an unsigned 64-bit integer.",
      Operation=Operation,
    )
  return Value


@dataclass(frozen=True, slots=True)
class RecentLogs:
  """Validated result returned by GET /api/v1/logs/recent."""

  Entries: tuple[LogEntry, ...]
  Count: int
  OldestAvailableSequence: int
  LatestSequence: int
  NextAfterSequence: int
  HistoryLost: bool
  HasMore: bool

  @classmethod
  def from_mapping(
    Class,
    Data: Mapping[str, Any],
    *,
    Operation: str,
  ) -> RecentLogs:
    RequiredFields = (
      "entries",
      "count",
      "oldestAvailableSequence",
      "latestSequence",
      "nextAfterSequence",
      "historyLost",
      "hasMore",
    )
    for FieldName in RequiredFields:
      if FieldName not in Data:
        raise InvalidEngineResponse(
          f"Recent logs data is missing required field '{FieldName}'.",
          Operation=Operation,
        )

    EntryData = Data["entries"]
    Count = Data["count"]
    if not isinstance(EntryData, list):
      raise InvalidEngineResponse(
        "Recent logs field 'entries' must be an array.",
        Operation=Operation,
      )
    if isinstance(Count, bool) or not isinstance(Count, int) or Count < 0:
      raise InvalidEngineResponse(
        "Recent logs field 'count' must be a non-negative integer.",
        Operation=Operation,
      )

    Entries = []
    PreviousSequence = 0
    for Item in EntryData:
      if not isinstance(Item, Mapping):
        raise InvalidEngineResponse(
          "Recent logs contains an invalid entry.",
          Operation=Operation,
        )
      Entry = LogEntry.from_mapping(Item, Operation=Operation)
      if Entry.Sequence <= PreviousSequence:
        raise InvalidEngineResponse(
          "Recent log sequences must be strictly increasing.",
          Operation=Operation,
        )
      PreviousSequence = Entry.Sequence
      Entries.append(Entry)
    if Count != len(Entries):
      raise InvalidEngineResponse(
        "Recent logs field 'count' does not match the entries array.",
        Operation=Operation,
      )

    HistoryLost = Data["historyLost"]
    HasMore = Data["hasMore"]
    if not isinstance(HistoryLost, bool) or not isinstance(HasMore, bool):
      raise InvalidEngineResponse(
        "Recent logs history flags must be boolean values.",
        Operation=Operation,
      )

    return Class(
      Entries=tuple(Entries),
      Count=Count,
      OldestAvailableSequence=_uint64_field(
        Data,
        "oldestAvailableSequence",
        Operation,
      ),
      LatestSequence=_uint64_field(Data, "latestSequence", Operation),
      NextAfterSequence=_uint64_field(Data, "nextAfterSequence", Operation),
      HistoryLost=HistoryLost,
      HasMore=HasMore,
    )

  def to_dict(Self) -> dict[str, Any]:
    return {
      "entries": [Entry.to_dict() for Entry in Self.Entries],
      "count": Self.Count,
      "oldestAvailableSequence": Self.OldestAvailableSequence,
      "latestSequence": Self.LatestSequence,
      "nextAfterSequence": Self.NextAfterSequence,
      "historyLost": Self.HistoryLost,
      "hasMore": Self.HasMore,
    }
