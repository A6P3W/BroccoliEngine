from __future__ import annotations

import json
import shutil
import subprocess
import sys
from pathlib import Path

import pytest

REPOSITORY_ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(REPOSITORY_ROOT))

import SetupProject
from SetupProject import SetupError, SetupState


@pytest.fixture(name="TmpPath")
def CreateTmpPath(tmp_path: Path) -> Path:
  return tmp_path


@pytest.fixture(name="TemplateRoot")
def CreateTemplateRoot() -> Path:
  return Path(SetupProject.__file__).resolve().parent / "ProjectTemplates" / "Default"


def RunGit(WorkingDirectory: Path, *Arguments: str) -> str:
  Result = subprocess.run(
    ["git", *Arguments],
    cwd=WorkingDirectory,
    check=True,
    capture_output=True,
    text=True,
    encoding="utf-8",
  )
  return Result.stdout.strip()


def CreateEngineRepository(
  RepositoryRoot: Path,
  TemplateRoot: Path,
) -> tuple[str, list[str]]:
  shutil.copytree(TemplateRoot, RepositoryRoot / "ProjectTemplates" / "Default")
  (RepositoryRoot / "Engine").mkdir()
  (RepositoryRoot / "Engine" / "engine.txt").write_text(
    "engine\n",
    encoding="utf-8",
    newline="\n",
  )
  (RepositoryRoot / "SetupProject.py").write_text(
    "# fixture\n",
    encoding="utf-8",
    newline="\n",
  )

  RunGit(RepositoryRoot, "init")
  RunGit(RepositoryRoot, "config", "user.name", "Broccoli Test")
  RunGit(RepositoryRoot, "config", "user.email", "broccoli@example.invalid")
  RunGit(RepositoryRoot, "add", ".")
  RunGit(RepositoryRoot, "commit", "-m", "fixture")
  RunGit(
    RepositoryRoot,
    "remote",
    "add",
    "origin",
    str(RepositoryRoot.parent / "Engine Origin"),
  )

  HeadCommit = RunGit(RepositoryRoot, "rev-parse", "HEAD")
  OriginalNames = sorted(PathValue.name for PathValue in RepositoryRoot.iterdir())
  return HeadCommit, OriginalNames


def TestRenderTemplatesCreatesBasicGameplay(
  TmpPath: Path,
  TemplateRoot: Path,
) -> None:
  OutputRoot = TmpPath / "TestOutput"

  SetupProject.RenderTemplates(TemplateRoot, OutputRoot, "TestGame")

  assert (OutputRoot / "TestGame" / "Source" / "Entry.cpp").is_file()
  assert (
    OutputRoot / "TestGame" / "Source" / "BasicGameplay" / "BasicGameplayGameMode.cpp"
  ).is_file()
  assert (OutputRoot / "TestGame" / "Resources" / "BasicGameplay.BLevel.json").is_file()
  assert (OutputRoot / ".gitignore").is_file()
  UserPresetsText = (OutputRoot / "CMakeUserPresets.json").read_text(encoding="utf-8")
  UserPresetsData = json.loads(UserPresetsText)
  UserConfigurePresets = {
    Preset["name"]: Preset for Preset in UserPresetsData.get("configurePresets", [])
  }
  assert "windows-x64-local" in UserConfigurePresets
  assert (
    UserConfigurePresets["windows-x64-local"]["environment"]["VCPKG_ROOT"]
    == "{YOUR_VCPKG_ROOT_DIRECTORY}"
  )
  assert "CMAKE_TOOLCHAIN_FILE" not in UserPresetsText

  assert not (OutputRoot / "Game").exists()
  SharedPresetsText = (OutputRoot / "CMakePresets.json").read_text(encoding="utf-8")
  SharedPresetsData = json.loads(SharedPresetsText)
  SharedConfigurePresets = {
    Preset["name"]: Preset for Preset in SharedPresetsData.get("configurePresets", [])
  }
  assert "windows-x64" in SharedConfigurePresets
  assert (
    SharedConfigurePresets["windows-x64"]["cacheVariables"]["CMAKE_TOOLCHAIN_FILE"]
    == "$env{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
  )

  RootCMake = (OutputRoot / "CMakeLists.txt").read_text(encoding="utf-8")
  assert "add_subdirectory(TestGame)" in RootCMake
  assert 'BROCCOLI_GAME_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/TestGame"' in RootCMake
  assert "add_subdirectory(BroccoliEngine/Engine)" in RootCMake

  AllText = "\n".join(
    PathValue.read_text(encoding="utf-8")
    for PathValue in OutputRoot.rglob("*")
    if PathValue.is_file()
  )
  assert "{PROJECT_NAME}" not in AllText
  assert "ABasicGameplayGameMode" in AllText
  assert "TestGame-game.exe" in AllText
  assert "Publish\\%TARGET%\\TestGame.exe" in AllText


