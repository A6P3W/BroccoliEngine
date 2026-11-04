"""Non-UI end-to-end update-control checks against a hidden Debug Engine."""

from __future__ import annotations

import ctypes
import json
import subprocess
import sys
import time
from ctypes import wintypes
from math import isclose
from pathlib import Path
from typing import Any

from broccoli_control.client import ControlClient
from broccoli_control.config import ControlConfig
from broccoli_control.errors import EngineApiError, EngineUnavailable

ROOT = Path(__file__).resolve().parents[3]
RESULT_PATH = ROOT / "Intermediate" / "EngineUpdateControlIntegration.json"
STEP_DELTA_TIME = 0.025


def RunBuildCli(*Arguments: str) -> subprocess.CompletedProcess[str]:
  CommandLine = subprocess.list2cmdline([str(ROOT / "broccoli.bat"), *Arguments])
  return subprocess.run(
    CommandLine,
    cwd=ROOT,
    check=True,
    capture_output=True,
    text=True,
    shell=True,
    timeout=30,
  )


def IsProcessRunning(ProcessId: int) -> bool:
  Kernel32 = ctypes.WinDLL("kernel32", use_last_error=True)
  Kernel32.OpenProcess.argtypes = [wintypes.DWORD, wintypes.BOOL, wintypes.DWORD]
  Kernel32.OpenProcess.restype = wintypes.HANDLE
  Kernel32.GetExitCodeProcess.argtypes = [wintypes.HANDLE, ctypes.POINTER(wintypes.DWORD)]
  Kernel32.GetExitCodeProcess.restype = wintypes.BOOL
  Kernel32.CloseHandle.argtypes = [wintypes.HANDLE]
  Kernel32.CloseHandle.restype = wintypes.BOOL
  ProcessHandle = Kernel32.OpenProcess(0x1000, False, ProcessId)
  if not ProcessHandle:
    return False
  try:
    ExitCode = ctypes.c_ulong()
    if not Kernel32.GetExitCodeProcess(ProcessHandle, ctypes.byref(ExitCode)):
      raise ctypes.WinError(ctypes.get_last_error())
    return ExitCode.value == 259
  finally:
    Kernel32.CloseHandle(ProcessHandle)


def TerminateOwnedProcess(ProcessId: int) -> None:
  Kernel32 = ctypes.WinDLL("kernel32", use_last_error=True)
  Kernel32.OpenProcess.argtypes = [wintypes.DWORD, wintypes.BOOL, wintypes.DWORD]
  Kernel32.OpenProcess.restype = wintypes.HANDLE
  Kernel32.TerminateProcess.argtypes = [wintypes.HANDLE, wintypes.UINT]
  Kernel32.TerminateProcess.restype = wintypes.BOOL
  Kernel32.CloseHandle.argtypes = [wintypes.HANDLE]
  Kernel32.CloseHandle.restype = wintypes.BOOL
  ProcessHandle = Kernel32.OpenProcess(0x0001, False, ProcessId)
  if not ProcessHandle:
    return
  try:
    if IsProcessRunning(ProcessId) and not Kernel32.TerminateProcess(ProcessHandle, 1):
      raise ctypes.WinError(ctypes.get_last_error())
  finally:
    Kernel32.CloseHandle(ProcessHandle)


def WaitForRegistration(RegistrationPath: Path, ProcessId: int) -> dict[str, int]:
  Deadline = time.monotonic() + 20
  while time.monotonic() < Deadline:
    if not IsProcessRunning(ProcessId):
      raise RuntimeError("Engine exited before publishing its Control registration.")
    try:
      Data = json.loads(RegistrationPath.read_text(encoding="utf-8"))
      if Data.get("pid") == ProcessId and isinstance(Data.get("port"), int):
        return Data
    except (OSError, json.JSONDecodeError):
      pass
    time.sleep(0.1)
  raise TimeoutError(f"Timed out waiting for Control registration: {RegistrationPath}")


