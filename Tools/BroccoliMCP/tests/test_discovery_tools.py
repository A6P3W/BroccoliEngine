from __future__ import annotations

from broccoli_mcp.models import (
  ActorInfo,
  ActorList,
  ActorTransform,
  SystemCommandInfo,
  SystemCommandList,
)
from broccoli_mcp.server import (
  create_server,
  find_actors_tool,
  get_actor_tool,
  get_system_commands_tool,
)


class FakeDiscoveryClient:
  def get_system_commands(Self) -> SystemCommandList:
    return SystemCommandList(
      Commands=(
        SystemCommandInfo(
          Name="pause_game",
          Description="Pause world updates while keeping automation available.",
          InputSchema={
            "type": "object",
            "properties": {},
            "additionalProperties": False,
          },
          Permission="SystemMutation",
        ),
      )
    )

  def get_actor(Self, ActorId: int) -> ActorInfo:
    assert ActorId == 7
    return ActorInfo(
      ActorId=7,
      InstanceName="Door_1",
      ClassName="ADoorActor",
      Transform=ActorTransform(
        LocationX=100.0,
        LocationY=200.0,
        Rotation=0.0,
        Scale=1.0,
      ),
    )

  def find_actors(Self, **Arguments: object) -> ActorList:
    assert Arguments == {"ClassName": "ADoorActor", "InstanceName": None}
    return ActorList(
      SceneName="Game01",
      ActorCount=0,
      Actors=(),
    )


def test_discovery_tools_return_data_and_are_registered() -> None:
  Client = FakeDiscoveryClient()

  Commands = get_system_commands_tool(Client)  # type: ignore[arg-type]
  Actor = get_actor_tool(Client, actor_id=7)  # type: ignore[arg-type]
  FoundActors = find_actors_tool(Client, class_name="ADoorActor")  # type: ignore[arg-type]
  Server = create_server(Client)  # type: ignore[arg-type]

  assert Commands["commands"][0]["name"] == "pause_game"
  assert Actor == {
    "actorId": 7,
    "instanceName": "Door_1",
    "className": "ADoorActor",
    "transform": {
      "location": {"x": 100.0, "y": 200.0},
      "rotation": 0.0,
      "scale": 1.0,
    },
  }
  assert FoundActors["actorCount"] == 0
  ToolNames = [Tool.name for Tool in Server._tool_manager.list_tools()]
  assert "get_system_commands" in ToolNames
  assert "get_actor" in ToolNames
  assert "find_actors" in ToolNames
