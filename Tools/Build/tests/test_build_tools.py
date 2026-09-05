import json
from pathlib import Path

import pytest
from broccoli_build.package_runtime import PackageRuntime
from broccoli_build.plugins import GeneratePluginsCmake, RemoveDisabledExamplePluginArtifacts
from broccoli_build.prepare_output import PrepareOutput
from broccoli_build.stage_runtime import StageRuntime
from broccoli_build.verify_runtime import VerifyRuntime


@pytest.fixture(name="TmpPath")
def CreateTmpPath(tmp_path: Path) -> Path:
  return tmp_path


def WriteConvertLevelsScript(ScriptPath: Path) -> None:
  ScriptPath.write_text(
    "from pathlib import Path\n"
    "import sys\n"
    "Resources = Path(sys.argv[sys.argv.index('--input') + 1])\n"
    "for Source in Resources.rglob('*.BLevel.json'):\n"
    "  Source.with_suffix('').with_suffix('.BLevel').write_bytes(Source.read_bytes())\n"
    "  Source.unlink()\n",
    encoding="utf-8",
  )


def TestPrepareOutputRemovesGeneratedArtifacts(TmpPath: Path) -> None:
  for Name in ("Logs", "Saved", "Resources", "Resources-EOS"):
    (TmpPath / Name).mkdir()
  (TmpPath / "Log.txt").write_text("log", encoding="utf-8")

  PrepareOutput(TmpPath)

  assert not any(
    (TmpPath / Name).exists() for Name in ("Logs", "Saved", "Resources", "Resources-EOS", "Log.txt")
  )


def WritePluginSettings(TmpPath: Path, Plugins: object) -> None:
  (TmpPath / ".broccoli-project.json").write_text(
    json.dumps({"schema_version": 1, "plugins": Plugins}) + "\n",
    encoding="utf-8",
    newline="\n",
  )


def TestGeneratePluginsCmakeReflectsConfigurationSettings(TmpPath: Path) -> None:
  WritePluginSettings(
    TmpPath,
    [{"name": "ExamplePlugin", "configurations": ["Debug", "Editor", "Release"]}],
  )

  assert GeneratePluginsCmake(TmpPath)
  assert not GeneratePluginsCmake(TmpPath)
  assert (TmpPath / "Intermediate" / "Generated" / "Plugins.cmake").read_text(
    encoding="utf-8"
  ) == (
    "set(BROCCOLI_PLUGINS\n"
    "  ExamplePlugin\n"
    ")\n\n"
    "set(BROCCOLI_PLUGINS_DEBUG\n"
    "  ExamplePlugin\n"
    ")\n\n"
    "set(BROCCOLI_PLUGINS_EDITOR\n"
    "  ExamplePlugin\n"
    ")\n\n"
    "set(BROCCOLI_PLUGINS_RELEASE\n"
    "  ExamplePlugin\n"
    ")\n"
  )


def TestGeneratePluginsCmakeRemovesOnlyDisabledConfigurationArtifacts(TmpPath: Path) -> None:
  WritePluginSettings(TmpPath, [{"name": "ExamplePlugin", "configurations": ["Editor"]}])
  for Configuration in ("Debug", "Editor", "Release"):
    ArtifactDirectory = TmpPath / "Bin" / "x64" / Configuration / "Plugins" / "ExamplePlugin"
    ArtifactDirectory.mkdir(parents=True)
    (ArtifactDirectory / "ExamplePlugin.dll").write_bytes(b"plugin")

  assert GeneratePluginsCmake(TmpPath)
  RemoveDisabledExamplePluginArtifacts(TmpPath)

  assert not (TmpPath / "Bin" / "x64" / "Debug" / "Plugins" / "ExamplePlugin").exists()
  assert (TmpPath / "Bin" / "x64" / "Editor" / "Plugins" / "ExamplePlugin").is_dir()
  assert not (TmpPath / "Bin" / "x64" / "Release" / "Plugins" / "ExamplePlugin").exists()


