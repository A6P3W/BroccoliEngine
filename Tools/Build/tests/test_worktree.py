from __future__ import annotations

import json
import subprocess
from pathlib import Path

import pytest
from broccoli_build.cli import CreateParser
from broccoli_build.worktree import CreateWorktree, ResolveDefaultTargetDirectory


def RunGit(Directory: Path, *Arguments: str) -> str:
  Result = subprocess.run(
    ["git", *Arguments], cwd=Directory, check=True, capture_output=True, text=True
  )
  return Result.stdout.strip()


def InitializeRepository(Directory: Path) -> None:
  Directory.mkdir(parents=True)
  RunGit(Directory, "init")
  RunGit(Directory, "config", "user.email", "worktree-test@example.com")
  RunGit(Directory, "config", "user.name", "Worktree Test")
  (Directory / "tracked.txt").write_text("tracked\n", encoding="utf-8", newline="\n")
  RunGit(Directory, "add", "tracked.txt")
  RunGit(Directory, "commit", "-m", "test: initialize repository")


def AddEngineSubmodule(GameDirectory: Path, EngineOrigin: Path) -> None:
  RunGit(
    GameDirectory,
    "-c",
    "protocol.file.allow=always",
    "submodule",
    "add",
    str(EngineOrigin),
    "BroccoliEngine",
  )
  (GameDirectory / ".broccoli-project.json").write_text(
    json.dumps({"engine": {"path": "BroccoliEngine"}}) + "\n",
    encoding="utf-8",
    newline="\n",
  )
  RunGit(GameDirectory, "add", ".broccoli-project.json", ".gitmodules", "BroccoliEngine")
  RunGit(GameDirectory, "commit", "-m", "test: add engine submodule")


def TestResolveDefaultTargetDirectoryReplacesBranchSeparators(tmp_path: Path) -> None:
  SourceDirectory = tmp_path / "BroccoliEngine"
  assert ResolveDefaultTargetDirectory(SourceDirectory, "feature/worktree") == (
    tmp_path / "BroccoliEngine-worktrees" / "feature-worktree"
  )


def TestParserAcceptsWorktreeCreatePath(tmp_path: Path) -> None:
  TargetDirectory = tmp_path / "target"
  Arguments = CreateParser().parse_args(
    ["worktree", "create", "feature/example", "--path", str(TargetDirectory)]
  )
  assert Arguments.Command == "worktree"
  assert Arguments.worktree_command == "create"
  assert Arguments.branch == "feature/example"
  assert Arguments.path == TargetDirectory.resolve()


def TestCreateWorktreeCopiesCompleteLocalSetup(tmp_path: Path) -> None:
  SourceDirectory = tmp_path / "source"
  TargetDirectory = tmp_path / "target"
  InitializeRepository(SourceDirectory)
  (SourceDirectory / "CMakeUserPresets.json").write_text(
    '{"version": 6}\n', encoding="utf-8", newline="\n"
  )
  BuildDirectory = SourceDirectory / "build" / "windows-x64"
  (BuildDirectory / "vcpkg_installed").mkdir(parents=True)
  (BuildDirectory / "CMakeCache.txt").write_text("cache\n", encoding="utf-8", newline="\n")
  (BuildDirectory / "project.obj").write_bytes(b"object")
  (BuildDirectory / "vcpkg_installed" / "package.txt").write_text(
    "package\n", encoding="utf-8", newline="\n"
  )

  CreateWorktree(SourceDirectory, TargetDirectory, "feature/copied-setup")

  assert RunGit(TargetDirectory, "branch", "--show-current") == "feature/copied-setup"
  assert (TargetDirectory / "CMakeUserPresets.json").is_file()
  assert (TargetDirectory / "build" / "windows-x64" / "CMakeCache.txt").is_file()
  assert (TargetDirectory / "build" / "windows-x64" / "project.obj").read_bytes() == b"object"
  assert (TargetDirectory / "build" / "windows-x64" / "vcpkg_installed" / "package.txt").is_file()


def TestCreateWorktreeSucceedsWithoutLocalSetup(tmp_path: Path) -> None:
  SourceDirectory = tmp_path / "source"
  TargetDirectory = tmp_path / "target"
  InitializeRepository(SourceDirectory)
  CreateWorktree(SourceDirectory, TargetDirectory, "feature/no-local-setup")
  assert TargetDirectory.is_dir()
  assert not (TargetDirectory / "CMakeUserPresets.json").exists()
  assert not (TargetDirectory / "build" / "windows-x64").exists()


def TestGitFailureDoesNotCopyLocalSetup(tmp_path: Path) -> None:
  SourceDirectory = tmp_path / "source"
  TargetDirectory = tmp_path / "target"
  InitializeRepository(SourceDirectory)
  (SourceDirectory / "CMakeUserPresets.json").write_text(
    "presets\n", encoding="utf-8", newline="\n"
  )
  with pytest.raises(subprocess.CalledProcessError):
    CreateWorktree(SourceDirectory, TargetDirectory, "invalid branch name")
  assert not TargetDirectory.exists()


def TestGameWorktreeClonesEngineAtGitlinkCommit(tmp_path: Path) -> None:
  EngineOrigin = tmp_path / "engine-origin"
  InitializeRepository(EngineOrigin)
  EngineCommit = RunGit(EngineOrigin, "rev-parse", "HEAD")
  GameDirectory = tmp_path / "game"
  InitializeRepository(GameDirectory)
  AddEngineSubmodule(GameDirectory, EngineOrigin)
  TargetDirectory = tmp_path / "game-worktree"

  CreateWorktree(GameDirectory, TargetDirectory, "feature/game-worktree")

  TargetEngine = TargetDirectory / "BroccoliEngine"
  assert RunGit(TargetEngine, "rev-parse", "HEAD") == EngineCommit
  assert (TargetEngine / ".git").is_dir()


def TestGameWorktreeRejectsDirtyEngineBeforeCreation(tmp_path: Path) -> None:
  EngineOrigin = tmp_path / "engine-origin"
  InitializeRepository(EngineOrigin)
  GameDirectory = tmp_path / "game"
  InitializeRepository(GameDirectory)
  AddEngineSubmodule(GameDirectory, EngineOrigin)
  (GameDirectory / "BroccoliEngine" / "tracked.txt").write_text(
    "dirty\n", encoding="utf-8", newline="\n"
  )
  TargetDirectory = tmp_path / "game-worktree"
  with pytest.raises(RuntimeError, match="uncommitted changes"):
    CreateWorktree(GameDirectory, TargetDirectory, "feature/dirty-engine")
  assert not TargetDirectory.exists()
