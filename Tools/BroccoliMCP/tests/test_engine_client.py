from __future__ import annotations

import json
from collections.abc import Callable

import httpx
import pytest

from broccoli_mcp.config import BridgeConfig
from broccoli_mcp.engine_client import EngineClient
from broccoli_mcp.errors import (
  EngineApiError,
  EngineTimeout,
  EngineUnavailable,
  InvalidEngineResponse,
)

STATE_DATA = {
  "sceneName": "LevelStarter",
  "fps": 59.5,
  "paused": False,
  "worldAvailable": True,
  "actorCount": 12,
}
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


def run_request(Handler: Callable[[httpx.Request], httpx.Response]):
  Client = EngineClient(BridgeConfig(), Transport=httpx.MockTransport(Handler))
  try:
    return Client.get_state()
  finally:
    Client.close()


def json_response(Status: int, Body: object) -> httpx.Response:
  return httpx.Response(
    Status,
    json=Body,
    headers={"Content-Type": "application/json; charset=utf-8"},
  )


def test_state_response_is_converted_and_unknown_fields_are_ignored() -> None:
  def handler(Request: httpx.Request) -> httpx.Response:
    assert Request.method == "GET"
    assert Request.url == "http://127.0.0.1:39100/api/v1/state"
    return json_response(200, {"success": True, "data": {**STATE_DATA, "future": 1}})

  State = run_request(handler)

  assert State.SceneName == "LevelStarter"
  assert State.Fps == 59.5
  assert State.ActorCount == 12
  assert "future" not in State.to_dict()


def test_engine_failure_preserves_code_message_and_status() -> None:
  def handler(Request: httpx.Request) -> httpx.Response:
    del Request
    return json_response(
      503,
      {
        "success": False,
        "error": {
          "code": "WORLD_NOT_AVAILABLE",
          "message": "No world is active.",
        },
      },
    )

  with pytest.raises(EngineApiError) as ErrorInfo:
    run_request(handler)

  assert ErrorInfo.value.Code == "WORLD_NOT_AVAILABLE"
  assert ErrorInfo.value.Message == "No world is active."
  assert ErrorInfo.value.HttpStatus == 503


def test_connection_refusal_becomes_engine_unavailable() -> None:
  def handler(Request: httpx.Request) -> httpx.Response:
    raise httpx.ConnectError("refused", request=Request)

  with pytest.raises(EngineUnavailable):
    run_request(handler)


def test_timeout_becomes_engine_timeout() -> None:
  def handler(Request: httpx.Request) -> httpx.Response:
    raise httpx.ReadTimeout("late", request=Request)

  with pytest.raises(EngineTimeout):
    run_request(handler)


@pytest.mark.parametrize(
  ("Response", "ExpectedText"),
  [
    (httpx.Response(200, text="not json"), "Content-Type"),
    (
      httpx.Response(
        200,
        text="{",
        headers={"Content-Type": "application/json"},
      ),
      "valid JSON",
    ),
    (json_response(200, []), "root"),
    (json_response(200, {"success": "yes", "data": {}}), "success"),
    (json_response(200, {"success": True}), "data"),
  ],
)
def test_invalid_envelope_is_rejected(
  Response: httpx.Response,
  ExpectedText: str,
) -> None:
  def handler(Request: httpx.Request) -> httpx.Response:
    del Request
    return Response

  with pytest.raises(InvalidEngineResponse) as ErrorInfo:
    run_request(handler)

  assert ExpectedText in ErrorInfo.value.Message


def test_missing_state_field_is_rejected() -> None:
  def handler(Request: httpx.Request) -> httpx.Response:
    del Request
    Data = dict(STATE_DATA)
    del Data["paused"]
    return json_response(200, {"success": True, "data": Data})

  with pytest.raises(InvalidEngineResponse, match="paused"):
    run_request(handler)


def test_oversized_response_is_rejected() -> None:
  Config = BridgeConfig(MaxResponseBytes=8)
  Client = EngineClient(
    Config,
    Transport=httpx.MockTransport(
      lambda Request: json_response(
        200,
        {"success": True, "data": STATE_DATA},
      )
    ),
  )
  try:
    with pytest.raises(InvalidEngineResponse, match="size limit"):
      Client.get_state()
  finally:
    Client.close()


def test_get_actors_uses_world_actors_url() -> None:
  def handler(Request: httpx.Request) -> httpx.Response:
    assert Request.url == "http://127.0.0.1:39100/api/v1/world/actors"
    return json_response(
      200,
      {
        "success": True,
        "data": {
          "sceneName": "BasicGameplay",
          "actorCount": 1,
          "actors": [ACTOR_DATA],
        },
      },
    )

  Client = EngineClient(BridgeConfig(), Transport=httpx.MockTransport(handler))
  try:
    Actors = Client.get_actors()
  finally:
    Client.close()

  assert Actors.ActorCount == 1
  assert Actors.Actors[0].ActorId == 42


def test_get_actor_uses_actor_id_url() -> None:
  def handler(Request: httpx.Request) -> httpx.Response:
    assert Request.url == "http://127.0.0.1:39100/api/v1/world/actors/42"
    return json_response(200, {"success": True, "data": ACTOR_DATA})

  Client = EngineClient(BridgeConfig(), Transport=httpx.MockTransport(handler))
  try:
    Actor = Client.get_actor(42)
  finally:
    Client.close()

  assert Actor.ActorId == 42


def test_get_actor_preserves_not_found_error() -> None:
  def handler(Request: httpx.Request) -> httpx.Response:
    del Request
    return json_response(
      404,
      {
        "success": False,
        "error": {
          "code": "ACTOR_NOT_FOUND",
          "message": "The requested actor was not found.",
        },
      },
    )

  Client = EngineClient(BridgeConfig(), Transport=httpx.MockTransport(handler))
  try:
    with pytest.raises(EngineApiError) as ErrorInfo:
      Client.get_actor(42)
  finally:
    Client.close()

  assert ErrorInfo.value.Code == "ACTOR_NOT_FOUND"
  assert ErrorInfo.value.HttpStatus == 404


def test_get_component_methods_uses_component_route() -> None:
  def handler(Request: httpx.Request) -> httpx.Response:
    assert Request.method == "GET"
    assert Request.url == "http://127.0.0.1:39100/api/v1/world/actors/42/components/7/methods"
    return json_response(
      200,
      {
        "success": True,
        "data": {
          "actorId": 42,
          "componentId": 7,
          "methods": [],
        },
      },
    )

  Client = EngineClient(BridgeConfig(), Transport=httpx.MockTransport(handler))
  try:
    Result = Client.get_component_methods(42, 7)
  finally:
    Client.close()

  assert Result["actorId"] == 42
  assert Result["componentId"] == 7


def test_invoke_component_method_uses_component_route_and_envelope() -> None:
  def handler(Request: httpx.Request) -> httpx.Response:
    assert Request.method == "POST"
    assert Request.url == (
      "http://127.0.0.1:39100/api/v1/world/actors/42/components/7/methods/set_active"
    )
    assert json.loads(Request.content) == {"arguments": {"active": True}}
    return json_response(
      200,
      {
        "success": True,
        "data": {
          "actorId": 42,
          "componentId": 7,
          "methodName": "set_active",
          "result": {"success": True},
        },
      },
    )

  Client = EngineClient(BridgeConfig(), Transport=httpx.MockTransport(handler))
  try:
    Result = Client.invoke_component_method(42, 7, "set_active", {"active": True})
  finally:
    Client.close()

  assert Result["methodName"] == "set_active"
  assert Result["result"] == {"success": True}