def TestGeneratePluginsCmakeRemovesAllArtifactsWhenPluginIsUndefined(TmpPath: Path) -> None:
  WritePluginSettings(TmpPath, [])
  for Configuration in ("Debug", "Editor", "Release"):
    ArtifactDirectory = TmpPath / "Bin" / "x64" / Configuration / "Plugins" / "ExamplePlugin"
    ArtifactDirectory.mkdir(parents=True)

  assert GeneratePluginsCmake(TmpPath)
  RemoveDisabledExamplePluginArtifacts(TmpPath)

  assert (TmpPath / "Intermediate" / "Generated" / "Plugins.cmake").read_text(
    encoding="utf-8"
  ) == (
    "set(BROCCOLI_PLUGINS)\n\n"
    "set(BROCCOLI_PLUGINS_DEBUG)\n\n"
    "set(BROCCOLI_PLUGINS_EDITOR)\n\n"
    "set(BROCCOLI_PLUGINS_RELEASE)\n"
  )
  for Configuration in ("Debug", "Editor", "Release"):
    assert not (
      TmpPath / "Bin" / "x64" / Configuration / "Plugins" / "ExamplePlugin"
    ).exists()


@pytest.mark.parametrize(
  ("Plugins", "Message"),
  [
    ({"ExamplePlugin": True}, "must be an array"),
    (["ExamplePlugin"], "must be an object"),
    ([{"name": "", "configurations": []}], "non-empty string"),
    ([{"name": "ExamplePlugin", "configurations": "Editor"}], "must be an array"),
    ([{"name": "ExamplePlugin", "configurations": [1]}], "must be a string"),
    ([{"name": "ExamplePlugin", "configurations": ["Profile"]}], "unknown configuration"),
    ([{"name": "ExamplePlugin", "configurations": ["Editor", "Editor"]}], "duplicated"),
    (
      [{"name": "ExamplePlugin", "configurations": ["Editor"], "enabled": True}],
      "not supported",
    ),
    (
      [
        {"name": "ExamplePlugin", "configurations": ["Debug"]},
        {"name": "ExamplePlugin", "configurations": ["Editor"]},
      ],
      "more than once",
    ),
  ],
)
def TestGeneratePluginsCmakeRejectsInvalidSettings(
  TmpPath: Path, Plugins: object, Message: str
) -> None:
  WritePluginSettings(TmpPath, Plugins)

  with pytest.raises(ValueError, match=Message):
    GeneratePluginsCmake(TmpPath)


def TestStageRuntimeStagesEditorBinariesWithoutResources(TmpPath: Path) -> None:
  OutputDirectory = TmpPath / "Output"
  EngineBinary = TmpPath / "BroccoliEngine.dll"
  EosBinary = TmpPath / "EOSSDK-Win64-Shipping.dll"
  EngineBinary.write_bytes(b"engine")
  EosBinary.write_bytes(b"eos")

  StageRuntime(
    "Editor",
    TmpPath / "Engine",
    TmpPath / "Game",
    OutputDirectory,
    EngineBinary,
    "Game",
    EosBinary,
    TmpPath / "ConvertLevels.py",
  )

  assert (OutputDirectory / "BroccoliEngine.dll").is_file()
  assert (OutputDirectory / "EOSSDK-Win64-Shipping.dll").is_file()
  assert not (OutputDirectory / "Resources").exists()


def TestStageRuntimeAcceptsEngineBinaryAlreadyInOutput(TmpPath: Path) -> None:
  OutputDirectory = TmpPath / "Output"
  OutputDirectory.mkdir()
  EngineBinary = OutputDirectory / "BroccoliEngine.dll"
  EosBinary = TmpPath / "EOSSDK-Win64-Shipping.dll"
  EngineBinary.write_bytes(b"engine")
  EosBinary.write_bytes(b"eos")

  StageRuntime(
    "Editor",
    TmpPath / "Engine",
    TmpPath / "Game",
    OutputDirectory,
    EngineBinary,
    "Game",
    EosBinary,
    TmpPath / "ConvertLevels.py",
  )

  assert EngineBinary.read_bytes() == b"engine"
  assert (OutputDirectory / "EOSSDK-Win64-Shipping.dll").is_file()


