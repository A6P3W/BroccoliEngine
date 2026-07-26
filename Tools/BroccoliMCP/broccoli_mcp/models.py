"""MCP-independent models for the Automation API."""

from __future__ import annotations

from collections.abc import Mapping
from dataclasses import dataclass
from typing import Any

from .errors import InvalidEngineResponse


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
