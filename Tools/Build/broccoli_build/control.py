"""JSON command-line adapter for the shared BROCCOLI ENGINE control client."""

from __future__ import annotations

import argparse
import json
import sys
from collections.abc import Callable, Mapping
from typing import Any

from broccoli_control import ControlClient, load_config
from broccoli_control.errors import ControlError


def JsonObjectArgument(Value: str) -> dict[str, object]:
  """Parse a JSON object passed as a command argument."""

  try:
    Parsed = json.loads(Value)
  except json.JSONDecodeError as Error:
    raise argparse.ArgumentTypeError(f"Invalid JSON object: {Error.msg}") from None
  if not isinstance(Parsed, dict):
    raise argparse.ArgumentTypeError("Arguments must be a JSON object.")
  return Parsed


def CreateParser() -> argparse.ArgumentParser:
  """Create the control command parser without any transport logic."""

  Parser = argparse.ArgumentParser(description="Control a running BROCCOLI ENGINE instance")
  Commands = Parser.add_subparsers(dest="Command", required=True)

  Commands.add_parser("state")
  Commands.add_parser("actors")
  FindActorsParser = Commands.add_parser("find-actors")
  FindActorsParser.add_argument("--class", dest="class_name")
  FindActorsParser.add_argument("--name", dest="instance_name")
  ActorParser = Commands.add_parser("actor")
  ActorParser.add_argument("actor_id", type=int)
  ComponentsParser = Commands.add_parser("actor-components")
  ComponentsParser.add_argument("actor_id", type=int)
  Commands.add_parser("actor-classes")
  Commands.add_parser("levels")
  ClassMethodsParser = Commands.add_parser("class-methods")
  ClassMethodsParser.add_argument("class_name")
  ActorMethodsParser = Commands.add_parser("actor-methods")
  ActorMethodsParser.add_argument("actor_id", type=int)

  SpawnParser = Commands.add_parser("spawn-actor")
  SpawnParser.add_argument("class_name")
  SpawnParser.add_argument("--x", type=float, default=0.0)
  SpawnParser.add_argument("--y", type=float, default=0.0)
  SpawnParser.add_argument("--rotation", type=float, default=0.0)
  SpawnParser.add_argument("--scale", type=float, default=1.0)
  SpawnParser.add_argument("--name", dest="instance_name")
  DestroyParser = Commands.add_parser("destroy-actor")
  DestroyParser.add_argument("actor_id", type=int)
  TransformParser = Commands.add_parser("set-transform")
  TransformParser.add_argument("actor_id", type=int)
  TransformParser.add_argument("--x", type=float)
  TransformParser.add_argument("--y", type=float)
  TransformParser.add_argument("--rotation", type=float)
  TransformParser.add_argument("--scale", type=float)
  CallParser = Commands.add_parser("call")
  CallParser.add_argument("actor_id", type=int)
  CallParser.add_argument("method_name")
  CallParser.add_argument("--arguments", type=JsonObjectArgument, default={})
  ComponentMethodsParser = Commands.add_parser("component-methods")
  ComponentMethodsParser.add_argument("actor_id", type=int)
  ComponentMethodsParser.add_argument("component_id", type=int)
  ComponentCallParser = Commands.add_parser("component-call")
  ComponentCallParser.add_argument("actor_id", type=int)
  ComponentCallParser.add_argument("component_id", type=int)
  ComponentCallParser.add_argument("method_name")
  ComponentCallParser.add_argument("--arguments", type=JsonObjectArgument, default={})
  Commands.add_parser("system-commands")
  SystemCallParser = Commands.add_parser("system-call")
  SystemCallParser.add_argument("command_name")
  SystemCallParser.add_argument("--arguments", type=JsonObjectArgument, default={})
  LogsParser = Commands.add_parser("logs")
  LogsParser.add_argument("--limit", type=int, default=100)
  LogsParser.add_argument("--level")
  LogsParser.add_argument("--after-sequence", type=int)
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
      LocationX=Arguments.x,
      LocationY=Arguments.y,
      Rotation=Arguments.rotation,
      Scale=Arguments.scale,
      InstanceName=Arguments.instance_name,
    ).to_dict(),
    "destroy-actor": lambda: Client.destroy_actor(Arguments.actor_id).to_dict(),
    "set-transform": lambda: Client.set_actor_transform(
      Arguments.actor_id,
      LocationX=Arguments.x,
      LocationY=Arguments.y,
      Rotation=Arguments.rotation,
      Scale=Arguments.scale,
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
    Value is None for Value in (Arguments.x, Arguments.y, Arguments.rotation, Arguments.scale)
  ):
    raise ValueError("Specify at least one transform value.")
  return CommandHandlers[Arguments.Command]()


def Run(Arguments: list[str] | None = None) -> int:
  """Run the CLI adapter and produce JSON on stdout or one error on stderr."""

  try:
    ParsedArguments = CreateParser().parse_args(Arguments)
    Config = load_config([])
    with ControlClient(Config) as Client:
      Result = Dispatch(Client, ParsedArguments)
  except (ControlError, ValueError) as Error:
    print(Error, file=sys.stderr)
    return 1
  print(json.dumps(Result, ensure_ascii=False, separators=(",", ":")))
  return 0
