"""JSON command-line adapter for the shared BROCCOLI ENGINE control client."""

from __future__ import annotations

import argparse
import ctypes
import json
import os
import sys
from collections.abc import Callable, Mapping
from dataclasses import dataclass, replace
from pathlib import Path
from typing import Any

from broccoli_control import ControlClient, ControlConfig, load_config
from broccoli_control.errors import ControlError


@dataclass(frozen=True, slots=True)
class ControlRegistration:
  """A validated control endpoint registered by a running engine process."""

  ProcessId: int
  Port: int


def _GetProcessExecutablePath(ProcessId: int) -> Path | None:
  """Return a process executable path without requiring administrator privileges."""

  if os.name != "nt":
    return None
  ProcessQueryLimitedInformation = 0x1000
  ProcessHandle = ctypes.windll.kernel32.OpenProcess(  # type: ignore[attr-defined]
    ProcessQueryLimitedInformation, False, ProcessId
  )
  if not ProcessHandle:
    return None
  try:
    Buffer = ctypes.create_unicode_buffer(32768)
    Size = ctypes.c_uint32(len(Buffer))
    if not ctypes.windll.kernel32.QueryFullProcessImageNameW(  # type: ignore[attr-defined]
      ProcessHandle, 0, Buffer, ctypes.byref(Size)
    ):
      return None
    return Path(Buffer.value)
  finally:
    ctypes.windll.kernel32.CloseHandle(ProcessHandle)  # type: ignore[attr-defined]


def _PathsMatch(First: Path, Second: Path) -> bool:
  return os.path.normcase(os.path.abspath(First)) == os.path.normcase(os.path.abspath(Second))


def _ReadRegistration(RegistrationPath: Path, TargetExecutable: Path) -> ControlRegistration | None:
  try:
    ProcessId = int(RegistrationPath.stem)
    Data = json.loads(RegistrationPath.read_text(encoding="utf-8"))
  except (OSError, ValueError, json.JSONDecodeError):
    return None
  if not isinstance(Data, dict):
    return None
  RegisteredProcessId = Data.get("pid")
  Port = Data.get("port")
  if (
    isinstance(RegisteredProcessId, bool)
    or not isinstance(RegisteredProcessId, int)
    or RegisteredProcessId != ProcessId
    or isinstance(Port, bool)
    or not isinstance(Port, int)
    or not 1 <= Port <= 65535
  ):
    return None
  ExecutablePath = _GetProcessExecutablePath(ProcessId)
  if ExecutablePath is None or not _PathsMatch(ExecutablePath, TargetExecutable):
    return None
  return ControlRegistration(ProcessId, Port)


def ResolveTargetExecutable(ProjectDirectory: Path) -> Path:
  """Resolve the executable produced by the most recent build of this project."""

  ConfigurationPath = ProjectDirectory / "Intermediate" / "LastBuildConfiguration.txt"
  try:
    Configuration = ConfigurationPath.read_text(encoding="utf-8").strip()
  except OSError as Error:
    raise ValueError(f"Latest build configuration does not exist: {ConfigurationPath}") from Error
  if Configuration not in {"Debug", "Editor", "Release"}:
    raise ValueError(f"Unsupported latest build configuration: {Configuration}")

  SettingsPath = ProjectDirectory / ".broccoli-project.json"
  try:
    Settings = json.loads(SettingsPath.read_text(encoding="utf-8"))
  except (OSError, json.JSONDecodeError) as Error:
    raise ValueError(f"Could not read project settings '{SettingsPath}': {Error}") from Error
  ProjectName = Settings.get("project_name", "Launcher")
  if not isinstance(ProjectName, str) or not ProjectName.strip():
    raise ValueError(f"Project setting 'project_name' must be a non-empty string: {SettingsPath}")
  if Configuration == "Editor":
    return ProjectDirectory / "Bin" / "x64" / Configuration / f"{ProjectName}-game.exe"
  return ProjectDirectory / "Publish" / Configuration / "Binaries" / f"{ProjectName}.exe"


