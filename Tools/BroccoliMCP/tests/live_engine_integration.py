"""Live MCP stdio integration check against a running BROCCOLI ENGINE."""

from __future__ import annotations

import asyncio
import json
import sys
from datetime import timedelta
from pathlib import Path

from mcp import ClientSession, StdioServerParameters
from mcp.client.stdio import stdio_client
from mcp.types import TextResourceContents

from broccoli_mcp.config import BridgeConfig
from broccoli_mcp.engine_client import EngineClient


async def run_integration() -> dict[str, object]:
  ProjectRoot = Path(__file__).resolve().parents[1]
  Parameters = StdioServerParameters(
    command=sys.executable,
    args=["-m", "broccoli_mcp"],
    cwd=ProjectRoot,
  )

  async with stdio_client(Parameters) as (ReadStream, WriteStream):
    async with ClientSession(
      ReadStream,
      WriteStream,
      read_timeout_seconds=timedelta(seconds=10),
    ) as Session:
      await Session.initialize()
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
        if ToolResult.isError or not isinstance(ToolResult.structuredContent, dict):
          raise RuntimeError(f"{ToolName} did not return structured discovery data.")
        DiscoveryResults[ToolName] = ToolResult.structuredContent

      RegisteredClasses = DiscoveryResults["get_registered_actor_classes"]
      if not isinstance(RegisteredClasses, dict) or not RegisteredClasses.get("classes"):
        raise RuntimeError("get_registered_actor_classes returned no registered classes.")
      DiscoveryClassName = RegisteredClasses["classes"][0]["className"]
      ClassMethodsResult = await Session.call_tool(
        "get_class_methods", {"class_name": DiscoveryClassName}
      )
      if ClassMethodsResult.isError or not isinstance(ClassMethodsResult.structuredContent, dict):
        raise RuntimeError("get_class_methods did not return structured discovery data.")
      DiscoveryResults["get_class_methods"] = ClassMethodsResult.structuredContent

      if Actors["actors"]:
        DiscoveryActorId = Actors["actors"][0]["actorId"]
        for ToolName in ("get_actor", "get_actor_components"):
          ToolResult = await Session.call_tool(ToolName, {"actor_id": DiscoveryActorId})
          if ToolResult.isError or not isinstance(ToolResult.structuredContent, dict):
            raise RuntimeError(f"{ToolName} did not return structured discovery data.")
          DiscoveryResults[ToolName] = ToolResult.structuredContent

      LevelStarterActors = [
        Actor for Actor in Actors["actors"] if Actor["className"] == "ALevelStarterWidget"
      ]
      ActorId = LevelStarterActors[0]["actorId"] if LevelStarterActors else None
      with EngineClient(BridgeConfig()) as Engine:
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
          if PatchedActor.Transform.to_dict() != {
            "location": {"x": 56.0, "y": 78.0},
            "rotation": 30.0,
            "scale": 1.5,
          }:
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
      if not {"pause_game", "resume_game", "open_level_by_id", "open_level_by_path"}.issubset(
        {Command["name"] for Command in SystemCommandList["commands"]}
      ):
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
        if MethodResult.isError:
          raise RuntimeError("invoke_actor_method returned an MCP error.")
        MethodData = MethodResult.structuredContent
        if (
          not isinstance(MethodData, dict)
          or MethodData.get("actorId") != ActorId
          or MethodData.get("methodName") != "get_status"
          or MethodData.get("result", {}).get("ready") is not True
        ):
          raise RuntimeError("invoke_actor_method returned invalid data.")
      else:
        MethodData = None

      PauseResult = await Session.call_tool(
        "execute_system_command",
        {"command_name": "pause_game", "arguments": {}},
      )
      PauseData = PauseResult.structuredContent
      if (
        PauseResult.isError
        or not isinstance(PauseData, dict)
        or PauseData.get("result", {}).get("changed") is not True
        or PauseData.get("result", {}).get("paused") is not True
      ):
        raise RuntimeError("pause_game did not pause the engine.")

      RepeatedPauseResult = await Session.call_tool(
        "execute_system_command",
        {"command_name": "pause_game", "arguments": {}},
      )
      RepeatedPauseData = RepeatedPauseResult.structuredContent
      if (
        RepeatedPauseResult.isError
        or not isinstance(RepeatedPauseData, dict)
        or RepeatedPauseData.get("result", {}).get("changed") is not False
        or RepeatedPauseData.get("result", {}).get("paused") is not True
      ):
        raise RuntimeError("Repeated pause_game was not idempotent.")

      PausedStateResult = await Session.read_resource("game://state")
      PausedState = json.loads(PausedStateResult.contents[0].text)  # type: ignore[union-attr]
      if PausedState.get("paused") is not True:
        raise RuntimeError("State resource did not report paused=true.")

      ResumeResult = await Session.call_tool(
        "execute_system_command",
        {"command_name": "resume_game", "arguments": {}},
      )
      ResumeData = ResumeResult.structuredContent
      if (
        ResumeResult.isError
        or not isinstance(ResumeData, dict)
        or ResumeData.get("result", {}).get("changed") is not True
        or ResumeData.get("result", {}).get("paused") is not False
      ):
        raise RuntimeError("resume_game did not resume the engine.")

      RepeatedResumeResult = await Session.call_tool(
        "execute_system_command",
        {"command_name": "resume_game", "arguments": {}},
      )
      RepeatedResumeData = RepeatedResumeResult.structuredContent
      if (
        RepeatedResumeResult.isError
        or not isinstance(RepeatedResumeData, dict)
        or RepeatedResumeData.get("result", {}).get("changed") is not False
        or RepeatedResumeData.get("result", {}).get("paused") is not False
      ):
        raise RuntimeError("Repeated resume_game was not idempotent.")

      ResumedStateResult = await Session.read_resource("game://state")
      ResumedState = json.loads(ResumedStateResult.contents[0].text)  # type: ignore[union-attr]
      if ResumedState.get("paused") is not False:
        raise RuntimeError("State resource did not report paused=false.")

  RequiredFields = {
    "sceneName",
    "fps",
    "paused",
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
      "pause": PauseData,
      "repeatedPause": RepeatedPauseData,
      "pausedState": PausedState,
      "resume": ResumeData,
      "repeatedResume": RepeatedResumeData,
      "resumedState": ResumedState,
    },
    "discovery": DiscoveryResults,
  }


def main() -> int:
  State = asyncio.run(run_integration())
  print(json.dumps(State, ensure_ascii=False, indent=2))
  return 0


if __name__ == "__main__":
  sys.exit(main())
