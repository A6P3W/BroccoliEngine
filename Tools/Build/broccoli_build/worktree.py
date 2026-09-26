"""Git worktree creation and local development setup transfer."""

from __future__ import annotations

import json
import shutil
import subprocess
from pathlib import Path

PROJECT_SETTINGS_FILE_NAME = ".broccoli-project.json"
ENGINE_DIRECTORY_NAME = "BroccoliEngine"
USER_PRESETS_FILE_NAME = "CMakeUserPresets.json"
BUILD_DIRECTORY = Path("build") / "windows-x64"
CMAKE_CACHE_FILE_NAME = "CMakeCache.txt"
CMAKE_FILES_DIRECTORY_NAME = "CMakeFiles"


def ResolveDefaultTargetDirectory(SourceDirectory: Path, Branch: str) -> Path:
  BranchDirectory = Branch.replace("/", "-").replace("\\", "-")
  return SourceDirectory.parent / f"{SourceDirectory.name}-worktrees" / BranchDirectory


def LoadEngineDirectory(SourceDirectory: Path) -> Path | None:
  SettingsPath = SourceDirectory / PROJECT_SETTINGS_FILE_NAME
  if not SettingsPath.is_file():
    return None
  try:
    Settings = json.loads(SettingsPath.read_text(encoding="utf-8"))
  except (OSError, json.JSONDecodeError) as Error:
    raise ValueError(f"Could not read project settings '{SettingsPath}': {Error}") from Error
  Engine = Settings.get("engine")
  if not isinstance(Engine, dict) or Engine.get("path") != ENGINE_DIRECTORY_NAME:
    return None
  return SourceDirectory / ENGINE_DIRECTORY_NAME


def RequireCleanEngine(EngineDirectory: Path) -> None:
  Result = subprocess.run(
    ["git", "-C", str(EngineDirectory), "status", "--porcelain"],
    check=True,
    capture_output=True,
    text=True,
  )
  if Result.stdout.strip():
    raise RuntimeError(f"BroccoliEngine checkout has uncommitted changes: {EngineDirectory}")


def CreateEngineCheckout(TargetDirectory: Path, SourceEngineDirectory: Path) -> None:
  TargetEngineDirectory = TargetDirectory / ENGINE_DIRECTORY_NAME
  GitlinkResult = subprocess.run(
    ["git", "-C", str(TargetDirectory), "rev-parse", f"HEAD:{ENGINE_DIRECTORY_NAME}"],
    check=True,
    capture_output=True,
    text=True,
  )
  GitlinkCommit = GitlinkResult.stdout.strip()
  subprocess.run(
    [
      "git",
      "clone",
      "--local",
      "--no-checkout",
      str(SourceEngineDirectory),
      str(TargetEngineDirectory),
    ],
    check=True,
  )
  subprocess.run(
    ["git", "-C", str(TargetEngineDirectory), "checkout", "--detach", GitlinkCommit],
    check=True,
  )


def CopyLocalSetup(SourceDirectory: Path, TargetDirectory: Path) -> None:
  SourcePresets = SourceDirectory / USER_PRESETS_FILE_NAME
  if SourcePresets.is_file():
    shutil.copy2(SourcePresets, TargetDirectory / USER_PRESETS_FILE_NAME)
  SourceBuildDirectory = SourceDirectory / BUILD_DIRECTORY
  if SourceBuildDirectory.is_dir():
    TargetBuildDirectory = TargetDirectory / BUILD_DIRECTORY
    shutil.copytree(SourceBuildDirectory, TargetBuildDirectory)
    (TargetBuildDirectory / CMAKE_CACHE_FILE_NAME).unlink(missing_ok=True)
    shutil.rmtree(TargetBuildDirectory / CMAKE_FILES_DIRECTORY_NAME, ignore_errors=True)


def CreateWorktree(SourceDirectory: Path, TargetDirectory: Path, Branch: str) -> None:
  SourceDirectory = SourceDirectory.resolve()
  TargetDirectory = TargetDirectory.resolve()
  SourceEngineDirectory = LoadEngineDirectory(SourceDirectory)
  if SourceEngineDirectory is not None:
    RequireCleanEngine(SourceEngineDirectory)
  subprocess.run(
    ["git", "worktree", "add", "-b", Branch, str(TargetDirectory), "HEAD"],
    cwd=SourceDirectory,
    check=True,
  )
  try:
    if SourceEngineDirectory is not None:
      CreateEngineCheckout(TargetDirectory, SourceEngineDirectory)
    CopyLocalSetup(SourceDirectory, TargetDirectory)
  except (OSError, RuntimeError, ValueError, subprocess.CalledProcessError) as Error:
    raise RuntimeError(
      f"Worktree was created at '{TargetDirectory}', but setup copying failed: {Error}"
    ) from Error
