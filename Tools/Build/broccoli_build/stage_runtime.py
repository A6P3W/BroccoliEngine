"""Stage files required for local runtime execution."""

from pathlib import Path

from .common import ConvertLevels, CopyDirectory, CopyFile, EnsureDirectory


def StageRuntime(
  Configuration: str,
  EngineDirectory: Path,
  GameDirectory: Path,
  OutputDirectory: Path,
  EngineBinary: Path,
  GameName: str,
  EosBinary: Path,
  ConvertLevelsScript: Path,
  MingwRuntime: list[Path] | None = None,
  RequiredPlugins: list[str] | None = None,
) -> None:
  EnsureDirectory(OutputDirectory)
  CopyFile(EngineBinary, OutputDirectory)
  if EosBinary.is_file():
    CopyFile(EosBinary, OutputDirectory)
  for RuntimeBinary in MingwRuntime or []:
    CopyFile(RuntimeBinary, OutputDirectory)
  if MingwRuntime:
    for PluginName in RequiredPlugins or []:
      CopyFile(
        OutputDirectory / "Plugins" / PluginName / f"{PluginName}.dll",
        OutputDirectory,
      )

  if Configuration.casefold() == "editor":
    print("Editor configuration: runtime resources staging skipped.")
    return

  ResourcesDirectory = OutputDirectory / "Resources"
  CopyDirectory(EngineDirectory / "Resources", ResourcesDirectory / "Engine")
  CopyDirectory(GameDirectory / "Resources", ResourcesDirectory / GameName)

  OnlineResourcesDirectory = GameDirectory / "Resources-EOS"
  if Configuration.casefold() == "debug" and OnlineResourcesDirectory.is_dir():
    StagedOnlineResourcesDirectory = OutputDirectory / "Resources-EOS"
    CopyDirectory(OnlineResourcesDirectory, StagedOnlineResourcesDirectory)
    ConvertLevels(ConvertLevelsScript, StagedOnlineResourcesDirectory)

  ConvertLevels(ConvertLevelsScript, ResourcesDirectory)
