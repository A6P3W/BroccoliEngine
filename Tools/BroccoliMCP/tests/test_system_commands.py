from __future__ import annotations

import json

import httpx
import pytest

from broccoli_mcp.config import BridgeConfig
from broccoli_mcp.engine_client import EngineClient
from broccoli_mcp.errors import InvalidEngineResponse
from broccoli_mcp.models import (
  SystemCommandInfo,
  SystemCommandList,
  SystemCommandResult,
)
from broccoli_mcp.server import (
  create_server,
  execute_system_command_tool,
  open_level_by_id_tool,
  open_level_by_path_tool,
)

COMMAND_SCHEMA = {
  "type": "object",
  "properties": {},
  "additionalProperties": False,
}
PAUSE_COMMAND = {
  "name": "pause_game",
  "description": "Pause world updates while keeping automation available.",
  "inputSchema": COMMAND_SCHEMA,
  "permission": "SystemMutation",
}
RESUME_COMMAND = {
  "name": "resume_game",
  "description": "Resume world updates.",
  "inputSchema": COMMAND_SCHEMA,
  "permission": "SystemMutation",
}


def json_response(Data: object) -> httpx.Response:
  return httpx.Response(
    200,
    json={"success": True, "data": Data},
    headers={"Content-Type": "application/json"},
  )


def test_system_command_models_accept_valid_data() -> None:
  Commands = SystemCommandList.from_mapping(
    {"commands": [PAUSE_COMMAND, RESUME_COMMAND]},
    Operation="test commands",
  )
  Result = SystemCommandResult.from_mapping(
    {
      "commandName": "pause_game",
      "result": {
        "commandName": "pause_game",
        "changed": True,
        "paused": True,
      },
    },
    Operation="test command",
    ExpectedCommandName="pause_game",
  )

  assert Commands.Commands[0] == SystemCommandInfo(
    Name="pause_game",
    Description="Pause world updates while keeping automation available.",
    InputSchema=COMMAND_SCHEMA,
    Permission="SystemMutation",
  )
  assert Result.Result == {
    "commandName": "pause_game",
    "changed": True,
    "paused": True,
  }


@pytest.mark.parametrize(
  "Data",
  [
    {**PAUSE_COMMAND, "name": "BadName"},
    {**PAUSE_COMMAND, "description": ""},
    {**PAUSE_COMMAND, "inputSchema": []},
    {**PAUSE_COMMAND, "permission": "Dangerous"},
  ],
)
def test_system_command_model_rejects_invalid_descriptor(Data: dict[str, object]) -> None:
  with pytest.raises(InvalidEngineResponse):
    SystemCommandInfo.from_mapping(Data, Operation="test commands")


def test_engine_client_lists_and_executes_system_commands() -> None:
  Requests: list[httpx.Request] = []

  def handler(Request: httpx.Request) -> httpx.Response:
    Requests.append(Request)
    if Request.method == "GET":
      return json_response({"commands": [PAUSE_COMMAND, RESUME_COMMAND]})
    assert json.loads(Request.content) == {}
    return json_response(
      {
        "commandName": "pause_game",
        "result": {
          "commandName": "pause_game",
          "changed": True,
          "paused": True,
        },
      }
    )

  Client = EngineClient(BridgeConfig(), Transport=httpx.MockTransport(handler))
  try:
    Commands = Client.get_system_commands()
    Result = Client.execute_system_command("pause_game")
  finally:
    Client.close()

  assert Requests[0].url.path == "/api/v1/system/commands"
  assert Requests[1].url.path == "/api/v1/system/commands/pause_game"
  assert [Command.Name for Command in Commands.Commands] == [
    "pause_game",
    "resume_game",
  ]
  assert Result.Result["paused"] is True


@pytest.mark.parametrize("CommandName", ["", "BadName", "a-b", "a" * 129])
def test_engine_client_rejects_invalid_command_name(CommandName: str) -> None:
  Client = EngineClient(
    BridgeConfig(),
    Transport=httpx.MockTransport(lambda Request: json_response({})),
  )
  try:
    with pytest.raises(ValueError, match="CommandName"):
      Client.execute_system_command(CommandName)
  finally:
    Client.close()


class FakeSystemCommandClient:
  def execute_system_command(
    Self,
    CommandName: str,
    Arguments: dict[str, object] | None = None,
  ) -> SystemCommandResult:
    assert (CommandName, Arguments) == ("pause_game", {})
    return SystemCommandResult(
      CommandName="pause_game",
      Result={
        "commandName": "pause_game",
        "changed": True,
        "paused": True,
      },
    )


class FakeOpenLevelClient:
  def execute_system_command(
    Self,
    CommandName: str,
    Arguments: dict[str, object] | None = None,
  ) -> SystemCommandResult:
    if CommandName == "open_level_by_id":
      assert Arguments == {"sceneId": 3}
      Result = {"commandName": CommandName, "sceneId": 3, "queued": True}
    else:
      assert CommandName == "open_level_by_path"
      assert Arguments == {"levelPath": "Levels/Test.level"}
      Result = {
        "commandName": CommandName,
        "levelPath": "Levels/Test.level",
        "queued": True,
      }
    return SystemCommandResult(CommandName=CommandName, Result=Result)


def test_system_command_tool_returns_data_and_is_registered() -> None:
  Client = FakeSystemCommandClient()

  Result = execute_system_command_tool(
    Client,  # type: ignore[arg-type]
    command_name="pause_game",
    arguments={},
  )
  Server = create_server(Client)  # type: ignore[arg-type]

  assert Result["result"]["paused"] is True
  assert "execute_system_command" in [Tool.name for Tool in Server._tool_manager.list_tools()]


def test_open_level_tools_return_queued_result_and_are_registered() -> None:
  Client = FakeOpenLevelClient()

  ByIdResult = open_level_by_id_tool(Client, scene_id=3)  # type: ignore[arg-type]
  ByPathResult = open_level_by_path_tool(  # type: ignore[arg-type]
    Client,
    level_path="Levels/Test.level",
  )
  Server = create_server(Client)  # type: ignore[arg-type]

  assert ByIdResult["queued"] is True
  assert ByPathResult["queued"] is True
  ToolNames = [Tool.name for Tool in Server._tool_manager.list_tools()]
  assert "open_level_by_id" in ToolNames
  assert "open_level_by_path" in ToolNames
