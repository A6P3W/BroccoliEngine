from __future__ import annotations

import math

import pytest

from broccoli_mcp.errors import InvalidEngineResponse
from broccoli_mcp.models import ActorInfo, ActorList

ACTOR_DATA = {
  "actorId": 42,
  "instanceName": "AForceFieldActor_1",
  "className": "AForceFieldActor",
  "transform": {
    "location": {"x": 100.0, "y": 200.0},
    "rotation": 45.0,
    "scale": 1.0,
  },
}


def test_actor_model_accepts_valid_data_and_ignores_unknown_fields() -> None:
  Actor = ActorInfo.from_mapping(
    {**ACTOR_DATA, "future": True},
    Operation="test actor",
  )

  assert Actor.ActorId == 42
  assert Actor.to_dict() == ACTOR_DATA


@pytest.mark.parametrize("ActorId", [True, 0, 1 << 64])
def test_actor_model_rejects_invalid_actor_id(ActorId: object) -> None:
  with pytest.raises(InvalidEngineResponse, match="actorId"):
    ActorInfo.from_mapping(
      {**ACTOR_DATA, "actorId": ActorId},
      Operation="test actor",
    )


def test_actor_model_rejects_missing_transform() -> None:
  Data = dict(ACTOR_DATA)
  del Data["transform"]

  with pytest.raises(InvalidEngineResponse, match="transform"):
    ActorInfo.from_mapping(Data, Operation="test actor")


@pytest.mark.parametrize("Value", [math.nan, math.inf, -math.inf])
def test_actor_model_rejects_non_finite_transform(Value: float) -> None:
  Data = {
    **ACTOR_DATA,
    "transform": {
      **ACTOR_DATA["transform"],
      "scale": Value,
    },
  }

  with pytest.raises(InvalidEngineResponse, match="finite"):
    ActorInfo.from_mapping(Data, Operation="test actor")


def test_actor_list_rejects_count_mismatch() -> None:
  with pytest.raises(InvalidEngineResponse, match="does not match"):
    ActorList.from_mapping(
      {"sceneName": "BasicGameplay", "actorCount": 0, "actors": [ACTOR_DATA]},
      Operation="test actors",
    )


def test_actor_list_rejects_duplicate_ids() -> None:
  with pytest.raises(InvalidEngineResponse, match="duplicate"):
    ActorList.from_mapping(
      {
        "sceneName": "BasicGameplay",
        "actorCount": 2,
        "actors": [ACTOR_DATA, ACTOR_DATA],
      },
      Operation="test actors",
    )
