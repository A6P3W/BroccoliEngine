from pathlib import Path

import pytest
from broccoli_build.package_runtime import PackageRuntime
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
  assert (OutputDirectory / "Resources-EOS" / "online.txt").is_file()


def TestPackageRuntimeCreatesVerifiedLayout(TmpPath: Path) -> None:
  OutputDirectory = TmpPath / "Output"
  PublishDirectory = TmpPath / "Publish"
  ResourcesDirectory = OutputDirectory / "Resources"
  (ResourcesDirectory / "Engine").mkdir(parents=True)
  (ResourcesDirectory / "Game").mkdir(parents=True)
  (ResourcesDirectory / "Engine" / "engine.txt").write_text("engine", encoding="utf-8")
  (ResourcesDirectory / "Game" / "level.BLevel").write_bytes(b"level")
  (OutputDirectory / "Resources-EOS").mkdir()
  (OutputDirectory / "Resources-EOS" / "online.txt").write_text("online", encoding="utf-8")
  GameBinary = OutputDirectory / "Game-game.exe"
  EngineBinary = OutputDirectory / "BroccoliEngine.dll"
  EosBinary = OutputDirectory / "EOSSDK-Win64-Shipping.dll"
  BootstrapBinary = OutputDirectory / "BroccoliBootstrap.exe"
  ConvertLevelsScript = TmpPath / "ConvertLevels.py"
  GameBinary.write_bytes(b"game")
  EngineBinary.write_bytes(b"engine")
  EosBinary.write_bytes(b"eos")
  BootstrapBinary.write_bytes(b"bootstrap")
  ConvertLevelsScript.write_text("", encoding="utf-8")

  PackageRuntime(
    "Release",
    OutputDirectory,
    PublishDirectory,
    GameBinary,
    EngineBinary,
    "Game",
    EosBinary,
    OutputDirectory / "Resources-EOS",
    ConvertLevelsScript,
    BootstrapBinary,
  )

  VerifyRuntime(OutputDirectory, "Game", PublishDirectory)
  assert (PublishDirectory / "Game.exe").is_file()
  assert (PublishDirectory / "Binaries" / "Game.exe").is_file()
  assert (PublishDirectory / "Resources-EOS" / "online.txt").is_file()


def TestVerifyRuntimeReportsMissingArtifacts(TmpPath: Path) -> None:
  with pytest.raises(ValueError, match="Missing required runtime artifacts"):
    VerifyRuntime(TmpPath, "Game")
