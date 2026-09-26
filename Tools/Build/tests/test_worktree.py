from __future__ import annotations

import json
import subprocess
from pathlib import Path

import pytest
from broccoli_build.cli import CreateParser
from broccoli_build.worktree import SetupWorktree


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


def AddWorktree(SourceDirectory: Path, TargetDirectory: Path, Branch: str) -> None:
  RunGit(SourceDirectory, "worktree", "add", "-b", Branch, str(TargetDirectory), "HEAD")


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


def TestParserAcceptsWorktreeSetupPath(tmp_path: Path) -> None:
  TargetDirectory = tmp_path / "target"
  Arguments = CreateParser().parse_args(["worktree", "setup", str(TargetDirectory)])
  assert Arguments.Command == "worktree"
  assert Arguments.worktree_command == "setup"
  assert Arguments.path == TargetDirectory.resolve()


def TestParserRejectsRemovedWorktreeCreateCommand() -> None:
  with pytest.raises(SystemExit):
    CreateParser().parse_args(["worktree", "create", "feature/example"])


def TestSetupWorktreeCopiesUserPresetsAndIgnoresBuildTree(tmp_path: Path) -> None:
  SourceDirectory = tmp_path / "source"
  TargetDirectory = tmp_path / "target"
  InitializeRepository(SourceDirectory)
  AddWorktree(SourceDirectory, TargetDirectory, "test/copied-setup")
  (SourceDirectory / "CMakeUserPresets.json").write_text(
    '{"version": 6}\n', encoding="utf-8", newline="\n"
  )
  BuildDirectory = SourceDirectory / "build" / "windows-x64"
  BuildDirectory.mkdir(parents=True)
  (BuildDirectory / "CMakeCache.txt").write_text("cache\n", encoding="utf-8", newline="\n")

  SetupWorktree(SourceDirectory, TargetDirectory)

  assert RunGit(TargetDirectory, "branch", "--show-current") == "test/copied-setup"
  assert (TargetDirectory / "CMakeUserPresets.json").is_file()
  assert not (TargetDirectory / "build").exists()


def TestSetupWorktreeSucceedsWithoutLocalSetup(tmp_path: Path) -> None:
  SourceDirectory = tmp_path / "source"
  TargetDirectory = tmp_path / "target"
  InitializeRepository(SourceDirectory)
  AddWorktree(SourceDirectory, TargetDirectory, "test/no-local-setup")
  SetupWorktree(SourceDirectory, TargetDirectory)
  assert not (TargetDirectory / "CMakeUserPresets.json").exists()
  assert not (TargetDirectory / "build" / "windows-x64").exists()


def TestSetupWorktreeRejectsMissingTarget(tmp_path: Path) -> None:
  SourceDirectory = tmp_path / "source"
  InitializeRepository(SourceDirectory)
  with pytest.raises(ValueError, match="does not exist"):
    SetupWorktree(SourceDirectory, tmp_path / "missing")


def TestSetupWorktreeRejectsSourceAsTarget(tmp_path: Path) -> None:
  SourceDirectory = tmp_path / "source"
  InitializeRepository(SourceDirectory)
  with pytest.raises(ValueError, match="must be different"):
    SetupWorktree(SourceDirectory, SourceDirectory)


def TestSetupWorktreeRejectsNonWorktreeTarget(tmp_path: Path) -> None:
  SourceDirectory = tmp_path / "source"
  TargetDirectory = tmp_path / "target"
  InitializeRepository(SourceDirectory)
  TargetDirectory.mkdir()
  with pytest.raises(ValueError, match="not a Git worktree root"):
    SetupWorktree(SourceDirectory, TargetDirectory)


def TestSetupWorktreeRejectsDifferentRepository(tmp_path: Path) -> None:
  SourceDirectory = tmp_path / "source"
  TargetDirectory = tmp_path / "target"
  InitializeRepository(SourceDirectory)
  InitializeRepository(TargetDirectory)
  with pytest.raises(ValueError, match="same repository"):
    SetupWorktree(SourceDirectory, TargetDirectory)


def TestGameWorktreeClonesEngineAtGitlinkCommit(tmp_path: Path) -> None:
  EngineOrigin = tmp_path / "engine-origin"
  InitializeRepository(EngineOrigin)
  EngineCommit = RunGit(EngineOrigin, "rev-parse", "HEAD")
  GameDirectory = tmp_path / "game"
  InitializeRepository(GameDirectory)
  AddEngineSubmodule(GameDirectory, EngineOrigin)
  TargetDirectory = tmp_path / "game-worktree"
  AddWorktree(GameDirectory, TargetDirectory, "test/game-worktree")

  SetupWorktree(GameDirectory, TargetDirectory)

  TargetEngine = TargetDirectory / "BroccoliEngine"
  assert RunGit(TargetEngine, "rev-parse", "HEAD") == EngineCommit
  assert (TargetEngine / ".git").is_dir()


def TestGameWorktreeRejectsDirtyEngineBeforeSetup(tmp_path: Path) -> None:
  EngineOrigin = tmp_path / "engine-origin"
  InitializeRepository(EngineOrigin)
  GameDirectory = tmp_path / "game"
  InitializeRepository(GameDirectory)
  AddEngineSubmodule(GameDirectory, EngineOrigin)
  TargetDirectory = tmp_path / "game-worktree"
  AddWorktree(GameDirectory, TargetDirectory, "test/dirty-engine")
  (GameDirectory / "BroccoliEngine" / "tracked.txt").write_text(
    "dirty\n", encoding="utf-8", newline="\n"
  )

  with pytest.raises(RuntimeError, match="uncommitted changes"):
    SetupWorktree(GameDirectory, TargetDirectory)

  assert not (TargetDirectory / "BroccoliEngine" / ".git").exists()