def ResolveRegistration(TargetExecutable: Path, ProcessId: int | None) -> ControlRegistration:
  """Find exactly one live registration belonging to the selected executable."""

  ControlDirectory = TargetExecutable.parent / "control"
  if ProcessId is not None:
    Registration = _ReadRegistration(ControlDirectory / f"{ProcessId}.json", TargetExecutable)
    if Registration is None:
      raise ValueError(f"No running BROCCOLI ENGINE instance exists for PID {ProcessId}.")
    return Registration

  if not ControlDirectory.is_dir():
    raise ValueError(f"No running BROCCOLI ENGINE instance was found for: {TargetExecutable}")
  Registrations = [
    Registration
    for RegistrationPath in ControlDirectory.glob("*.json")
    if (Registration := _ReadRegistration(RegistrationPath, TargetExecutable)) is not None
  ]
  if not Registrations:
    raise ValueError(f"No running BROCCOLI ENGINE instance was found for: {TargetExecutable}")
  if len(Registrations) == 1:
    return Registrations[0]

  ProcessIds = "\n".join(f"PID {Registration.ProcessId}" for Registration in Registrations)
  raise ValueError(
    "Multiple BROCCOLI ENGINE instances are running.\n\n"
    f"{ProcessIds}\n\n"
    "Specify the target PID:\n"
    f"  broccoli.bat control --pid {Registrations[0].ProcessId} state"
  )


def ResolveControlConfig(ProjectDirectory: Path, ProcessId: int | None) -> ControlConfig:
  """Create a client configuration using current discovery data and user timeout settings."""

  Registration = ResolveRegistration(ResolveTargetExecutable(ProjectDirectory), ProcessId)
  return replace(load_config([]), Port=Registration.Port)


def JsonObjectArgument(Value: str) -> dict[str, object]:
  """Parse a JSON object passed as a command argument."""

  try:
    Parsed = json.loads(Value)
  except json.JSONDecodeError as Error:
    raise argparse.ArgumentTypeError(f"Invalid JSON object: {Error.msg}") from None
  if not isinstance(Parsed, dict):
    raise argparse.ArgumentTypeError("Arguments must be a JSON object.")
  return Parsed


def JsonOrFloatArgument(Value: str) -> dict[str, object] | float:
  """Parse a JSON object or a float passed as a command argument."""

  try:
    Parsed = json.loads(Value)
    if isinstance(Parsed, (dict, int, float)) and not isinstance(Parsed, bool):
      return Parsed
  except json.JSONDecodeError:
    pass
  try:
    return float(Value)
  except ValueError:
    raise argparse.ArgumentTypeError(f"Invalid JSON object or number: '{Value}'") from None