def TestStageRuntimeStagesDebugArtifacts(TmpPath: Path) -> None:
  EngineDirectory = TmpPath / "Engine"
  GameDirectory = TmpPath / "Game"
  OutputDirectory = TmpPath / "Output"
  (EngineDirectory / "Resources").mkdir(parents=True)
  (GameDirectory / "Resources").mkdir(parents=True)
  (GameDirectory / "Resources-EOS").mkdir(parents=True)
  (EngineDirectory / "Resources" / "engine.txt").write_text("engine", encoding="utf-8")
  (GameDirectory / "Resources" / "level.BLevel.json").write_text("{}", encoding="utf-8")
  (GameDirectory / "Resources-EOS" / "online.BLevel.json").write_text("{}", encoding="utf-8")
  (GameDirectory / "Resources-EOS" / "online.txt").write_text("online", encoding="utf-8")
  EngineBinary = TmpPath / "BroccoliEngine.dll"
  EosBinary = TmpPath / "EOSSDK-Win64-Shipping.dll"
  ConvertLevelsScript = TmpPath / "ConvertLevels.py"
  EngineBinary.write_bytes(b"engine")
  EosBinary.write_bytes(b"eos")
  WriteConvertLevelsScript(ConvertLevelsScript)

  StageRuntime(
    "Debug",
    EngineDirectory,
    GameDirectory,
    OutputDirectory,
    EngineBinary,
    "Game",
    EosBinary,
    ConvertLevelsScript,
  )

  assert (OutputDirectory / "BroccoliEngine.dll").is_file()
  assert (OutputDirectory / "EOSSDK-Win64-Shipping.dll").is_file()
  assert (OutputDirectory / "Resources" / "Engine" / "engine.txt").is_file()
  assert (OutputDirectory / "Resources" / "Game" / "level.BLevel").is_file()
  assert not (OutputDirectory / "Resources" / "Game" / "level.BLevel.json").exists()
  assert (OutputDirectory / "Resources-EOS" / "online.BLevel").is_file()
  assert not (OutputDirectory / "Resources-EOS" / "online.BLevel.json").exists()
  assert (OutputDirectory / "Resources-EOS" / "online.txt").is_file()


