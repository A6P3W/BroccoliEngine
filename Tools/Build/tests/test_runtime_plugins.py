import json
import subprocess
from pathlib import Path

import pytest
from broccoli_build import cli
from broccoli_build.package_runtime import PackageRuntime
from broccoli_build.runtime_plugins import (
  GenerateRuntimePlugins,
  LoadRuntimePlugins,
  StagePlugins,
  VerifyPlugins,
)
from broccoli_build.stage_runtime import StageRuntime
from broccoli_build.verify_runtime import VerifyRuntime


@pytest.mark.parametrize("Names", [[], ["ExamplePlugin"], ["ExamplePlugin", "EOSPlugin"]])
def TestStageAndPackageSinglePluginCopies(tmp_path: Path, Names: list[str]) -> None:
  Output = tmp_path / "Bin"
  Output.mkdir()
  Engine = tmp_path / "Engine"
  Game = tmp_path / "Game"
  for Directory in [Engine, Game]:
    (Directory / "Resources").mkdir(parents=True)
    (Directory / "Resources" / "asset.txt").write_text("asset", encoding="utf-8")
  Converter = tmp_path / "ConvertLevels.py"
  Converter.write_text("", encoding="utf-8")
  for Name in [
    "BroccoliEngine.dll",
    "Game-game.exe",
    "BroccoliBootstrap.exe",
    *[f"{Name}.dll" for Name in Names],
  ]:
    (Output / Name).write_bytes(b"binary")
  Dependency = tmp_path / "SDK.dll"
  Dependency.write_bytes(b"dependency")
  Dependencies = [Dependency] if "EOSPlugin" in Names else []
  GenerateRuntimePlugins(Output, Names)
  StageRuntime(
    "Debug",
    Engine,
    Game,
    Output,
    Output / "BroccoliEngine.dll",
    "Game",
    Converter,
    [],
    Dependencies,
  )
  Publish = tmp_path / "Publish"
  PackageRuntime(
    "Debug",
    Output,
    Publish,
    Output / "Game-game.exe",
    Output / "BroccoliEngine.dll",
    "Game",
    Converter,
    Output / "BroccoliBootstrap.exe",
    Dependencies,
  )
  VerifyRuntime(Output, "Game", Publish, Dependencies)
  assert LoadRuntimePlugins(Publish / "Binaries") == Names
  assert not (Publish / "Binaries" / "Plugins").exists()
  assert len(list(Publish.rglob("SDK.dll"))) == (1 if Dependencies else 0)
  for Name in Names:
    assert len(list(Publish.rglob(f"{Name}.dll"))) == 1
  # Rebuild only a plugin, and build delivery must refresh the package.
  if Names:
    Plugin = Output / f"{Names[0]}.dll"
    Plugin.write_bytes(b"updated")
    PackageRuntime(
      "Debug",
      Output,
      Publish,
      Output / "Game-game.exe",
      Output / "BroccoliEngine.dll",
      "Game",
      Converter,
      Output / "BroccoliBootstrap.exe",
      Dependencies,
    )
    assert (Publish / "Binaries" / Plugin.name).read_bytes() == b"updated"


def TestDisablingPluginsRemovesOnlyManagedArtifacts(tmp_path: Path) -> None:
  Dependency = tmp_path / "Source" / "SDK.dll"
  Dependency.parent.mkdir()
  Dependency.write_bytes(b"sdk")
  Output = tmp_path / "Bin"
  GenerateRuntimePlugins(Output, ["EOSPlugin"])
  (Output / "EOSPlugin.dll").write_bytes(b"plugin")
  (Output / "User.dll").write_bytes(b"user")
  StagePlugins(Output, [Dependency])
  GenerateRuntimePlugins(Output, [])
  StagePlugins(Output, [])
  assert not (Output / "EOSPlugin.dll").exists()
  assert not (Output / "SDK.dll").exists()
  assert (Output / "User.dll").read_bytes() == b"user"