def CreateParser() -> argparse.ArgumentParser:
  """Create the control command parser without any transport logic."""

  Parser = argparse.ArgumentParser(
    prog="broccoli.bat control",
    description="Control a running BROCCOLI ENGINE instance",
  )
  Parser.add_argument("--pid", type=int, help="PID of the target BROCCOLI ENGINE instance")
  Commands = Parser.add_subparsers(
    dest="Command",
    required=True,
    title="subcommands",
    description="Available control operations",
    metavar="<command>",
  )

  Commands.add_parser(
    "state",
    help="Get current engine scene, FPS, and physics state",
    description="Get current engine scene, FPS, and physics state.",
  )
  Commands.add_parser(
    "actors",
    help="List all actors in the current world with transform info",
    description="List all actors in the current world with transform info.",
  )
  FindActorsParser = Commands.add_parser(
    "find-actors",
    help="Find actors by class name or instance name",
    description="Find actors by class name or instance name.",
  )
  FindActorsParser.add_argument("--class", dest="class_name", help="Actor class name to filter by")
  FindActorsParser.add_argument("--name", dest="instance_name", help="Actor instance name to filter by")
  ActorParser = Commands.add_parser(
    "actor",
    help="Get actor details by actor ID",
    description="Get actor details by actor ID.",
  )
  ActorParser.add_argument("actor_id", type=int, help="Target actor ID")
  ComponentsParser = Commands.add_parser(
    "actor-components",
    help="List components held by an actor",
    description="List components held by an actor.",
  )
  ComponentsParser.add_argument("actor_id", type=int, help="Target actor ID")
  Commands.add_parser(
    "actor-classes",
    help="List registered actor classes",
    description="List registered actor classes in BROCCOLI ENGINE.",
  )
  Commands.add_parser(
    "levels",
    help="List registered levels",
    description="List registered levels in BROCCOLI ENGINE.",
  )
  ClassMethodsParser = Commands.add_parser(
    "class-methods",
    help="List automation methods for an actor class",
    description="List automation methods registered for an actor class.",
  )
  ClassMethodsParser.add_argument("class_name", help="Actor class name")
  ActorMethodsParser = Commands.add_parser(
    "actor-methods",
    help="List automation methods for an actor instance",
    description="List automation methods available on an actor instance.",
  )
  ActorMethodsParser.add_argument("actor_id", type=int, help="Target actor ID")

  SpawnParser = Commands.add_parser(
    "spawn-actor",
    help="Spawn an actor in the current world",
    description="Spawn an actor in the current world with location, rotation, and scale.",
  )
  SpawnParser.add_argument("class_name", help="Actor class name to spawn")
  SpawnParser.add_argument(
    "--location",
    type=JsonObjectArgument,
    help='3D location as a JSON object, e.g. \'{"x": 0.0, "y": 1.0, "z": 2.0}\'',
  )
  SpawnParser.add_argument(
    "--rotation",
    type=JsonOrFloatArgument,
    help='Euler rotation in degrees as a JSON object (e.g. \'{"pitch": 0, "yaw": 0, "roll": 90}\') or a single number for 2D roll',
  )
  SpawnParser.add_argument(
    "--scale",
    type=JsonOrFloatArgument,
    help='Scale as a JSON object (e.g. \'{"x": 1, "y": 1, "z": 1}\') or a single number for uniform scale',
  )
  SpawnParser.add_argument("--x", type=float, help="Location X coordinate")
  SpawnParser.add_argument("--y", type=float, help="Location Y coordinate")
  SpawnParser.add_argument("--z", type=float, help="Location Z coordinate")
  SpawnParser.add_argument("--pitch", type=float, help="Rotation Pitch in degrees (X axis)")
  SpawnParser.add_argument("--yaw", type=float, help="Rotation Yaw in degrees (Y axis)")
  SpawnParser.add_argument("--roll", type=float, help="Rotation Roll in degrees (Z axis)")
  SpawnParser.add_argument("--scale-x", dest="scale_x", type=float, help="Scale X factor")
  SpawnParser.add_argument("--scale-y", dest="scale_y", type=float, help="Scale Y factor")
  SpawnParser.add_argument("--scale-z", dest="scale_z", type=float, help="Scale Z factor")
  SpawnParser.add_argument("--name", dest="instance_name", help="Optional instance name")
  DestroyParser = Commands.add_parser(
    "destroy-actor",
    help="Destroy an actor by ID",
    description="Request destruction of an actor in the current world.",
  )
  DestroyParser.add_argument("actor_id", type=int, help="Actor ID to destroy")
  TransformParser = Commands.add_parser(
    "set-transform",
    help="Partially or fully update an actor's transform",
    description="Partially or fully update an actor's transform (3D location, Euler rotation, scale).",
  )
  TransformParser.add_argument("actor_id", type=int, help="Target actor ID")
  TransformParser.add_argument(
    "--location",
    type=JsonObjectArgument,
    help='3D location as a JSON object, e.g. \'{"x": 0.0, "y": 1.0, "z": 2.0}\'',
  )
  TransformParser.add_argument(
    "--rotation",
    type=JsonOrFloatArgument,
    help='Euler rotation in degrees as a JSON object (e.g. \'{"pitch": 0, "yaw": 0, "roll": 90}\') or a single number for 2D roll',
  )
  TransformParser.add_argument(
    "--scale",
    type=JsonOrFloatArgument,
    help='Scale as a JSON object (e.g. \'{"x": 1, "y": 1, "z": 1}\') or a single number for uniform scale',
  )
  TransformParser.add_argument("--x", type=float, help="Location X coordinate")
  TransformParser.add_argument("--y", type=float, help="Location Y coordinate")
  TransformParser.add_argument("--z", type=float, help="Location Z coordinate")
  TransformParser.add_argument("--pitch", type=float, help="Rotation Pitch in degrees (X axis)")
  TransformParser.add_argument("--yaw", type=float, help="Rotation Yaw in degrees (Y axis)")
  TransformParser.add_argument("--roll", type=float, help="Rotation Roll in degrees (Z axis)")
  TransformParser.add_argument("--scale-x", dest="scale_x", type=float, help="Scale X factor")
  TransformParser.add_argument("--scale-y", dest="scale_y", type=float, help="Scale Y factor")
  TransformParser.add_argument("--scale-z", dest="scale_z", type=float, help="Scale Z factor")
  CallParser = Commands.add_parser(
    "call",
    help="Invoke an automation method on an actor",
    description="Invoke a registered automation method on an actor.",
  )
  CallParser.add_argument("actor_id", type=int, help="Target actor ID")
  CallParser.add_argument("method_name", help="Method name to invoke")
  CallParser.add_argument(
    "--arguments",
    type=JsonObjectArgument,
    default={},
    help="Method arguments as a JSON object",
  )
  ComponentMethodsParser = Commands.add_parser(
    "component-methods",
    help="List automation methods for a component",
    description="List registered automation methods for a component on an actor.",
  )
  ComponentMethodsParser.add_argument("actor_id", type=int, help="Target actor ID")
  ComponentMethodsParser.add_argument("component_id", type=int, help="Target component ID")
  ComponentCallParser = Commands.add_parser(
    "component-call",
    help="Invoke an automation method on a component",
    description="Invoke a registered automation method on a component of an actor.",
  )
  ComponentCallParser.add_argument("actor_id", type=int, help="Target actor ID")
  ComponentCallParser.add_argument("component_id", type=int, help="Target component ID")
  ComponentCallParser.add_argument("method_name", help="Component method name to invoke")
  ComponentCallParser.add_argument(
    "--arguments",
    type=JsonObjectArgument,
    default={},
    help="Component method arguments as a JSON object",
  )
  Commands.add_parser(
    "system-commands",
    help="List available system commands",
    description="List registered BROCCOLI ENGINE system commands.",
  )
  SystemCallParser = Commands.add_parser(
    "system-call",
    help="Execute a registered system command",
    description="Execute a registered BROCCOLI ENGINE system command (e.g. open_level_by_path).",
  )
  SystemCallParser.add_argument("command_name", help="System command name")
  SystemCallParser.add_argument(
    "--arguments",
    type=JsonObjectArgument,
    default={},
    help="System command arguments as a JSON object",
  )
  LogsParser = Commands.add_parser(
    "logs",
    help="Get recent in-memory engine logs",
    description="Get recent in-memory engine logs.",
  )
  LogsParser.add_argument("--limit", type=int, default=100, help="Maximum number of log entries (1-1000, default: 100)")
  LogsParser.add_argument("--level", help="Filter by minimum log level (debug, info, warning, error)")
  LogsParser.add_argument("--after-sequence", type=int, help="Return entries logged after this sequence number")
  return Parser


