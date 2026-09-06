import json
from pathlib import Path

import pytest
from broccoli_build import cli
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


def TestBuildParserNormalizesConfigurationNames() -> None:
  Parser = cli.CreateParser()

  Arguments = Parser.parse_args(["build", "eDiToR"])

  assert Arguments.configuration == "Editor"


def TestBuildParserAcceptsConfigOption() -> None:
  Parser = cli.CreateParser()

  Arguments = Parser.parse_args(["build", "--config", "release"])

  assert Arguments.config == "Release"


def TestBuildConfiguresOnlyWhenPluginSettingsChange(TmpPath: Path, monkeypatch: pytest.MonkeyPatch) -> None:
  WritePluginSettings(TmpPath, [])
  CacheFile = TmpPath / "build" / "windows-x64" / "CMakeCache.txt"
  CacheFile.parent.mkdir(parents=True)
  CacheFile.write_text("cache", encoding="utf-8")
  Commands: list[tuple[list[str], Path]] = []

  def RecordRun(Command: list[str], cwd: Path, check: bool) -> None:
    Commands.append((Command, cwd))

  monkeypatch.setattr(cli, "FindCmakeCommand", lambda: "cmake")
  monkeypatch.setattr(cli.subprocess, "run", RecordRun)

  cli.Build(TmpPath, "Debug", False)

  assert Commands == [
    (["cmake", "--preset", "windows-x64-local"], TmpPath),
    (["cmake", "--build", "--preset", "debug-local", "--target", "BroccoliProjectBuild_Debug"], TmpPath),
  ]
  Commands.clear()

  cli.Build(TmpPath, "Debug", False)

  assert Commands == [
    (["cmake", "--build", "--preset", "debug-local", "--target", "BroccoliProjectBuild_Debug"], TmpPath)
  ]


def TestRegenerateDoesNotBuild(TmpPath: Path, monkeypatch: pytest.MonkeyPatch) -> None:
  WritePluginSettings(TmpPath, [])
  Commands: list[list[str]] = []

  def RecordRun(Command: list[str], cwd: Path, check: bool) -> None:
    Commands.append(Command)

  monkeypatch.setattr(cli, "FindCmakeCommand", lambda: "cmake")
  monkeypatch.setattr(cli.subprocess, "run", RecordRun)

  cli.Regenerate(TmpPath)

  assert Commands == [["cmake", "--preset", "windows-x64-local"]]


def TestBuildRecordsLatestConfiguration(TmpPath: Path, monkeypatch: pytest.MonkeyPatch) -> None:
  WritePluginSettings(TmpPath, [])
  CacheFile = TmpPath / "build" / "windows-x64" / "CMakeCache.txt"
  CacheFile.parent.mkdir(parents=True)
  CacheFile.write_text("cache", encoding="utf-8")

  monkeypatch.setattr(cli, "FindCmakeCommand", lambda: "cmake")
  monkeypatch.setattr(cli.subprocess, "run", lambda *_, **__: None)

  cli.Build(TmpPath, "Release", False)

  assert cli.LoadLatestBuildConfiguration(TmpPath) == "Release"