@pytest.mark.parametrize(
  "Config",
  [
    None,
    [],
    {},
    {"schema_version": True, "required_plugins": []},
    {"schema_version": 2, "required_plugins": []},
    {"schema_version": 1, "required_plugins": ["../Bad"]},
    {"schema_version": 1, "required_plugins": ["Bad", "Bad"]},
    {"schema_version": 1, "required_plugins": [""]},
  ],
)
def TestRejectInvalidRuntimeConfiguration(tmp_path: Path, Config: object) -> None:
  (tmp_path / "plugins.json").write_text(json.dumps(Config), encoding="utf-8")
  with pytest.raises(ValueError):
    LoadRuntimePlugins(tmp_path)


def TestRequiredPluginAndDependencyMissingFail(tmp_path: Path) -> None:
  GenerateRuntimePlugins(tmp_path, ["ExamplePlugin"])
  with pytest.raises(ValueError, match="plugin DLL missing"):
    VerifyPlugins(tmp_path, [])
  (tmp_path / "ExamplePlugin.dll").write_bytes(b"plugin")
  with pytest.raises(ValueError, match="dependency missing"):
    VerifyPlugins(tmp_path, [Path("SDK.dll")])


def TestBuildFailureNeverStagesOrRecordsSuccess(
  tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
  Cache = tmp_path / cli.CMAKE_CACHE_FILE
  Cache.parent.mkdir(parents=True)
  Cache.write_text("", encoding="utf-8")
  monkeypatch.setattr(cli, "FindCmakeCommand", lambda: "cmake")

  def Fail(*Arguments, **Keywords):
    raise subprocess.CalledProcessError(1, "cmake")

  monkeypatch.setattr(cli.subprocess, "run", Fail)
  monkeypatch.setattr(
    cli, "StageBuiltProject", lambda *Arguments: pytest.fail("Failed build staged artifacts")
  )
  with pytest.raises(subprocess.CalledProcessError):
    cli.Build(tmp_path, "Debug", False)
  assert not (tmp_path / cli.LATEST_BUILD_CONFIGURATION_FILE).exists()


def TestBuildStagesAfterAllTargetsSucceed(tmp_path: Path, monkeypatch: pytest.MonkeyPatch) -> None:
  Cache = tmp_path / cli.CMAKE_CACHE_FILE
  Cache.parent.mkdir(parents=True)
  Cache.write_text("", encoding="utf-8")
  Events = []
  monkeypatch.setattr(cli, "FindCmakeCommand", lambda: "cmake")
  monkeypatch.setattr(cli.subprocess, "run", lambda *Arguments, **Keywords: Events.append("build"))
  monkeypatch.setattr(cli, "StageBuiltProject", lambda *Arguments: Events.append("stage"))
  cli.Build(tmp_path, "Debug", False)
  assert Events == ["build", "stage"]


def TestEditorStagesPluginsWithoutPackagingResources(tmp_path: Path) -> None:
  Output = tmp_path / "Bin"
  GenerateRuntimePlugins(Output, [])
  (Output / "BroccoliEngine.dll").write_bytes(b"engine")
  StageRuntime(
    "Editor",
    tmp_path,
    tmp_path,
    Output,
    Output / "BroccoliEngine.dll",
    "Game",
    tmp_path / "converter.py",
  )
  VerifyRuntime(Output, "Game", Configuration="Editor")
  assert not (Output / "Resources").exists()


def TestCoreHasNoEOSDependency() -> None:
  Root = Path(__file__).resolve().parents[3]
  for Source in (Root / "Engine" / "EngineCore").rglob("*"):
    if Source.suffix in (".h", ".cpp"):
      Text = Source.read_text(encoding="utf-8")
      assert "#include <eos_" not in Text
      assert "ProductUserId" not in Text
  assert "EOSSDK" not in (Root / "Engine" / "CMakeLists.txt").read_text(encoding="utf-8")
  assert "EOS" not in (
    Root / "ProjectTemplates" / "Default" / "{PROJECT_NAME}" / "Source" / "Entry.cpp.template"
  ).read_text(encoding="utf-8")
  Text = (Root / "Tools" / "Build" / "broccoli_build" / "cli.py").read_text(encoding="utf-8")
  assert "--eos-binary" not in Text and "--online-resources-dir" not in Text
