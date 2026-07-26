from __future__ import annotations

import json
import math

import httpx
import pytest

from broccoli_mcp.config import BridgeConfig
from broccoli_mcp.engine_client import EngineClient
from broccoli_mcp.models import ActorInfo, ActorTransform, DestroyActorResult, TransformPatch
from broccoli_mcp.server import (
  create_server,
  destroy_actor_tool,
  set_actor_transform_tool,
  spawn_actor_tool,
)

ACTOR_DATA = {
  "actorId": 42,
  "instanceName": "SpawnedActor",
  "className": "AForceFieldActor",
  "transform": {
    "location": {"x": 100.0, "y": 200.0},
    "rotation": 45.0,
    "scale": 2.0,
  },
}


def json_response(Status: int, Data: object) -> httpx.Response:
  return httpx.Response(
    Status,
    json={"success": True, "data": Data},
    headers={"Content-Type": "application/json"},
  )


def test_spawn_actor_sends_validated_json_and_accepts_201() -> None:
  def handler(Request: httpx.Request) -> httpx.Response:
    assert Request.method == "POST"
    assert Request.url == "http://127.0.0.1:39100/api/v1/world/actors"
    assert json.loads(Request.content) == {
      "className": "AForceFieldActor",
      "transform": {
        "location": {"x": 100.0, "y": 200.0},
        "rotation": 45.0,
        "scale": 2.0,
      },
      "instanceName": "SpawnedActor",
    }
    return json_response(201, ACTOR_DATA)

  Client = EngineClient(BridgeConfig(), Transport=httpx.MockTransport(handler))
  try:
    Actor = Client.spawn_actor(
      "AForceFieldActor",
      LocationX=100.0,
      LocationY=200.0,
      Rotation=45.0,
      Scale=2.0,
      InstanceName="SpawnedActor",
    )
  finally:
    Client.close()

  assert Actor.ActorId == 42


def test_destroy_actor_sends_delete_and_validates_result() -> None:
  def handler(Request: httpx.Request) -> httpx.Response:
    assert Request.method == "DELETE"
    assert Request.url == "http://127.0.0.1:39100/api/v1/world/actors/42"
    return json_response(200, {"actorId": 42, "pendingDestroy": True})

  Client = EngineClient(BridgeConfig(), Transport=httpx.MockTransport(handler))
  try:
    Result = Client.destroy_actor(42)
  finally:
    Client.close()

  assert Result == DestroyActorResult(ActorId=42, PendingDestroy=True)


def test_set_actor_transform_sends_only_requested_values() -> None:
  def handler(Request: httpx.Request) -> httpx.Response:
    assert Request.method == "PATCH"
    assert Request.url == "http://127.0.0.1:39100/api/v1/world/actors/42/transform"
    assert json.loads(Request.content) == {"rotation": 45.0}
    return json_response(200, ACTOR_DATA)

  Client = EngineClient(BridgeConfig(), Transport=httpx.MockTransport(handler))
  try:
    Actor = Client.set_actor_transform(42, Rotation=45.0)
  finally:
    Client.close()

  assert Actor.Transform.Rotation == 45.0


@pytest.mark.parametrize(
  "Arguments",
  [
    {},
    {"LocationX": 1.0},
    {"Scale": 0.0},
    {"Rotation": math.inf},
  ],
)
def test_transform_patch_rejects_invalid_values(Arguments: dict[str, float]) -> None:
  with pytest.raises(ValueError):
    TransformPatch(**Arguments)


class FakeMutationClient:
  def spawn_actor(Self, ClassName: str, **Arguments: object) -> ActorInfo:
    assert ClassName == "ATestActor"
    assert Arguments["LocationX"] == 1.0
    return ActorInfo(
      ActorId=7,
      InstanceName="ATestActor_1",
      ClassName=ClassName,
      Transform=ActorTransform(1.0, 2.0, 3.0, 1.0),
    )

  def destroy_actor(Self, ActorId: int) -> DestroyActorResult:
    return DestroyActorResult(ActorId=ActorId, PendingDestroy=True)

  def set_actor_transform(Self, ActorId: int, **Arguments: object) -> ActorInfo:
    assert ActorId == 7
    assert Arguments["Rotation"] == 90.0
    return ActorInfo(
      ActorId=ActorId,
      InstanceName="ATestActor_1",
      ClassName="ATestActor",
      Transform=ActorTransform(1.0, 2.0, 90.0, 1.0),
    )


def test_actor_tools_return_json_objects_and_are_registered() -> None:
  Client = FakeMutationClient()

  assert spawn_actor_tool(Client, class_name="ATestActor", location_x=1.0)["actorId"] == 7
  assert destroy_actor_tool(Client, actor_id=7) == {"actorId": 7, "pendingDestroy": True}
  assert (
    set_actor_transform_tool(Client, actor_id=7, rotation=90.0)["transform"]["rotation"] == 90.0
  )

  Server = create_server(Client)  # type: ignore[arg-type]
  assert [Tool.name for Tool in Server._tool_manager.list_tools()] == [
    "spawn_actor",
    "destroy_actor",
    "set_actor_transform",
    "invoke_actor_method",
  ]