def TestRunUsesLatestConfigurationAndForwardsArguments(
  TmpPath: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
  WritePluginSettings(TmpPath, [])
  ExecutablePath = TmpPath / "Publish" / "Debug" / "Launcher.exe"
  ExecutablePath.parent.mkdir(parents=True)
  ExecutablePath.write_bytes(b"launcher")
  (TmpPath / "Intermediate").mkdir()
  (TmpPath / "Intermediate" / "LastBuildConfiguration.txt").write_text(
    "Debug\n", encoding="utf-8"
  )
  Commands: list[tuple[list[str], Path]] = []

  def RecordPopen(Command: list[str], cwd: Path) -> None:
    Commands.append((Command, cwd))

  monkeypatch.setattr(cli.subprocess, "Popen", RecordPopen)

  cli.Run(TmpPath, cli.LoadLatestBuildConfiguration(TmpPath), ["--example", "value"])

  assert Commands == [([str(ExecutablePath), "--example", "value"], TmpPath)]


def TestRunUsesGeneratedProjectName(TmpPath: Path) -> None:
  (TmpPath / ".broccoli-project.json").write_text(
    '{"project_name": "ExampleGame", "plugins": []}\n', encoding="utf-8"
  )

  assert cli.GetRunExecutable(TmpPath, "Editor") == (
    TmpPath / "Bin" / "x64" / "Editor" / "ExampleGame-game.exe"
  )
  assert cli.GetRunExecutable(TmpPath, "Debug") == TmpPath / "Publish" / "Debug" / "ExampleGame.exe"


def TestRunRejectsMissingLatestConfiguration(TmpPath: Path) -> None:
  with pytest.raises(ValueError, match="Latest build configuration does not exist"):
    cli.LoadLatestBuildConfiguration(TmpPath)


def TestRunParserForwardsArgumentsAfterLatestSeparator(TmpPath: Path) -> None:
  (TmpPath / "Intermediate").mkdir()
  (TmpPath / "Intermediate" / "LastBuildConfiguration.txt").write_text(
    "Editor\n", encoding="utf-8"
  )
  Parser = cli.CreateParser()
  Arguments = Parser.parse_args(
    ["run", "--latest", "--project-dir", str(TmpPath), "--", "--automation"]
  )

  Configuration, ApplicationArguments = cli.ResolveRunInvocation(Arguments)

  assert Configuration == "Editor"
  assert ApplicationArguments == ["--automation"]


def TestCleanOnlyRemovesTheRequestedConfiguration(
  TmpPath: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
  for Configuration in ("Debug", "Editor"):
    (TmpPath / "Bin" / "x64" / Configuration).mkdir(parents=True)
    (TmpPath / "Publish" / Configuration).mkdir(parents=True)
  CacheFile = TmpPath / "build" / "windows-x64" / "CMakeCache.txt"
  CacheFile.parent.mkdir(parents=True)
  CacheFile.write_text("cache", encoding="utf-8")
  Commands: list[list[str]] = []

  def RecordRun(Command: list[str], cwd: Path, check: bool) -> None:
    Commands.append(Command)

  monkeypatch.setattr(cli, "FindCmakeCommand", lambda: "cmake")
  monkeypatch.setattr(cli.subprocess, "run", RecordRun)

  cli.Clean(TmpPath, "Debug", False)

  assert Commands == [["cmake", "--build", "--preset", "debug-local", "--target", "clean"]]
  assert not (TmpPath / "Bin" / "x64" / "Debug").exists()
  assert not (TmpPath / "Publish" / "Debug").exists()
  assert (TmpPath / "Bin" / "x64" / "Editor").is_dir()
  assert (TmpPath / "Publish" / "Editor").is_dir()


def TestCleanAllRemovesOnlyGeneratedDirectories(TmpPath: Path) -> None:
  for Directory in ("build/windows-x64", "Intermediate", "Bin", "Publish"):
    (TmpPath / Directory).mkdir(parents=True)
  SourceFile = TmpPath / "Source" / "main.cpp"
  SourceFile.parent.mkdir()
  SourceFile.write_text("source", encoding="utf-8")

  cli.Clean(TmpPath, None, True)

  assert not (TmpPath / "build" / "windows-x64").exists()
  assert not any((TmpPath / Directory).exists() for Directory in ("Intermediate", "Bin", "Publish"))
  assert SourceFile.is_file()


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
    ["ExamplePlugin"],
  )

  VerifyRuntime(OutputDirectory, "Game", PublishDirectory, ["ExamplePlugin"])
  assert (PublishDirectory / "Game.exe").is_file()
  assert (PublishDirectory / "Binaries" / "Game.exe").is_file()
  assert (PublishDirectory / "Binaries" / "Plugins" / "ExamplePlugin" / "ExamplePlugin.dll").is_file()
  assert (PublishDirectory / "Binaries" / "Plugins" / "ExamplePlugin" / "plugin.json").is_file()
  assert (PublishDirectory / "Resources-EOS" / "online.BLevel").is_file()
  assert not (PublishDirectory / "Resources-EOS" / "online.BLevel.json").exists()
  assert (PublishDirectory / "Resources-EOS" / "online.txt").is_file()

  (PluginDirectory / "ExamplePlugin.dll").unlink()
  with pytest.raises(ValueError, match="ExamplePlugin.dll"):
    VerifyRuntime(OutputDirectory, "Game", PublishDirectory, ["ExamplePlugin"])
  with pytest.raises(ValueError, match="ExamplePlugin.dll"):
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
      ["ExamplePlugin"],
    )


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
