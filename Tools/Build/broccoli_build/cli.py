"""Command-line interface for BroccoliEngine build processing."""

from __future__ import annotations

import argparse
import subprocess
import sys
from pathlib import Path

from .package_runtime import PackageRuntime
from .plugins import (
  GeneratePluginsCmake,
  LoadPluginConfigurations,
  RemoveDisabledExamplePluginArtifacts,
)
from .prepare_output import PrepareOutput
from .stage_runtime import StageRuntime
from .verify_runtime import VerifyRuntime


def PathArgument(Value: str) -> Path:
  return Path(Value).resolve()


def CreateParser() -> argparse.ArgumentParser:
  Parser = argparse.ArgumentParser(description="BroccoliEngine build and packaging tools")
  Commands = Parser.add_subparsers(dest="Command", required=True)

  PrepareParser = Commands.add_parser("prepare-output", help="Remove generated runtime output")
  PrepareParser.add_argument("--output-dir", type=PathArgument, required=True)

  PluginsParser = Commands.add_parser(
    "generate-plugins", help="Generate the CMake plugin selection for a project"
  )
  PluginsParser.add_argument("--project-dir", type=PathArgument, required=True)
  PluginsParser.add_argument("--changed-exit-code", action="store_true")

  StageParser = Commands.add_parser("stage-runtime", help="Stage local runtime dependencies")
  StageParser.add_argument("--configuration", required=True)
  StageParser.add_argument("--engine-dir", type=PathArgument, required=True)
  StageParser.add_argument("--game-dir", type=PathArgument, required=True)
  StageParser.add_argument("--output-dir", type=PathArgument, required=True)
  StageParser.add_argument("--engine-binary", type=PathArgument, required=True)
  StageParser.add_argument("--game-name", required=True)
  StageParser.add_argument("--eos-binary", type=PathArgument, required=True)
  StageParser.add_argument("--convert-levels-script", type=PathArgument, required=True)

  PackageParser = Commands.add_parser("package-runtime", help="Create a distributable package")
  PackageParser.add_argument("--configuration", required=True)
  PackageParser.add_argument("--output-dir", type=PathArgument, required=True)
  PackageParser.add_argument("--publish-dir", type=PathArgument, required=True)
  PackageParser.add_argument("--game-binary", type=PathArgument, required=True)
  PackageParser.add_argument("--engine-binary", type=PathArgument, required=True)
  PackageParser.add_argument("--game-name", required=True)
  PackageParser.add_argument("--eos-binary", type=PathArgument, required=True)
  PackageParser.add_argument("--online-resources-dir", type=PathArgument, required=True)
  PackageParser.add_argument("--convert-levels-script", type=PathArgument, required=True)
  PackageParser.add_argument("--bootstrap-binary", type=PathArgument, required=True)

  VerifyParser = Commands.add_parser("verify-runtime", help="Verify runtime artifacts")
  VerifyParser.add_argument("--output-dir", type=PathArgument, required=True)
  VerifyParser.add_argument("--game-name", required=True)
  VerifyParser.add_argument("--publish-dir", type=PathArgument)
  VerifyParser.add_argument("--configuration")
  return Parser


def Main() -> int:
  Arguments = CreateParser().parse_args()
  try:
    if Arguments.Command == "generate-plugins":
      PluginConfigurations = LoadPluginConfigurations(Arguments.project_dir)
      Changed = GeneratePluginsCmake(Arguments.project_dir, PluginConfigurations)
      RemoveDisabledExamplePluginArtifacts(Arguments.project_dir, PluginConfigurations)
      if Changed and Arguments.changed_exit_code:
        return 2
    elif Arguments.Command == "prepare-output":
      PrepareOutput(Arguments.output_dir)
    elif Arguments.Command == "stage-runtime":
      StageRuntime(
        Arguments.configuration,
        Arguments.engine_dir,
        Arguments.game_dir,
        Arguments.output_dir,
        Arguments.engine_binary,
        Arguments.game_name,
        Arguments.eos_binary,
        Arguments.convert_levels_script,
      )
    elif Arguments.Command == "package-runtime":
      PackageRuntime(
        Arguments.configuration,
        Arguments.output_dir,
        Arguments.publish_dir,
        Arguments.game_binary,
        Arguments.engine_binary,
        Arguments.game_name,
        Arguments.eos_binary,
        Arguments.online_resources_dir,
        Arguments.convert_levels_script,
        Arguments.bootstrap_binary,
      )
    elif Arguments.Command == "verify-runtime":
      if Arguments.configuration is not None and Arguments.configuration.casefold() == "editor":
        print("Editor configuration: runtime verification skipped.")
      else:
        VerifyRuntime(Arguments.output_dir, Arguments.game_name, Arguments.publish_dir)
  except (OSError, RuntimeError, ValueError, subprocess.CalledProcessError) as Error:
    print(Error, file=sys.stderr)
    return 1
  return 0