def TestRenderTemplatesRefusesOverwrite(
  TmpPath: Path,
  TemplateRoot: Path,
) -> None:
  OutputRoot = TmpPath / "TestOutput"
  SetupProject.RenderTemplates(TemplateRoot, OutputRoot, "TestGame")

  with pytest.raises(SetupError, match="Refusing to overwrite"):
    SetupProject.RenderTemplates(TemplateRoot, OutputRoot, "TestGame")


@pytest.mark.parametrize("ProjectName", ["", "1Game", "Bad-Game", "ゲーム", "BroccoliEngine"])
def TestInvalidProjectNameIsRejected(ProjectName: str) -> None:
  with pytest.raises(SetupError, match="Project name"):
    SetupProject.ValidateProjectName(ProjectName)


def TestInPlaceCreatesVerifiedSubmodule(
  TmpPath: Path,
  TemplateRoot: Path,
) -> None:
  RepositoryRoot = TmpPath / "EngineRepository"
  RepositoryRoot.mkdir()
  HeadCommit, _ = CreateEngineRepository(RepositoryRoot, TemplateRoot)

  SetupProject.ReconfigureInPlace(RepositoryRoot, "TestGame")

  StageFields = RunGit(
    RepositoryRoot,
    "ls-files",
    "--stage",
    "BroccoliEngine",
  ).split()
  assert StageFields[0] == "160000"
  assert StageFields[1] == HeadCommit
  assert (RepositoryRoot / "BroccoliEngine" / ".git").is_file()
  assert (RepositoryRoot / "TestGame" / "Source" / "Entry.cpp").is_file()
  assert (RepositoryRoot / ".broccoli-project.json").is_file()
  ProjectSettings = json.loads(
    (RepositoryRoot / ".broccoli-project.json").read_text(encoding="utf-8")
  )
  assert ProjectSettings["plugins"] == [
    {"name": "ExamplePlugin", "configurations": ["Debug", "Editor", "Release"]}
  ]
  assert HeadCommit in RunGit(
    RepositoryRoot,
    "submodule",
    "status",
    "BroccoliEngine",
  )


def TestInPlaceRollsBackAfterSubmoduleRegistrationFailure(
  TmpPath: Path,
  TemplateRoot: Path,
) -> None:
  RepositoryRoot = TmpPath / "EngineRepository"
  RepositoryRoot.mkdir()
  _, OriginalNames = CreateEngineRepository(RepositoryRoot, TemplateRoot)

  def FailAfterRegistration(State: SetupState) -> None:
    if State == SetupState.SUBMODULE_REGISTERED:
      raise RuntimeError("injected failure")

  with pytest.raises(SetupError, match="SUBMODULE_REGISTERED"):
    SetupProject.ReconfigureInPlace(
      RepositoryRoot,
      "TestGame",
      Hook=FailAfterRegistration,
    )

  assert sorted(PathValue.name for PathValue in RepositoryRoot.iterdir()) == OriginalNames
  assert not (RepositoryRoot / "BroccoliEngine").exists()
  assert not (RepositoryRoot / "TestGame").exists()
  assert RunGit(RepositoryRoot, "status", "--porcelain") == ""


def TestInPlaceRollsBackWhenInitialMoveFails(
  TmpPath: Path,
  TemplateRoot: Path,
) -> None:
  RepositoryRoot = TmpPath / "EngineRepository"
  RepositoryRoot.mkdir()
  HeadCommit, _ = CreateEngineRepository(RepositoryRoot, TemplateRoot)

  def FailInitialMove(Source: str, Destination: str) -> str:
    del Source, Destination
    raise OSError("injected initial move failure")

  with pytest.MonkeyPatch.context() as MonkeyPatch:
    MonkeyPatch.setattr(SetupProject.shutil, "move", FailInitialMove)
    with pytest.raises(SetupError, match="TEMPLATES_PREPARED"):
      SetupProject.ReconfigureInPlace(RepositoryRoot, "TestGame")

  assert (RepositoryRoot / ".git").is_dir()
  assert RunGit(RepositoryRoot, "rev-parse", "HEAD") == HeadCommit
  assert not (RepositoryRoot / "BroccoliEngine").exists()