def TestPackageRuntimeCreatesVerifiedLayout(TmpPath: Path) -> None:
  OutputDirectory = TmpPath / "Output"
  PublishDirectory = TmpPath / "Publish"
  OnlineResourcesDirectory = TmpPath / "OnlineResources"
  ResourcesDirectory = OutputDirectory / "Resources"
  (ResourcesDirectory / "Engine").mkdir(parents=True)
  (ResourcesDirectory / "Game").mkdir(parents=True)
  (ResourcesDirectory / "Engine" / "engine.txt").write_text("engine", encoding="utf-8")
  (ResourcesDirectory / "Game" / "level.BLevel").write_bytes(b"level")
  PluginDirectory = OutputDirectory / "Plugins" / "ExamplePlugin"
  PluginDirectory.mkdir(parents=True)
  (PluginDirectory / "ExamplePlugin.dll").write_bytes(b"plugin")
  (PluginDirectory / "plugin.json").write_text("{}", encoding="utf-8")
  (OutputDirectory / "ExamplePlugin.dll").write_bytes(b"plugin runtime")
  OnlineResourcesDirectory.mkdir()
  (OnlineResourcesDirectory / "online.BLevel.json").write_text("{}", encoding="utf-8")
  (OnlineResourcesDirectory / "online.txt").write_text("online", encoding="utf-8")
  GameBinary = OutputDirectory / "Game-game.exe"
  EngineBinary = OutputDirectory / "BroccoliEngine.dll"
  EosBinary = OutputDirectory / "EOSSDK-Win64-Shipping.dll"
  BootstrapBinary = OutputDirectory / "BroccoliBootstrap.exe"
  ConvertLevelsScript = TmpPath / "ConvertLevels.py"
  GameBinary.write_bytes(b"game")
  EngineBinary.write_bytes(b"engine")
  EosBinary.write_bytes(b"eos")
  BootstrapBinary.write_bytes(b"bootstrap")
  WriteConvertLevelsScript(ConvertLevelsScript)

  PackageRuntime(
    "Release",
    OutputDirectory,
    PublishDirectory,
    GameBinary,
    EngineBinary,
    "Game",
    EosBinary,
    OnlineResourcesDirectory,
    ConvertLevelsScript,
    BootstrapBinary,
  )

  VerifyRuntime(OutputDirectory, "Game", PublishDirectory)
  assert (PublishDirectory / "Game.exe").is_file()
  assert (PublishDirectory / "Binaries" / "Game.exe").is_file()
  assert (PublishDirectory / "Binaries" / "ExamplePlugin.dll").is_file()
  assert (PublishDirectory / "Binaries" / "Plugins" / "ExamplePlugin" / "ExamplePlugin.dll").is_file()
  assert (PublishDirectory / "Binaries" / "Plugins" / "ExamplePlugin" / "plugin.json").is_file()
  assert (PublishDirectory / "Resources-EOS" / "online.BLevel").is_file()
  assert not (PublishDirectory / "Resources-EOS" / "online.BLevel.json").exists()
  assert (PublishDirectory / "Resources-EOS" / "online.txt").is_file()


def TestPackageRuntimeAcceptsBuildWithoutPlugins(TmpPath: Path) -> None:
  OutputDirectory = TmpPath / "Output"
  PublishDirectory = TmpPath / "Publish"
  OnlineResourcesDirectory = TmpPath / "OnlineResources"
  ResourcesDirectory = OutputDirectory / "Resources"
  (ResourcesDirectory / "Engine").mkdir(parents=True)
  (ResourcesDirectory / "Game").mkdir(parents=True)
  (ResourcesDirectory / "Engine" / "engine.txt").write_text("engine", encoding="utf-8")
  (ResourcesDirectory / "Game" / "level.BLevel").write_bytes(b"level")
  OnlineResourcesDirectory.mkdir()
  GameBinary = OutputDirectory / "Game-game.exe"
  EngineBinary = OutputDirectory / "BroccoliEngine.dll"
  EosBinary = OutputDirectory / "EOSSDK-Win64-Shipping.dll"
  BootstrapBinary = OutputDirectory / "BroccoliBootstrap.exe"
  ConvertLevelsScript = TmpPath / "ConvertLevels.py"
  GameBinary.write_bytes(b"game")
  EngineBinary.write_bytes(b"engine")
  EosBinary.write_bytes(b"eos")
  BootstrapBinary.write_bytes(b"bootstrap")
  WriteConvertLevelsScript(ConvertLevelsScript)

  PackageRuntime(
    "Release",
    OutputDirectory,
    PublishDirectory,
    GameBinary,
    EngineBinary,
    "Game",
    EosBinary,
    OnlineResourcesDirectory,
    ConvertLevelsScript,
    BootstrapBinary,
  )

  VerifyRuntime(OutputDirectory, "Game", PublishDirectory)
  assert not (PublishDirectory / "Binaries" / "Plugins").exists()


def TestVerifyRuntimeReportsMissingArtifacts(TmpPath: Path) -> None:
  with pytest.raises(ValueError, match="Missing required runtime artifacts"):
    VerifyRuntime(TmpPath, "Game")
