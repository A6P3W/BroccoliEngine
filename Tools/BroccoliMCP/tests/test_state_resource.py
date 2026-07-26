from __future__ import annotations

import json

import anyio
import pytest
from mcp.shared.memory import create_connected_server_and_client_session

from broccoli_mcp.errors import EngineTimeout
from broccoli_mcp.models import EngineState
from broccoli_mcp.server import create_server, read_state_resource


class FakeEngineClient:
  def __init__(Self, Result: EngineState | Exception) -> None:
    Self.Result = Result

  def get_state(Self) -> EngineState:
    if isinstance(Self.Result, Exception):
      raise Self.Result
    return Self.Result


def test_state_resource_returns_json() -> None:
  Client = FakeEngineClient(
    EngineState(
      SceneName="SceneTest",
      Fps=60.0,
      Paused=False,
      WorldAvailable=True,
      ActorCount=4,
    )
  )

  Result = read_state_resource(Client)  # type: ignore[arg-type]

  assert json.loads(Result) == {
    "sceneName": "SceneTest",
    "fps": 60.0,
    "paused": False,
    "worldAvailable": True,
    "actorCount": 4,
  }


def test_state_resource_preserves_bridge_errors() -> None:
  Client = FakeEngineClient(EngineTimeout(Operation="get engine state"))

  with pytest.raises(EngineTimeout):
    read_state_resource(Client)  # type: ignore[arg-type]


def test_server_lists_game_state_resource() -> None:
  Client = FakeEngineClient(
    EngineState(
      SceneName="",
      Fps=0.0,
      Paused=False,
      WorldAvailable=False,
      ActorCount=0,
    )
  )
  Server = create_server(Client)  # type: ignore[arg-type]

  Resources = Server._resource_manager.list_resources()

  assert [str(Resource.uri) for Resource in Resources] == [
    "game://state",
    "game://world/actors",
  ]


def test_mcp_client_can_list_and_read_game_state() -> None:
  Client = FakeEngineClient(
    EngineState(
      SceneName="SceneTest",
      Fps=60.0,
      Paused=False,
      WorldAvailable=True,
      ActorCount=4,
    )
  )
  Server = create_server(Client)  # type: ignore[arg-type]

  async def execute() -> tuple[list[str], str]:
    async with create_connected_server_and_client_session(Server) as Session:
      Listed = await Session.list_resources()
      Read = await Session.read_resource("game://state")
      return (
        [str(Resource.uri) for Resource in Listed.resources],
        Read.contents[0].text,  # type: ignore[union-attr]
      )

  ResourceUris, Content = anyio.run(execute)

  assert ResourceUris == ["game://state", "game://world/actors"]
  assert json.loads(Content)["sceneName"] == "SceneTest"