def TestInPlaceRollsBackWhenCompletionHookFails(
  TmpPath: Path,
  TemplateRoot: Path,
) -> None:

  RepositoryRoot = TmpPath / "EngineRepository"
  RepositoryRoot.mkdir()
  _, OriginalNames = CreateEngineRepository(RepositoryRoot, TemplateRoot)

  def FailAfterCompletion(State: SetupState) -> None:
    if State == SetupState.COMPLETED:
      raise RuntimeError("injected completion failure")

  with pytest.raises(SetupError, match="COMPLETED"):
    SetupProject.ReconfigureInPlace(
      RepositoryRoot,
      "TestGame",
      Hook=FailAfterCompletion,
    )

  assert sorted(PathValue.name for PathValue in RepositoryRoot.iterdir()) == OriginalNames
  assert not (RepositoryRoot / ".broccoli-project.json").exists()
  assert RunGit(RepositoryRoot, "status", "--porcelain") == ""


def TestResetProjectRestoresEngineRepository(
  TmpPath: Path,
  TemplateRoot: Path,
) -> None:
  RepositoryRoot = TmpPath / "EngineRepository"
  RepositoryRoot.mkdir()
  HeadCommit, OriginalNames = CreateEngineRepository(RepositoryRoot, TemplateRoot)

  SetupProject.ReconfigureInPlace(RepositoryRoot, "TestGame")
  (RepositoryRoot / "TestGame" / "save.dat").write_text(
    "game data\n",
    encoding="utf-8",
    newline="\n",
  )
  BackupRoot = TmpPath / "GameWorkspaceBackup"

  Result = SetupProject.ResetProject(RepositoryRoot, BackupRoot)

  assert Result == BackupRoot
  assert sorted(PathValue.name for PathValue in RepositoryRoot.iterdir()) == OriginalNames
  assert (RepositoryRoot / ".git").is_dir()
  assert not (RepositoryRoot / "BroccoliEngine").exists()
  assert not (RepositoryRoot / ".broccoli-project.json").exists()
  assert (BackupRoot / "TestGame" / "save.dat").read_text(encoding="utf-8") == ("game data\n")
  assert RunGit(RepositoryRoot, "rev-parse", "HEAD") == HeadCommit
  assert RunGit(RepositoryRoot, "status", "--porcelain") == ""


def TestResetProjectRejectsDirectoryWithoutMarker(TmpPath: Path) -> None:
  ProjectRoot = TmpPath / "NotGenerated"
  ProjectRoot.mkdir()

  with pytest.raises(SetupError, match="not a generated Broccoli project"):
    SetupProject.ResetProject(ProjectRoot, TmpPath / "Backup")


def TestResetProjectRollsBackWhenEngineMoveFails(
  TmpPath: Path,
  TemplateRoot: Path,
) -> None:
  RepositoryRoot = TmpPath / "EngineRepository"
  RepositoryRoot.mkdir()
  _, _ = CreateEngineRepository(RepositoryRoot, TemplateRoot)
  SetupProject.ReconfigureInPlace(RepositoryRoot, "TestGame")

  EngineRoot = RepositoryRoot / "BroccoliEngine"
  OriginalMove = SetupProject.shutil.move

  def FailOnEngineMove(Source: str, Destination: str) -> str:
    if Path(Source).parent == EngineRoot:
      raise OSError("injected reset move failure")
    return OriginalMove(Source, Destination)

  with pytest.MonkeyPatch.context() as MonkeyPatch:
    MonkeyPatch.setattr(SetupProject.shutil, "move", FailOnEngineMove)
    with pytest.raises(SetupError, match="Project reset failed"):
      SetupProject.ResetProject(RepositoryRoot, TmpPath / "Backup")

  assert (RepositoryRoot / ".broccoli-project.json").is_file()
  assert (RepositoryRoot / "BroccoliEngine" / ".git").is_file()
  assert (RepositoryRoot / "TestGame" / "Source" / "Entry.cpp").is_file()
  assert not (TmpPath / "Backup").exists()
  assert RunGit(
    RepositoryRoot,
    "ls-files",
    "--stage",
    "BroccoliEngine",
  ).startswith("160000 ")


def TestCliRequestsManualUserPresetEdit(TmpPath: Path) -> None:
  OutputRoot = TmpPath / "CliOutput"
  Result = subprocess.run(
    [
      sys.executable,
      str(REPOSITORY_ROOT / "SetupProject.py"),
      "--name",
      "TestGame",
      "--output",
      str(OutputRoot),
    ],
    check=False,
    capture_output=True,
    text=True,
    encoding="utf-8",
    errors="replace",
  )

  assert Result.returncode == 0, Result.stderr
  assert "CMakeUserPresets.json" in Result.stdout
  assert (OutputRoot / "CMakeUserPresets.json").is_file()
