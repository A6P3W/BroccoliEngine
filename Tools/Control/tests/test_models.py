from __future__ import annotations

import math

import pytest

from broccoli_control.errors import InvalidEngineResponse
from broccoli_control.models import (
  ActorInfo,
  ActorList,
  ActorMethodInfo,
  RecentLogs,
  SystemCommandInfo,
)

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
LOG_DATA = {
  "entries": [
    {
      "sequence": 125,
      "timestamp": "2026-07-26T08:30:15.123Z",
      "level": "warning",
      "category": "InitializeAutomation",
      "message": "Automation server startup failed.",
    }
  ],
  "count": 1,
  "oldestAvailableSequence": 42,
  "latestSequence": 125,
  "nextAfterSequence": 125,
  "droppedEntries": 0,
  "historyLost": False,
  "hasMore": False,
}


def test_actor_models_accept_valid_data_and_ignore_unknown_fields() -> None:
  Actor = ActorInfo.from_mapping({**ACTOR_DATA, "future": True}, Operation="test actor")
  Actors = ActorList.from_mapping(
    {"sceneName": "BasicGameplay", "actorCount": 1, "actors": [ACTOR_DATA]},
    Operation="test actors",
  )

  assert Actor.to_dict() == ACTOR_DATA
  assert Actors.Actors == (Actor,)


@pytest.mark.parametrize("ActorId", [True, 0, 1 << 64])
def test_actor_model_rejects_invalid_actor_id(ActorId: object) -> None:
  with pytest.raises(InvalidEngineResponse, match="actorId"):
    ActorInfo.from_mapping({**ACTOR_DATA, "actorId": ActorId}, Operation="test actor")


@pytest.mark.parametrize("Value", [math.nan, math.inf, -math.inf])
def test_actor_model_rejects_non_finite_transform(Value: float) -> None:
  Data = {**ACTOR_DATA, "transform": {**ACTOR_DATA["transform"], "scale": Value}}

  with pytest.raises(InvalidEngineResponse, match="finite"):
    ActorInfo.from_mapping(Data, Operation="test actor")


def test_actor_list_rejects_inconsistent_data() -> None:
  with pytest.raises(InvalidEngineResponse, match="does not match"):
    ActorList.from_mapping(
      {"sceneName": "BasicGameplay", "actorCount": 0, "actors": [ACTOR_DATA]},
      Operation="test actors",
    )

  with pytest.raises(InvalidEngineResponse, match="duplicate"):
    ActorList.from_mapping(
      {"sceneName": "BasicGameplay", "actorCount": 2, "actors": [ACTOR_DATA, ACTOR_DATA]},
      Operation="test actors",
    )


def test_automation_descriptors_reject_invalid_values() -> None:
  with pytest.raises(InvalidEngineResponse):
    ActorMethodInfo.from_mapping(
      {
        "name": "BadName",
        "description": "Test.",
        "inputSchema": {},
        "permission": "ReadOnly",
      },
      Operation="test actor method",
    )

  with pytest.raises(InvalidEngineResponse):
    SystemCommandInfo.from_mapping(
      {
        "name": "pause_game",
        "description": "",
        "inputSchema": {},
        "permission": "SystemMutation",
      },
      Operation="test system command",
    )


def test_recent_logs_reject_inconsistent_count() -> None:
  assert RecentLogs.from_mapping(LOG_DATA, Operation="test logs").to_dict() == LOG_DATA

  with pytest.raises(InvalidEngineResponse, match="count"):
    RecentLogs.from_mapping({**LOG_DATA, "count": 2}, Operation="test logs")
