import json
from pathlib import Path

import pytest
from broccoli_build import cli
from broccoli_build.plugins import GeneratePluginsCmake
from broccoli_build.prepare_output import PrepareOutput


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
  for Name in ("Logs", "Saved", "Resources"):
    (TmpPath / Name).mkdir()
  (TmpPath / "Log.txt").write_text("log", encoding="utf-8")

  PrepareOutput(TmpPath)
  assert (TmpPath / "Saved").is_dir()

  assert not any((TmpPath / Name).exists() for Name in ("Logs", "Resources", "Log.txt"))


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


def TestBuildLeavesPluginGenerationToCmakeConfigure(
  TmpPath: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
  WritePluginSettings(TmpPath, [])
  (TmpPath / "CMakeUserPresets.json").write_text("{}", encoding="utf-8")
  CacheFile = TmpPath / "build" / "windows-x64-gcc26" / "CMakeCache.txt"
  CacheFile.parent.mkdir(parents=True)
  CacheFile.write_text("cache", encoding="utf-8")
  Commands: list[tuple[list[str], Path]] = []

  def RecordRun(Command: list[str], cwd: Path, check: bool) -> None:
    Commands.append((Command, cwd))

  monkeypatch.setattr(cli, "StageBuiltProject", lambda *Arguments: None)
  monkeypatch.setattr(cli, "FindCmakeCommand", lambda: "cmake")
  monkeypatch.setattr(cli.subprocess, "run", RecordRun)

  cli.Build(TmpPath, "Debug", False)

  assert Commands == [
    (
      ["cmake", "--build", "--preset", "debug-local", "--target", "BroccoliProjectBuild_Debug"],
      TmpPath,
    )
  ]


def TestRegenerateDoesNotBuild(TmpPath: Path, monkeypatch: pytest.MonkeyPatch) -> None:
  WritePluginSettings(TmpPath, [])
  (TmpPath / "CMakeUserPresets.json").write_text("{}", encoding="utf-8")
  Commands: list[list[str]] = []

  def RecordRun(Command: list[str], cwd: Path, check: bool) -> None:
    Commands.append(Command)

  monkeypatch.setattr(cli, "StageBuiltProject", lambda *Arguments: None)
  monkeypatch.setattr(cli, "FindCmakeCommand", lambda: "cmake")
  monkeypatch.setattr(cli.subprocess, "run", RecordRun)

  cli.Regenerate(TmpPath)

  assert Commands == [["cmake", "--preset", "windows-x64-local"]]


@pytest.mark.parametrize("HasUserPresets", [False, True])
@pytest.mark.parametrize("Configuration", ["Debug", "Editor"])
def TestBuildSelectsEnvironmentOrLocalPresets(
  TmpPath: Path,
  monkeypatch: pytest.MonkeyPatch,
  HasUserPresets: bool,
  Configuration: str,
) -> None:
  if HasUserPresets:
    (TmpPath / "CMakeUserPresets.json").write_text("{}", encoding="utf-8")
  Commands: list[list[str]] = []

  def RecordRun(Command: list[str], cwd: Path, check: bool) -> None:
    assert cwd == TmpPath
    assert check
    Commands.append(Command)

  monkeypatch.setattr(cli, "StageBuiltProject", lambda *Arguments: None)
  monkeypatch.setattr(cli, "FindCmakeCommand", lambda: "cmake")
  monkeypatch.setattr(cli.subprocess, "run", RecordRun)

  cli.Build(TmpPath, Configuration, False)

  Suffix = "local" if HasUserPresets else "env"
  assert Commands == [
    ["cmake", "--preset", f"windows-x64-{Suffix}"],
    [
      "cmake",
      "--build",
      "--preset",
      f"{Configuration.casefold()}-{Suffix}",
      "--target",
      f"BroccoliProjectBuild_{Configuration}",
    ],
  ]


def TestBuildRecordsLatestConfiguration(TmpPath: Path, monkeypatch: pytest.MonkeyPatch) -> None:
  WritePluginSettings(TmpPath, [])
  CacheFile = TmpPath / "build" / "windows-x64-gcc26" / "CMakeCache.txt"
  CacheFile.parent.mkdir(parents=True)
  CacheFile.write_text("cache", encoding="utf-8")

  monkeypatch.setattr(cli, "StageBuiltProject", lambda *Arguments: None)
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
  (TmpPath / "Intermediate" / "LastBuildConfiguration.txt").write_text("Debug\n", encoding="utf-8")
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
  (TmpPath / "Intermediate" / "LastBuildConfiguration.txt").write_text("Editor\n", encoding="utf-8")
  CliArguments, ApplicationArguments = cli.SplitRunApplicationArguments(
    ["run", "--latest", "--project-dir", str(TmpPath), "--", "--control"]
  )
  Arguments = cli.CreateParser().parse_args(CliArguments)

  Configuration = cli.ResolveRunInvocation(Arguments)

  assert Configuration == "Editor"
  assert ApplicationArguments == ["--control"]


@pytest.mark.parametrize(
  ("RawArguments", "ExpectedApplicationArguments"),
  [
    (["run", "Debug", "--", "--control"], ["--control"]),
    (["run", "Debug", "--", "level1"], ["level1"]),
    (["run", "--latest", "--", "--config", "custom"], ["--config", "custom"]),
  ],
)
def TestRunSeparatesApplicationArguments(
  RawArguments: list[str], ExpectedApplicationArguments: list[str]
) -> None:
  CliArguments, ApplicationArguments = cli.SplitRunApplicationArguments(RawArguments)

  assert ApplicationArguments == ExpectedApplicationArguments
  cli.CreateParser().parse_args(CliArguments)


def TestRunParserAcceptsControlOption() -> None:
  Arguments = cli.CreateParser().parse_args(["run", "Debug", "--control"])

  assert Arguments.control


def TestIncludeControlArgumentAddsItOnlyOnce() -> None:
  assert cli.IncludeControlArgument(["--level", "Test"], True) == [
    "--level",
    "Test",
    "--control",
  ]
  assert cli.IncludeControlArgument(["--control"], True) == ["--control"]
  assert cli.IncludeControlArgument(["--level", "Test"], False) == ["--level", "Test"]


def TestMainForwardsSeparatedArgumentsWithoutReinterpretingThem(
  TmpPath: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
  (TmpPath / "Intermediate").mkdir()
  (TmpPath / "Intermediate" / "LastBuildConfiguration.txt").write_text("Editor\n", encoding="utf-8")
  Calls: list[tuple[Path, str, list[str]]] = []

  def RecordRun(
    ProjectDirectory: Path, Configuration: str, ApplicationArguments: list[str]
  ) -> None:
    Calls.append((ProjectDirectory, Configuration, ApplicationArguments))

  monkeypatch.setattr(cli, "Run", RecordRun)
  monkeypatch.setattr(
    cli.sys,
    "argv",
    [
      "broccoli_build",
      "run",
      "--latest",
      "--project-dir",
      str(TmpPath),
      "--",
      "--config",
      "custom",
    ],
  )

  assert cli.Main() == 0
  assert Calls == [(TmpPath.resolve(), "Editor", ["--config", "custom"])]


def TestMainAddsControlArgumentOnlyOnce(TmpPath: Path, monkeypatch: pytest.MonkeyPatch) -> None:
  Calls: list[list[str]] = []

  def RecordRun(
    _ProjectDirectory: Path, _Configuration: str, ApplicationArguments: list[str]
  ) -> None:
    Calls.append(ApplicationArguments)

  monkeypatch.setattr(cli, "Run", RecordRun)
  monkeypatch.setattr(
    cli.sys,
    "argv",
    ["broccoli_build", "run", "Debug", "--control", "--", "--control", "--level", "Test"],
  )

  assert cli.Main() == 0
  assert Calls == [["--control", "--level", "Test"]]


def TestCleanOnlyRemovesTheRequestedConfiguration(
  TmpPath: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
  for Configuration in ("Debug", "Editor"):
    (TmpPath / "Bin" / "x64" / Configuration).mkdir(parents=True)
    (TmpPath / "Publish" / Configuration).mkdir(parents=True)
  (TmpPath / "CMakeUserPresets.json").write_text("{}", encoding="utf-8")
  CacheFile = TmpPath / "build" / "windows-x64-gcc26" / "CMakeCache.txt"
  CacheFile.parent.mkdir(parents=True)
  CacheFile.write_text("cache", encoding="utf-8")
  Commands: list[list[str]] = []

  def RecordRun(Command: list[str], cwd: Path, check: bool) -> None:
    Commands.append(Command)

  monkeypatch.setattr(cli, "StageBuiltProject", lambda *Arguments: None)
  monkeypatch.setattr(cli, "FindCmakeCommand", lambda: "cmake")
  monkeypatch.setattr(cli.subprocess, "run", RecordRun)

  cli.Clean(TmpPath, "Debug", False)

  assert Commands == [["cmake", "--build", "--preset", "debug-local", "--target", "clean"]]
  assert not (TmpPath / "Bin" / "x64" / "Debug").exists()
  assert not (TmpPath / "Publish" / "Debug").exists()
  assert (TmpPath / "Bin" / "x64" / "Editor").is_dir()
  assert (TmpPath / "Publish" / "Editor").is_dir()


def TestCleanAllRemovesOnlyGeneratedDirectories(TmpPath: Path) -> None:
  for Directory in (
    "build/windows-x64",
    "build/windows-x64-gcc",
    "build/windows-x64-gcc26",
    "Intermediate",
    "Bin",
    "Publish",
  ):
    (TmpPath / Directory).mkdir(parents=True)
  SourceFile = TmpPath / "Source" / "main.cpp"
  SourceFile.parent.mkdir()
  SourceFile.write_text("source", encoding="utf-8")

  cli.Clean(TmpPath, None, True)

  assert not any(
    (TmpPath / "build" / Directory).exists()
    for Directory in ("windows-x64", "windows-x64-gcc", "windows-x64-gcc26")
  )
  assert not any((TmpPath / Directory).exists() for Directory in ("Intermediate", "Bin", "Publish"))
  assert SourceFile.is_file()


def TestGeneratePluginsCmakeReflectsConfigurationSettings(TmpPath: Path) -> None:
  WritePluginSettings(
    TmpPath,
    [{"name": "ExamplePlugin", "configurations": ["Debug", "Editor", "Release"]}],
  )

  assert GeneratePluginsCmake(TmpPath)
  GeneratedFile = TmpPath / "Intermediate" / "Generated" / "Plugins.cmake"
  ModifiedTime = GeneratedFile.stat().st_mtime_ns
  assert not GeneratePluginsCmake(TmpPath)
  assert GeneratedFile.stat().st_mtime_ns == ModifiedTime
  assert GeneratedFile.read_text(encoding="utf-8") == (
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
