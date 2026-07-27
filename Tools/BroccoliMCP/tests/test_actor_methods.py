from __future__ import annotations

import json

import httpx
import pytest

from broccoli_mcp.config import BridgeConfig
from broccoli_mcp.engine_client import EngineClient
from broccoli_mcp.errors import InvalidEngineResponse
from broccoli_mcp.models import (
  ActorMethodInfo,
  ActorMethodList,
  ActorMethodResult,
)
from broccoli_mcp.server import create_server, invoke_actor_method_tool

METHOD_DATA = {
  "name": "get_status",
  "description": "Return the LevelStarter widget status.",
  "inputSchema": {
    "type": "object",
    "properties": {},
    "additionalProperties": False,
  },
  "permission": "ReadOnly",
}


def json_response(Data: object) -> httpx.Response:
  return httpx.Response(
    200,
    json={"success": True, "data": Data},
    headers={"Content-Type": "application/json"},
  )


def test_actor_method_models_accept_valid_data() -> None:
  Methods = ActorMethodList.from_mapping(
    {
      "actorId": 7,
      "className": "ALevelStarterWidget",
      "methods": [METHOD_DATA],
    },
    Operation="test methods",
  )
  Result = ActorMethodResult.from_mapping(
    {
      "actorId": 7,
      "className": "ALevelStarterWidget",
      "methodName": "get_status",
      "result": {"ready": True},
    },
    Operation="test method",
    ExpectedActorId=7,
    ExpectedMethodName="get_status",
  )

  assert Methods.Methods == (
    ActorMethodInfo(
      Name="get_status",
      Description="Return the LevelStarter widget status.",
      InputSchema=METHOD_DATA["inputSchema"],
      Permission="ReadOnly",
    ),
  )
  assert Result.to_dict()["result"] == {"ready": True}


@pytest.mark.parametrize(
  "Data",
  [
    {**METHOD_DATA, "name": "BadName"},
    {**METHOD_DATA, "description": ""},
    {**METHOD_DATA, "inputSchema": []},
    {**METHOD_DATA, "permission": "Dangerous"},
  ],
)
def test_actor_method_model_rejects_invalid_descriptor(Data: dict[str, object]) -> None:
  with pytest.raises(InvalidEngineResponse):
    ActorMethodInfo.from_mapping(Data, Operation="test methods")


def test_engine_client_lists_and_invokes_actor_methods() -> None:
  Requests: list[httpx.Request] = []

  def handler(Request: httpx.Request) -> httpx.Response:
    Requests.append(Request)
    if Request.method == "GET":
      return json_response(
        {
          "actorId": 7,
          "className": "ALevelStarterWidget",
          "methods": [METHOD_DATA],
        }
      )
    assert json.loads(Request.content) == {}
    return json_response(
      {
        "actorId": 7,
        "className": "ALevelStarterWidget",
        "methodName": "get_status",
        "result": {"ready": True},
      }
    )

  Client = EngineClient(BridgeConfig(), Transport=httpx.MockTransport(handler))
  try:
    Methods = Client.get_actor_methods(7)
    Result = Client.invoke_actor_method(7, "get_status")
  finally:
    Client.close()

  assert Requests[0].url.path == "/api/v1/world/actors/7/methods"
  assert Requests[1].url.path == "/api/v1/world/actors/7/methods/get_status"
  assert Methods.Methods[0].Name == "get_status"
  assert Result.Result == {"ready": True}


@pytest.mark.parametrize("MethodName", ["", "BadName", "a-b", "a" * 129])
def test_engine_client_rejects_invalid_method_name(MethodName: str) -> None:
  Client = EngineClient(
    BridgeConfig(), Transport=httpx.MockTransport(lambda Request: json_response({}))
  )
  try:
    with pytest.raises(ValueError, match="MethodName"):
      Client.invoke_actor_method(7, MethodName)
  finally:
    Client.close()


class FakeActorMethodClient:
  def invoke_actor_method(
    Self,
    ActorId: int,
    MethodName: str,
    Arguments: dict[str, object] | None = None,
  ) -> ActorMethodResult:
    assert (ActorId, MethodName, Arguments) == (7, "get_status", {})
    return ActorMethodResult(
      ActorId=7,
      ClassName="ALevelStarterWidget",
      MethodName="get_status",
      Result={"ready": True},
    )


def test_actor_method_tool_returns_data_and_is_registered() -> None:
  Client = FakeActorMethodClient()

  Result = invoke_actor_method_tool(
    Client,  # type: ignore[arg-type]
    actor_id=7,
    method_name="get_status",
    arguments={},
  )
  Server = create_server(Client)  # type: ignore[arg-type]

  assert Result["result"] == {"ready": True}
  assert "invoke_actor_method" in [Tool.name for Tool in Server._tool_manager.list_tools()]
