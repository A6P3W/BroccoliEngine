"""Command-line interface for BroccoliEngine build processing."""

from __future__ import annotations

import argparse
import os
import shutil
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

CONFIGURATION_PRESETS = {
  "debug": ("Debug", "debug-local"),
  "editor": ("Editor", "editor-local"),
  "release": ("Release", "release-local"),
}
CONFIGURE_PRESET = "windows-x64-local"
CMAKE_CACHE_FILE = Path("build") / "windows-x64" / "CMakeCache.txt"


def PathArgument(Value: str) -> Path:
  return Path(Value).resolve()


def ConfigurationArgument(Value: str) -> str:
  Configuration = Value.casefold()
  if Configuration not in CONFIGURATION_PRESETS:
    raise argparse.ArgumentTypeError(f"Unsupported configuration: {Value}")
  return CONFIGURATION_PRESETS[Configuration][0]


def FindCmakeCommand() -> str:
  CmakeCommand = shutil.which("cmake")
  if CmakeCommand is not None:
    return CmakeCommand

  ProgramFilesX86 = os.environ.get("ProgramFiles(x86)")
  if ProgramFilesX86 is not None:
    Vswhere = Path(ProgramFilesX86) / "Microsoft Visual Studio" / "Installer" / "vswhere.exe"
    if Vswhere.is_file():
      Result = subprocess.run(
        [str(Vswhere), "-latest", "-property", "installationPath"],
        check=True,
        capture_output=True,
        text=True,
      )
      InstallationPath = Result.stdout.strip()
      if InstallationPath:
        VisualStudioCmake = (
          Path(InstallationPath)
          / "Common7"
          / "IDE"
          / "CommonExtensions"
          / "Microsoft"
          / "CMake"
          / "CMake"
          / "bin"
          / "cmake.exe"
        )
        if VisualStudioCmake.is_file():
          return str(VisualStudioCmake)

  raise RuntimeError("CMake was not found. Add cmake to PATH or install Visual Studio CMake tools.")


def ValidateUserPresets(ProjectDirectory: Path) -> None:
  PresetsPath = ProjectDirectory / "CMakeUserPresets.json"
  if PresetsPath.is_file() and "{YOUR_VCPKG_ROOT_DIRECTORY}" in PresetsPath.read_text(encoding="utf-8"):
    raise RuntimeError(
      "CMakeUserPresets.json still contains {YOUR_VCPKG_ROOT_DIRECTORY}. "
      "Please update VCPKG_ROOT in CMakeUserPresets.json to point to your vcpkg installation."
    )


def Regenerate(ProjectDirectory: Path, CmakeCommand: str | None = None) -> None:
  PluginConfigurations = LoadPluginConfigurations(ProjectDirectory)
  GeneratePluginsCmake(ProjectDirectory, PluginConfigurations)
  RemoveDisabledExamplePluginArtifacts(ProjectDirectory, PluginConfigurations)
  ValidateUserPresets(ProjectDirectory)
  subprocess.run(
    [CmakeCommand or FindCmakeCommand(), "--preset", CONFIGURE_PRESET],
    cwd=ProjectDirectory,
    check=True,
  )


def Build(ProjectDirectory: Path, Configuration: str, Reconfigure: bool) -> None:
  PluginConfigurations = LoadPluginConfigurations(ProjectDirectory)
  PluginsChanged = GeneratePluginsCmake(ProjectDirectory, PluginConfigurations)
  RemoveDisabledExamplePluginArtifacts(ProjectDirectory, PluginConfigurations)
  CmakeCommand = FindCmakeCommand()
  if Reconfigure or PluginsChanged or not (ProjectDirectory / CMAKE_CACHE_FILE).is_file():
    Regenerate(ProjectDirectory, CmakeCommand)

  BuildPreset = CONFIGURATION_PRESETS[Configuration.casefold()][1]
  subprocess.run(
    [CmakeCommand, "--build", "--preset", BuildPreset, "--target", f"BroccoliProjectBuild_{Configuration}"],
    cwd=ProjectDirectory,
    check=True,
  )


def CreateParser() -> argparse.ArgumentParser:
  Parser = argparse.ArgumentParser(description="BroccoliEngine build and packaging tools")
  Commands = Parser.add_subparsers(dest="Command", required=True)

  BuildParser = Commands.add_parser("build", help="Build a project configuration")
  BuildParser.add_argument("configuration", nargs="?", type=ConfigurationArgument)
  BuildParser.add_argument("--config", "-c", dest="config", type=ConfigurationArgument)
  BuildParser.add_argument("--reconfigure", action="store_true")
  BuildParser.add_argument("--project-dir", type=PathArgument, default=Path.cwd())

  RegenerateParser = Commands.add_parser("regenerate", help="Regenerate the CMake project model")
  RegenerateParser.add_argument("--project-dir", type=PathArgument, default=Path.cwd())

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
  PackageParser.add_argument("--required-plugin", action="append", default=[])

  VerifyParser = Commands.add_parser("verify-runtime", help="Verify runtime artifacts")
  VerifyParser.add_argument("--output-dir", type=PathArgument, required=True)
  VerifyParser.add_argument("--game-name", required=True)
  VerifyParser.add_argument("--publish-dir", type=PathArgument)
  VerifyParser.add_argument("--configuration")
  VerifyParser.add_argument("--required-plugin", action="append", default=[])
  return Parser


def Main() -> int:
  Arguments = CreateParser().parse_args()
  try:
    if Arguments.Command == "build":
      if Arguments.configuration is not None and Arguments.config is not None:
        raise ValueError("Specify the configuration either as a positional argument or with --config/-c.")
      Build(
        Arguments.project_dir,
        Arguments.config or Arguments.configuration or "Debug",
        Arguments.reconfigure,
      )
    elif Arguments.Command == "regenerate":
      Regenerate(Arguments.project_dir)
    elif Arguments.Command == "generate-plugins":
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
        [PluginName for PluginName in Arguments.required_plugin if PluginName],
      )
    elif Arguments.Command == "verify-runtime":
      if Arguments.configuration is not None and Arguments.configuration.casefold() == "editor":
        print("Editor configuration: runtime verification skipped.")
      else:
        VerifyRuntime(
          Arguments.output_dir,
          Arguments.game_name,
          Arguments.publish_dir,
          [PluginName for PluginName in Arguments.required_plugin if PluginName],
        )
  except (OSError, RuntimeError, ValueError, subprocess.CalledProcessError) as Error:
    print(Error, file=sys.stderr)
    return 1
  return 0
