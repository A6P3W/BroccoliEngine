"""Live MCP stdio integration check against a running BROCCOLI ENGINE."""

from __future__ import annotations

import asyncio
import json
import sys
from math import isclose
from pathlib import Path

from broccoli_control.client import ControlClient
from broccoli_control.config import ControlConfig
from mcp.client import Client
from mcp.client.stdio import StdioServerParameters
from mcp.types import TextResourceContents


async def run_integration() -> dict[str, object]:
  ProjectRoot = Path(__file__).resolve().parents[1]
  Parameters = StdioServerParameters(
    command=sys.executable,
    args=["-m", "broccoli_mcp"],
    cwd=ProjectRoot,
  )

  async with Client(Parameters, read_timeout_seconds=10) as Session:
    Resources = await Session.list_resources()
    ResourceUris = [str(Resource.uri) for Resource in Resources.resources]
    Tools = await Session.list_tools()
    ToolNames = [Tool.name for Tool in Tools.tools]
    if "game://state" not in ResourceUris:
      raise RuntimeError("game://state is not exposed by the bridge.")
    if "game://world/actors" not in ResourceUris:
      raise RuntimeError("game://world/actors is not exposed by the bridge.")
    if "game://logs/recent" not in ResourceUris:
      raise RuntimeError("game://logs/recent is not exposed by the bridge.")
    for ToolName in (
      "spawn_actor",
      "destroy_actor",
      "set_actor_transform",
      "invoke_actor_method",
      "execute_system_command",
      "get_system_commands",
      "get_registered_actor_classes",
      "get_levels",
      "get_actor",
      "find_actors",
      "get_actor_components",
      "get_class_methods",
    ):
      if ToolName not in ToolNames:
        raise RuntimeError(f"{ToolName} is not exposed by the bridge.")

    Result = await Session.read_resource("game://state")
    if len(Result.contents) != 1 or not isinstance(
      Result.contents[0],
      TextResourceContents,
    ):
      raise RuntimeError("game://state did not return one text resource.")
    State = json.loads(Result.contents[0].text)

    ActorResult = await Session.read_resource("game://world/actors")
    if len(ActorResult.contents) != 1 or not isinstance(
      ActorResult.contents[0],
      TextResourceContents,
    ):
      raise RuntimeError("game://world/actors did not return one text resource.")
    Actors = json.loads(ActorResult.contents[0].text)

    LogResult = await Session.read_resource("game://logs/recent")
    if len(LogResult.contents) != 1 or not isinstance(
      LogResult.contents[0],
      TextResourceContents,
    ):
      raise RuntimeError("game://logs/recent did not return one text resource.")
    Logs = json.loads(LogResult.contents[0].text)

    DiscoveryResults: dict[str, object] = {}
    for ToolName, Arguments in (
      ("get_system_commands", {}),
      ("get_registered_actor_classes", {}),
      ("get_levels", {}),
      ("find_actors", {}),
    ):
      ToolResult = await Session.call_tool(ToolName, Arguments)
      if ToolResult.is_error or not isinstance(ToolResult.structured_content, dict):
        raise RuntimeError(f"{ToolName} did not return structured discovery data.")
      DiscoveryResults[ToolName] = ToolResult.structured_content

    RegisteredClasses = DiscoveryResults["get_registered_actor_classes"]
    if not isinstance(RegisteredClasses, dict) or not RegisteredClasses.get("classes"):
      raise RuntimeError("get_registered_actor_classes returned no registered classes.")
    DiscoveryClassName = RegisteredClasses["classes"][0]["className"]
    ClassMethodsResult = await Session.call_tool(
      "get_class_methods", {"class_name": DiscoveryClassName}
    )
    if ClassMethodsResult.is_error or not isinstance(ClassMethodsResult.structured_content, dict):
      raise RuntimeError("get_class_methods did not return structured discovery data.")
    DiscoveryResults["get_class_methods"] = ClassMethodsResult.structured_content

    if Actors["actors"]:
      DiscoveryActorId = Actors["actors"][0]["actorId"]
      for ToolName in ("get_actor", "get_actor_components"):
        ToolResult = await Session.call_tool(ToolName, {"actor_id": DiscoveryActorId})
        if ToolResult.is_error or not isinstance(ToolResult.structured_content, dict):
          raise RuntimeError(f"{ToolName} did not return structured discovery data.")
        DiscoveryResults[ToolName] = ToolResult.structured_content

    LevelStarterActors = [
      Actor for Actor in Actors["actors"] if Actor["className"] == "ALevelStarterWidget"
    ]
    ActorId = LevelStarterActors[0]["actorId"] if LevelStarterActors else None
    with ControlClient(ControlConfig()) as Engine:
      ActorMethods = Engine.get_actor_methods(ActorId).to_dict() if ActorId else None
      SystemCommandList = Engine.get_system_commands().to_dict()
      SpawnedActorId = None
      MutationResults: dict[str, object] = {}
      try:
        SpawnedActor = Engine.spawn_actor(
          "ADoorActor",
          LocationX=12.0,
          LocationY=34.0,
          Rotation=15.0,
          Scale=1.25,
        )
        SpawnedActorId = SpawnedActor.ActorId
        if SpawnedActor.ClassName != "ADoorActor":
          raise RuntimeError("spawn_actor returned an unexpected actor class.")

        PatchedActor = Engine.set_actor_transform(
          SpawnedActorId,
          LocationX=56.0,
          LocationY=78.0,
          Rotation=30.0,
          Scale=1.5,
        )
        PatchedTransform = PatchedActor.Transform
        if (
          PatchedTransform.LocationX != 56.0
          or PatchedTransform.LocationY != 78.0
          or not isclose(PatchedTransform.Rotation, 30.0, rel_tol=0.0, abs_tol=0.0001)
          or PatchedTransform.Scale != 1.5
        ):
          raise RuntimeError("set_actor_transform returned an unexpected transform.")

        DoorMethods = Engine.get_actor_methods(SpawnedActorId).to_dict()
        DoorMethodNames = {Method["name"] for Method in DoorMethods["methods"]}
        if not {"open_door", "close_door", "get_door_state"}.issubset(DoorMethodNames):
          raise RuntimeError("ADoorActor method discovery is incomplete.")
        OpenDoorResult = Engine.invoke_actor_method(SpawnedActorId, "open_door").to_dict()
        DoorStateResult = Engine.invoke_actor_method(SpawnedActorId, "get_door_state").to_dict()
        if DoorStateResult["result"].get("is_open") is not True:
          raise RuntimeError("ADoorActor method invocation did not update state.")

        Components = Engine.get_actor_components(SpawnedActorId)
        DoorComponent = next(
          (
            Component
            for Component in Components.Components
            if Component.ClassName == "MDoorAutomationTestComponent"
          ),
          None,
        )
        if DoorComponent is None:
          raise RuntimeError("ADoorActor automation component was not discovered.")
        ComponentMethods = Engine.get_component_methods(SpawnedActorId, DoorComponent.ComponentId)
        ComponentMethodNames = {Method["name"] for Method in ComponentMethods.get("methods", [])}
        if not {"set_active", "is_active"}.issubset(ComponentMethodNames):
          raise RuntimeError("Automation component method discovery is incomplete.")
        SetActiveResult = Engine.invoke_component_method(
          SpawnedActorId,
          DoorComponent.ComponentId,
          "set_active",
          {"active": True},
        )
        ActiveStateResult = Engine.invoke_component_method(
          SpawnedActorId,
          DoorComponent.ComponentId,
          "is_active",
        )
        if ActiveStateResult.get("result", {}).get("active") is not True:
          raise RuntimeError("Automation component invocation did not update state.")

        MutationResults = {
          "spawnedActor": SpawnedActor.to_dict(),
          "patchedActor": PatchedActor.to_dict(),
          "openDoor": OpenDoorResult,
          "doorState": DoorStateResult,
          "components": Components.to_dict(),
          "componentMethods": ComponentMethods,
          "setActive": SetActiveResult,
          "activeState": ActiveStateResult,
        }
      finally:
        if SpawnedActorId is not None:
          DestroyedActor = Engine.destroy_actor(SpawnedActorId)
          MutationResults["destroyedActor"] = DestroyedActor.to_dict()
    if ActorMethods and [Method["name"] for Method in ActorMethods["methods"]] != ["get_status"]:
      raise RuntimeError("LevelStarter actor method list is invalid.")
    CommandNames = {Command["name"] for Command in SystemCommandList["commands"]}
    if not {"start_simulation", "stop_simulation", "open_level_by_id", "open_level_by_path"}.issubset(
      CommandNames
    ) or {"pause_game", "resume_game"}.intersection(CommandNames):
      raise RuntimeError("System command list is invalid.")

    if ActorId:
      MethodResult = await Session.call_tool(
        "invoke_actor_method",
        {
          "actor_id": ActorId,
          "method_name": "get_status",
          "arguments": {},
        },
      )
      if MethodResult.is_error:
        raise RuntimeError("invoke_actor_method returned an MCP error.")
      MethodData = MethodResult.structured_content
      if (
        not isinstance(MethodData, dict)
        or MethodData.get("actorId") != ActorId
        or MethodData.get("methodName") != "get_status"
        or MethodData.get("result", {}).get("ready") is not True
      ):
        raise RuntimeError("invoke_actor_method returned invalid data.")
    else:
      MethodData = None

    StopResult = await Session.call_tool(
      "execute_system_command",
      {"command_name": "stop_simulation", "arguments": {}},
    )
    StopData = StopResult.structured_content
    if (
      StopResult.is_error
      or not isinstance(StopData, dict)
      or StopData.get("result", {}).get("worldAvailable") is not True
      or StopData.get("result", {}).get("changed") is not True
      or StopData.get("result", {}).get("simulating") is not False
    ):
      raise RuntimeError("stop_simulation did not stop the simulation.")

    RepeatedStopResult = await Session.call_tool(
      "execute_system_command",
      {"command_name": "stop_simulation", "arguments": {}},
    )
    RepeatedStopData = RepeatedStopResult.structured_content
    if (
      RepeatedStopResult.is_error
      or not isinstance(RepeatedStopData, dict)
      or RepeatedStopData.get("result", {}).get("changed") is not False
      or RepeatedStopData.get("result", {}).get("simulating") is not False
    ):
      raise RuntimeError("Repeated stop_simulation was not idempotent.")

    StoppedStateResult = await Session.read_resource("game://state")
    StoppedState = json.loads(StoppedStateResult.contents[0].text)  # type: ignore[union-attr]
    if StoppedState.get("simulating") is not False:
      raise RuntimeError("State resource did not report simulating=false.")

    StartResult = await Session.call_tool(
      "execute_system_command",
      {"command_name": "start_simulation", "arguments": {}},
    )
    StartData = StartResult.structured_content
    if (
      StartResult.is_error
      or not isinstance(StartData, dict)
      or StartData.get("result", {}).get("worldAvailable") is not True
      or StartData.get("result", {}).get("changed") is not True
      or StartData.get("result", {}).get("simulating") is not True
    ):
      raise RuntimeError("start_simulation did not start the simulation.")

    RepeatedStartResult = await Session.call_tool(
      "execute_system_command",
      {"command_name": "start_simulation", "arguments": {}},
    )
    RepeatedStartData = RepeatedStartResult.structured_content
    if (
      RepeatedStartResult.is_error
      or not isinstance(RepeatedStartData, dict)
      or RepeatedStartData.get("result", {}).get("changed") is not False
      or RepeatedStartData.get("result", {}).get("simulating") is not True
    ):
      raise RuntimeError("Repeated start_simulation was not idempotent.")

    StartedStateResult = await Session.read_resource("game://state")
    StartedState = json.loads(StartedStateResult.contents[0].text)  # type: ignore[union-attr]
    if StartedState.get("simulating") is not True:
      raise RuntimeError("State resource did not report simulating=true.")

  RequiredFields = {
    "sceneName",
    "fps",
    "simulating",
    "worldAvailable",
    "actorCount",
  }
  MissingFields = RequiredFields.difference(State)
  if MissingFields:
    raise RuntimeError(f"State response is missing fields: {sorted(MissingFields)}")
  RequiredActorListFields = {"sceneName", "actorCount", "actors"}
  MissingActorListFields = RequiredActorListFields.difference(Actors)
  if MissingActorListFields:
    raise RuntimeError(f"Actor response is missing fields: {sorted(MissingActorListFields)}")
  if Actors["actorCount"] != len(Actors["actors"]):
    raise RuntimeError("Actor count does not match the actors array.")
  RequiredLogFields = {
    "entries",
    "count",
    "oldestAvailableSequence",
    "latestSequence",
    "nextAfterSequence",
    "historyLost",
    "hasMore",
  }
  MissingLogFields = RequiredLogFields.difference(Logs)
  if MissingLogFields:
    raise RuntimeError(f"Log response is missing fields: {sorted(MissingLogFields)}")
  if Logs["count"] != len(Logs["entries"]):
    raise RuntimeError("Log count does not match the entries array.")
  return {
    "state": State,
    "worldActors": Actors,
    "recentLogs": Logs,
    "actorMethod": {"actorId": ActorId, "methodName": "get_status"},
    "actorMethods": ActorMethods,
    "systemCommandList": SystemCommandList,
    "mutations": MutationResults,
    "systemCommands": {
      "stop": StopData,
      "repeatedStop": RepeatedStopData,
      "stoppedState": StoppedState,
      "start": StartData,
      "repeatedStart": RepeatedStartData,
      "startedState": StartedState,
    },
    "discovery": DiscoveryResults,
  }


def main() -> int:
  State = asyncio.run(run_integration())
  print(json.dumps(State, ensure_ascii=False, indent=2))
  return 0


if __name__ == "__main__":
  sys.exit(main())