def Dispatch(Client: ControlClient, Arguments: argparse.Namespace) -> Mapping[str, Any]:
  """Map a parsed CLI command to one ControlClient operation."""

  CommandHandlers: dict[str, Callable[[], Mapping[str, Any]]] = {
    "state": lambda: Client.get_state().to_dict(),
    "actors": lambda: Client.get_actors().to_dict(),
    "find-actors": lambda: Client.find_actors(
      ClassName=Arguments.class_name, InstanceName=Arguments.instance_name
    ).to_dict(),
    "actor": lambda: Client.get_actor(Arguments.actor_id).to_dict(),
    "actor-components": lambda: Client.get_actor_components(Arguments.actor_id).to_dict(),
    "actor-classes": lambda: Client.get_registered_actor_classes().to_dict(),
    "levels": lambda: Client.get_levels().to_dict(),
    "class-methods": lambda: Client.get_class_methods(Arguments.class_name).to_dict(),
    "actor-methods": lambda: Client.get_actor_methods(Arguments.actor_id).to_dict(),
    "spawn-actor": lambda: Client.spawn_actor(
      Arguments.class_name,
      Location=Arguments.location,
      Rotation=Arguments.rotation,
      Scale=Arguments.scale,
      LocationX=Arguments.x,
      LocationY=Arguments.y,
      LocationZ=Arguments.z,
      Pitch=Arguments.pitch,
      Yaw=Arguments.yaw,
      Roll=Arguments.roll,
      ScaleX=Arguments.scale_x,
      ScaleY=Arguments.scale_y,
      ScaleZ=Arguments.scale_z,
      InstanceName=Arguments.instance_name,
    ).to_dict(),
    "destroy-actor": lambda: Client.destroy_actor(Arguments.actor_id).to_dict(),
    "set-transform": lambda: Client.set_actor_transform(
      Arguments.actor_id,
      Location=Arguments.location,
      Rotation=Arguments.rotation,
      Scale=Arguments.scale,
      LocationX=Arguments.x,
      LocationY=Arguments.y,
      LocationZ=Arguments.z,
      Pitch=Arguments.pitch,
      Yaw=Arguments.yaw,
      Roll=Arguments.roll,
      ScaleX=Arguments.scale_x,
      ScaleY=Arguments.scale_y,
      ScaleZ=Arguments.scale_z,
    ).to_dict(),
    "call": lambda: Client.invoke_actor_method(
      Arguments.actor_id, Arguments.method_name, Arguments=Arguments.arguments
    ).to_dict(),
    "component-methods": lambda: Client.get_component_methods(
      Arguments.actor_id, Arguments.component_id
    ),
    "component-call": lambda: Client.invoke_component_method(
      Arguments.actor_id,
      Arguments.component_id,
      Arguments.method_name,
      Arguments.arguments,
    ),
    "system-commands": lambda: Client.get_system_commands().to_dict(),
    "system-call": lambda: Client.execute_system_command(
      Arguments.command_name, Arguments=Arguments.arguments
    ).to_dict(),
    "logs": lambda: Client.get_recent_logs(
      Limit=Arguments.limit,
      Level=Arguments.level,
      AfterSequence=Arguments.after_sequence,
    ).to_dict(),
  }
  if Arguments.Command == "set-transform" and all(
    Value is None
    for Value in (
      Arguments.location,
      Arguments.rotation,
      Arguments.scale,
      Arguments.x,
      Arguments.y,
      Arguments.z,
      Arguments.pitch,
      Arguments.yaw,
      Arguments.roll,
      Arguments.scale_x,
      Arguments.scale_y,
      Arguments.scale_z,
    )
  ):
    raise ValueError("Specify at least one transform value.")
  return CommandHandlers[Arguments.Command]()


def Run(Arguments: list[str] | None = None, ProjectDirectory: Path | None = None) -> int:
  """Run the CLI adapter and produce JSON on stdout or one error on stderr."""

  try:
    ParsedArguments = CreateParser().parse_args(Arguments)
    if ParsedArguments.pid is not None and ParsedArguments.pid <= 0:
      raise ValueError("PID must be a positive integer.")
    Config = ResolveControlConfig(ProjectDirectory or Path.cwd(), ParsedArguments.pid)
    with ControlClient(Config) as Client:
      Result = Dispatch(Client, ParsedArguments)
  except (ControlError, ValueError) as Error:
    print(Error, file=sys.stderr)
    return 1
  print(json.dumps(Result, ensure_ascii=False, separators=(",", ":")))
  return 0
