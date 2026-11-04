"""Stage files required for local runtime execution."""

from pathlib import Path

from .common import ConvertLevels, CopyDirectory, CopyFile, EnsureDirectory
from .runtime_plugins import StagePlugins


def StageRuntime(
  Configuration: str,
  EngineDirectory: Path,
  GameDirectory: Path,
  OutputDirectory: Path,
  EngineBinary: Path,
  GameName: str,
  ConvertLevelsScript: Path,
  MingwRuntime: list[Path] | None = None,
  PluginDependencies: list[Path] | None = None,
) -> None:
  EnsureDirectory(OutputDirectory)
  CopyFile(EngineBinary, OutputDirectory)
  for RuntimeBinary in MingwRuntime or []:
    CopyFile(RuntimeBinary, OutputDirectory)
  StagePlugins(OutputDirectory, PluginDependencies or [])

  if Configuration.casefold() == "editor":
    print("Editor configuration: runtime resources staging skipped.")
    return

  ResourcesDirectory = OutputDirectory / "Resources"
  CopyDirectory(EngineDirectory / "Resources", ResourcesDirectory / "Engine")
  CopyDirectory(GameDirectory / "Resources", ResourcesDirectory / GameName)

  ConvertLevels(ConvertLevelsScript, ResourcesDirectory)
