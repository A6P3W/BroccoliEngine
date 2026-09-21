"""Live Control integration check against a running BROCCOLI ENGINE."""

from __future__ import annotations

import json
import sys
from math import isclose

from broccoli_control.client import ControlClient
from broccoli_control.config import ControlConfig


def run_integration() -> dict[str, object]:
  with ControlClient(ControlConfig()) as Engine:
    State = Engine.get_state()
    Actors = Engine.get_actors()
    Logs = Engine.get_recent_logs()

    SystemCommands = Engine.get_system_commands()
    RegisteredClasses = Engine.get_registered_actor_classes()
    Levels = Engine.get_levels()
    FoundActors = Engine.find_actors()
    if not RegisteredClasses.Classes:
      raise RuntimeError("get_registered_actor_classes returned no registered classes.")

    DiscoveryClassName = RegisteredClasses.Classes[0].ClassName
    ClassMethods = Engine.get_class_methods(DiscoveryClassName)
    ActorDetails = None
    ActorComponents = None
    if Actors.Actors:
      DiscoveryActorId = Actors.Actors[0].ActorId
      ActorDetails = Engine.get_actor(DiscoveryActorId)
      ActorComponents = Engine.get_actor_components(DiscoveryActorId)

    LevelStarterActors = [
      Actor for Actor in Actors.Actors if Actor.ClassName == "ALevelStarterWidget"
    ]
    ActorId = LevelStarterActors[0].ActorId if LevelStarterActors else None
    ActorMethods = Engine.get_actor_methods(ActorId).to_dict() if ActorId else None

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
        MutationResults["destroyedActor"] = Engine.destroy_actor(SpawnedActorId).to_dict()

    if ActorMethods and [Method["name"] for Method in ActorMethods["methods"]] != ["get_status"]:
      raise RuntimeError("LevelStarter actor method list is invalid.")
    CommandNames = {Command["name"] for Command in SystemCommands.to_dict()["commands"]}
    if not {
      "start_simulation",
      "stop_simulation",
      "open_level_by_id",
      "open_level_by_path",
    }.issubset(CommandNames) or {"pause_game", "resume_game"}.intersection(CommandNames):
      raise RuntimeError("System command list is invalid.")

    MethodData = None
    if ActorId:
      MethodData = Engine.invoke_actor_method(ActorId, "get_status").to_dict()
      if (
        MethodData.get("actorId") != ActorId
        or MethodData.get("methodName") != "get_status"
        or MethodData.get("result", {}).get("ready") is not True
      ):
        raise RuntimeError("invoke_actor_method returned invalid data.")

    StopData = Engine.execute_system_command("stop_simulation").to_dict()
    if (
      StopData.get("result", {}).get("worldAvailable") is not True
      or StopData.get("result", {}).get("changed") is not True
      or StopData.get("result", {}).get("simulating") is not False
    ):
      raise RuntimeError("stop_simulation did not stop the simulation.")
    RepeatedStopData = Engine.execute_system_command("stop_simulation").to_dict()
    if (
      RepeatedStopData.get("result", {}).get("changed") is not False
      or RepeatedStopData.get("result", {}).get("simulating") is not False
    ):
      raise RuntimeError("Repeated stop_simulation was not idempotent.")
    StoppedState = Engine.get_state()
    if StoppedState.Simulating is not False:
      raise RuntimeError("State endpoint did not report simulating=false.")

    StartData = Engine.execute_system_command("start_simulation").to_dict()
    if (
      StartData.get("result", {}).get("worldAvailable") is not True
      or StartData.get("result", {}).get("changed") is not True
      or StartData.get("result", {}).get("simulating") is not True
    ):
      raise RuntimeError("start_simulation did not start the simulation.")
    RepeatedStartData = Engine.execute_system_command("start_simulation").to_dict()
    if (
      RepeatedStartData.get("result", {}).get("changed") is not False
      or RepeatedStartData.get("result", {}).get("simulating") is not True
    ):
      raise RuntimeError("Repeated start_simulation was not idempotent.")
    StartedState = Engine.get_state()
    if StartedState.Simulating is not True:
      raise RuntimeError("State endpoint did not report simulating=true.")

  return {
    "state": State.to_dict(),
    "worldActors": Actors.to_dict(),
    "recentLogs": Logs.to_dict(),
    "actorMethod": MethodData,
    "actorMethods": ActorMethods,
    "systemCommandList": SystemCommands.to_dict(),
    "mutations": MutationResults,
    "systemCommands": {
      "stop": StopData,
      "repeatedStop": RepeatedStopData,
      "stoppedState": StoppedState.to_dict(),
      "start": StartData,
      "repeatedStart": RepeatedStartData,
      "startedState": StartedState.to_dict(),
    },
    "discovery": {
      "registeredActorClasses": RegisteredClasses.to_dict(),
      "levels": Levels.to_dict(),
      "actors": FoundActors.to_dict(),
      "classMethods": ClassMethods.to_dict(),
      "actor": ActorDetails.to_dict() if ActorDetails else None,
      "actorComponents": ActorComponents.to_dict() if ActorComponents else None,
    },
  }


def main() -> int:
  Result = run_integration()
  print(json.dumps(Result, ensure_ascii=False, indent=2))
  return 0


if __name__ == "__main__":
  sys.exit(main())