def RunIntegration() -> dict[str, Any]:
  Launch = RunBuildCli(
    "run",
    "Debug",
    "--control",
    "--json",
    "--",
    "--window-hidden",
    "--engine-update",
    "stopped",
    "--step-delta-time",
    str(STEP_DELTA_TIME),
  )
  LaunchData = json.loads(Launch.stdout)
  ProcessId = LaunchData["pid"]
  Executable = Path(LaunchData["executable"])
  RegistrationPath = Executable.parent / "control" / f"{ProcessId}.json"
  Registration: dict[str, int] | None = None
  Results: dict[str, Any] = {
    "launch": LaunchData,
  }
  PhysicalActorId: int | None = None
  DoorActorId: int | None = None
  Engine: ControlClient | None = None

  try:
    Registration = WaitForRegistration(RegistrationPath, ProcessId)
    Results["initialRegistration"] = Registration
    CliState = json.loads(RunBuildCli("control", "--pid", str(ProcessId), "state").stdout)
    if CliState.get("engineUpdate") != "stopped":
      raise RuntimeError("broccoli.bat control --pid did not reach the launched Debug Engine.")
    Results["cliState"] = CliState
    Engine = ControlClient(ControlConfig(Port=Registration["port"]))
    with Engine:
      Initial = Engine.get_state()
      if Initial.EngineUpdate != "stopped" or not Initial.EngineStopped:
        raise RuntimeError("Engine did not start in the stopped state.")
      if not isclose(Initial.StepDeltaTime, STEP_DELTA_TIME, rel_tol=0.0, abs_tol=1e-7):
        raise RuntimeError(f"Unexpected fixed step delta time: {Initial.StepDeltaTime}")

      SpawnedActor = Engine.spawn_actor(
        "ADoorActor", InstanceName="UpdateControlProbe", LocationX=12.0
      )
      SpawnedActorId = SpawnedActor.ActorId
      if Engine.get_actors().ActorCount != Initial.ActorCount + 1:
        raise RuntimeError("Spawned actor was not immediately visible through actor enumeration.")
      if Engine.get_actor(SpawnedActorId).InstanceName != "UpdateControlProbe":
        raise RuntimeError("Actor mutation did not apply while automatic updates were stopped.")
      Engine.destroy_actor(SpawnedActorId)
      SpawnedActorId = None
      if Engine.get_actors().ActorCount != Initial.ActorCount:
        raise RuntimeError("Destroyed actor remained visible through actor enumeration.")
      Door = Engine.spawn_actor("ADoorActor", InstanceName="UpdateConflictProbe", LocationX=24.0)
      DoorActorId = Door.ActorId

      PhysicalActor = Engine.spawn_actor(
        "APhysics3DDynamicBoxActor", LocationX=1000.0, LocationY=100.0, LocationZ=1000.0
      )
      PhysicalActorId = PhysicalActor.ActorId
      Engine.set_actor_transform(
        PhysicalActorId,
        Location={"x": 1000.0, "y": 100.0, "z": 1000.0},
      )
      Engine.execute_system_command("stop_simulation")
      BeforeStoppedPhysics = Engine.get_actor(PhysicalActorId).Transform.LocationY
      InactiveStep = Engine.step_engine_update(3)
      FinishedInactiveStep = Engine.wait_for_operation(InactiveStep.OperationId, TimeoutSeconds=20)
      AfterStoppedPhysics = Engine.get_actor(PhysicalActorId).Transform.LocationY
      if FinishedInactiveStep.Status != "completed" or not isclose(
        BeforeStoppedPhysics, AfterStoppedPhysics, rel_tol=0.0, abs_tol=0.0001
      ):
        raise RuntimeError("A paused simulation changed the dynamic body during Engine steps.")
      Results["pausedPhysics"] = {
        "beforeY": BeforeStoppedPhysics,
        "afterY": AfterStoppedPhysics,
        "operation": FinishedInactiveStep.to_dict(),
      }

      SimulationStart = Engine.execute_system_command("start_simulation").to_dict()
      if SimulationStart["result"].get("simulating") is not True:
        raise RuntimeError("Simulation did not start before fixed updates.")

      BeforeStep = Engine.get_state()
      Step = Engine.step_engine_update(60)
      FinishedStep = Engine.wait_for_operation(Step.OperationId, TimeoutSeconds=20)
      AfterStep = Engine.get_state()
      AfterRunningPhysics = Engine.get_actor(PhysicalActorId).Transform.LocationY
      if FinishedStep.Status != "completed" or FinishedStep.CompletedCount != 60:
        raise RuntimeError(f"Sixty update steps did not complete: {FinishedStep.to_dict()}")
      if AfterStep.CompletedEngineUpdates - BeforeStep.CompletedEngineUpdates != 60:
        raise RuntimeError("The fixed step advanced an unexpected number of Engine updates.")
      if AfterStep.EngineUpdate != "stopped" or not AfterStep.Simulating:
        raise RuntimeError("Step completion changed the stopped or simulation state unexpectedly.")
      if AfterRunningPhysics >= AfterStoppedPhysics - 0.25:
        raise RuntimeError("The simulated dynamic body did not fall during controlled updates.")
      Results["fixedStep"] = {
        "operation": FinishedStep.to_dict(),
        "state": AfterStep.to_dict(),
        "physicsY": AfterRunningPhysics,
      }
      Engine.execute_system_command("stop_simulation")

      Engine.start_engine_update()
      RunningState = Engine.get_state()
      Stop = Engine.stop_engine_update()
      FinishedStop = Engine.wait_for_operation(Stop.OperationId, TimeoutSeconds=20)
      StoppedState = Engine.get_state()
      if FinishedStop.Status != "completed" or StoppedState.EngineUpdate != "stopped":
        raise RuntimeError("stop_engine_update did not settle at a stopped update boundary.")
      Results["stop"] = {"operation": FinishedStop.to_dict(), "state": StoppedState.to_dict()}
      Results["runningBeforeStop"] = RunningState.to_dict()

      try:
        Engine.execute_system_command("step_engine_update", {"count": 1_000_001})
      except EngineApiError as Error:
        if Error.Code != "INVALID_ARGUMENT" or Error.HttpStatus != 400:
          raise
      else:
        raise RuntimeError("The Engine accepted a step count above the schema maximum.")

      CancelStep = Engine.step_engine_update(1_000_000)
      OperationState = Engine.get_operation_status(CancelStep.OperationId)
      if OperationState.Status not in {"pending", "running"}:
        raise RuntimeError("The long step operation completed before the conflict check.")
      for Operation, Label in (
        (lambda: Engine.set_actor_transform(PhysicalActorId, LocationY=90.0), "Actor transform"),
        (lambda: Engine.invoke_actor_method(DoorActorId, "open_door"), "actor method"),
        (
          lambda: Engine.execute_system_command(
            "open_level_by_path",
            {"levelPath": "Resources/Launcher/BasicGameplay.BLevel", "afterUpdate": "run"},
          ),
          "level transition",
        ),
      ):
        try:
          Operation()
        except EngineApiError as Error:
          if Error.Code != "CONFLICT" or Error.HttpStatus != 409:
            raise
        else:
          raise RuntimeError(f"{Label} mutation was accepted during a fixed step operation.")
      CancelStop = Engine.stop_engine_update()
      CancelledStep = Engine.wait_for_operation(CancelStep.OperationId, TimeoutSeconds=20)
      FinishedCancelStop = Engine.wait_for_operation(CancelStop.OperationId, TimeoutSeconds=20)
      if CancelledStep.Status != "cancelled" or FinishedCancelStop.Status != "completed":
        raise RuntimeError("stop_engine_update did not cancel the active step operation.")
      Results["cancel"] = {
        "step": CancelledStep.to_dict(),
        "stop": FinishedCancelStop.to_dict(),
      }

      Engine.start_engine_update()
      TargetLevelPath = "Resources/Launcher/LevelStarter.BLevel"
      LevelResult = Engine.execute_system_command(
        "open_level_by_path",
        {"levelPath": TargetLevelPath, "afterUpdate": "stop"},
      )
      LevelOperationId = LevelResult.Result["operationId"]
      FinishedLevel = Engine.wait_for_operation(LevelOperationId, TimeoutSeconds=30)
      LevelStoppedState = Engine.get_state()
      if FinishedLevel.Status != "completed" or LevelStoppedState.EngineUpdate != "stopped":
        raise RuntimeError("A successful afterUpdate=stop level load did not stop Engine updates.")
      Results["levelAfterStop"] = {
        "levelPath": TargetLevelPath,
        "operation": FinishedLevel.to_dict(),
        "state": LevelStoppedState.to_dict(),
      }

      Engine.start_engine_update()
      try:
        FailedLevelResult = Engine.execute_system_command(
          "open_level_by_path",
          {
            "levelPath": "Resources/Launcher/EngineUpdateControlMissing.BLevel",
            "afterUpdate": "stop",
          },
        )
        FailedLevel = Engine.wait_for_operation(
          FailedLevelResult.Result["operationId"], TimeoutSeconds=30
        )
        FailedLevelState = Engine.get_state()
        if FailedLevel.Status != "failed" or FailedLevelState.EngineUpdate != "running":
          raise RuntimeError("A failed level load did not preserve the running update state.")
        Results["levelFailure"] = {
          "operation": FailedLevel.to_dict(),
          "state": FailedLevelState.to_dict(),
        }
      except EngineApiError as Error:
        raise RuntimeError(
          f"Missing level should be accepted and fail asynchronously, got {Error.Code}."
        ) from Error

      Results["logs"] = Engine.get_recent_logs(Limit=1000).to_dict()
      Results["finalState"] = Engine.get_state().to_dict()
    Engine = None
  finally:
    if Engine is not None:
      Engine.close()
      Engine = None
    if Registration is not None and IsProcessRunning(ProcessId):
      Engine = ControlClient(ControlConfig(Port=Registration["port"]))
      try:
        Results["cleanupQuit"] = Engine.execute_system_command("quit_game").to_dict()
      except EngineUnavailable as Error:
        Results["cleanupError"] = str(Error)
      except EngineApiError as Error:
        Results["cleanupError"] = str(Error)
      finally:
        Engine.close()
    Deadline = time.monotonic() + 20
    while IsProcessRunning(ProcessId) and time.monotonic() < Deadline:
      time.sleep(0.1)
    Results["processExited"] = not IsProcessRunning(ProcessId)
    Results["registrationRemoved"] = not RegistrationPath.exists()
    if not Results["processExited"]:
      TerminateOwnedProcess(ProcessId)
      Results["forcedTermination"] = True
      Results["processExited"] = not IsProcessRunning(ProcessId)
    if sys.exc_info()[1] is not None:
      Results["error"] = repr(sys.exc_info()[1])
    RESULT_PATH.parent.mkdir(parents=True, exist_ok=True)
    RESULT_PATH.write_text(json.dumps(Results, ensure_ascii=False, indent=2), encoding="utf-8")
  if not Results["processExited"]:
    raise TimeoutError(f"Engine process {ProcessId} could not be stopped.")
  if Results.get("forcedTermination"):
    raise RuntimeError(f"Engine process {ProcessId} required forced termination after quit_game.")
  if not Results["registrationRemoved"]:
    raise RuntimeError(f"Engine registration was not removed after PID {ProcessId} exited.")
  return Results


def main() -> int:
  Results = RunIntegration()
  print(
    json.dumps({"passed": True, "resultPath": str(RESULT_PATH), "pid": Results["launch"]["pid"]})
  )
  return 0


if __name__ == "__main__":
  sys.exit(main())
