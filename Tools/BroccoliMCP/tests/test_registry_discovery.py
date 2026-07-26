from __future__ import annotations

import httpx

from broccoli_mcp.config import BridgeConfig
from broccoli_mcp.engine_client import EngineClient
from broccoli_mcp.models import ActorClassList, ActorClassMethodList, LevelList
from broccoli_mcp.server import (
  create_server,
  get_class_methods_tool,
  get_levels_tool,
  get_registered_actor_classes_tool,
)


def json_response(Data: object) -> httpx.Response:
  return httpx.Response(
    200,
    json={"success": True, "data": Data},
    headers={"Content-Type": "application/json"},
  )


def test_registry_discovery_client_uses_expected_routes() -> None:
  def handler(Request: httpx.Request) -> httpx.Response:
    if Request.url.path == "/api/v1/actor-classes":
      return json_response({"classes": [{"className": "ADoorActor", "isGameMode": False}]})
    if Request.url.path == "/api/v1/levels":
      return json_response(
        {"levels": [{"sceneId": 2, "levelName": "Game01", "levelPath": "Levels/Game01.BLevel"}]}
      )
    assert Request.url.path == "/api/v1/actor-classes/ADoorActor/methods"
    return json_response(
      {
        "className": "ADoorActor",
        "methods": [
          {
            "name": "open_door",
            "description": "Open the door.",
            "inputSchema": {"type": "object", "properties": {}, "additionalProperties": False},
            "permission": "WorldMutation",
          }
        ],
      }
    )

  Client = EngineClient(BridgeConfig(), Transport=httpx.MockTransport(handler))
  try:
    assert Client.get_registered_actor_classes().Classes[0].ClassName == "ADoorActor"
    assert Client.get_levels().Levels[0].SceneId == 2
    assert Client.get_class_methods("ADoorActor").Methods[0].Name == "open_door"
  finally:
    Client.close()


class FakeRegistryDiscoveryClient:
  def get_registered_actor_classes(Self) -> ActorClassList:
    return ActorClassList.from_mapping(
      {"classes": [{"className": "ADoorActor", "isGameMode": False}]}, Operation="test"
    )

  def get_levels(Self) -> LevelList:
    return LevelList.from_mapping(
      {"levels": [{"sceneId": 2, "levelName": "Game01", "levelPath": "Levels/Game01.BLevel"}]},
      Operation="test",
    )

  def get_class_methods(Self, ClassName: str) -> ActorClassMethodList:
    assert ClassName == "ADoorActor"
    return ActorClassMethodList.from_mapping(
      {"className": ClassName, "methods": []}, Operation="test"
    )


def test_registry_discovery_tools_return_data_and_are_registered() -> None:
  Client = FakeRegistryDiscoveryClient()
  assert get_registered_actor_classes_tool(Client)["classes"][0]["className"] == "ADoorActor"  # type: ignore[arg-type]
  assert get_levels_tool(Client)["levels"][0]["sceneId"] == 2  # type: ignore[arg-type]
  assert get_class_methods_tool(Client, class_name="ADoorActor")["methods"] == []  # type: ignore[arg-type]
  ToolNames = {Tool.name for Tool in create_server(Client)._tool_manager.list_tools()}  # type: ignore[arg-type]
  assert {"get_registered_actor_classes", "get_levels", "get_class_methods"}.issubset(ToolNames)
