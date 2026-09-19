from __future__ import annotations

import json

import anyio
from broccoli_control.models import ActorInfo, ActorTransform, EngineState
from mcp.client import Client

from broccoli_mcp.server import create_server


class FakeServerClient:
  def get_state(Self) -> EngineState:
    return EngineState(
      SceneName="SceneTest",
      Fps=60.0,
      Paused=False,
      WorldAvailable=True,
      ActorCount=0,
    )

  def spawn_actor(Self, ClassName: str, **Arguments: object) -> ActorInfo:
    assert ClassName == "ATestActor"
    assert Arguments["LocationX"] == 1.0
    return ActorInfo(
      ActorId=7,
      InstanceName="ATestActor_1",
      ClassName=ClassName,
      Transform=ActorTransform(LocationX=1.0, LocationY=0.0, Rotation=0.0, Scale=1.0),
    )


def test_server_registers_resources_and_tools() -> None:
  Server = create_server(FakeServerClient())  # type: ignore[arg-type]

  assert [str(Resource.uri) for Resource in Server._resource_manager.list_resources()] == [
    "game://state",
    "game://world/actors",
    "game://logs/recent",
  ]
  ToolNames = {Tool.name for Tool in Server._tool_manager.list_tools()}
  assert {"spawn_actor", "get_actor", "get_system_commands", "open_level_by_id"}.issubset(ToolNames)


def test_mcp_server_reads_a_resource_and_calls_a_tool() -> None:
  Server = create_server(FakeServerClient())  # type: ignore[arg-type]

  async def execute() -> tuple[str, dict[str, object]]:
    async with Client(Server) as Session:
      ResourceResult = await Session.read_resource("game://state")
      ToolResult = await Session.call_tool(
        "spawn_actor",
        {"class_name": "ATestActor", "location_x": 1.0},
      )
      assert not ToolResult.is_error
      assert isinstance(ToolResult.structured_content, dict)
      return ResourceResult.contents[0].text, ToolResult.structured_content  # type: ignore[union-attr]

  ResourceContent, SpawnedActor = anyio.run(execute)

  assert json.loads(ResourceContent)["sceneName"] == "SceneTest"
  assert SpawnedActor["actorId"] == 7
