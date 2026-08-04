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
) -> None:
    EnsureDirectory(OutputDirectory)
    CopyFile(EngineBinary, OutputDirectory)
    if EosBinary.is_file():
        CopyFile(EosBinary, OutputDirectory)

    if Configuration.casefold() == "editor":
        print("Editor configuration: runtime resources staging skipped.")
        return

    ResourcesDirectory = OutputDirectory / "Resources"
    CopyDirectory(EngineDirectory / "Resources", ResourcesDirectory / "Engine")
    CopyDirectory(GameDirectory / "Resources", ResourcesDirectory / GameName)

    OnlineResourcesDirectory = GameDirectory / "Resources-EOS"
    if Configuration.casefold() == "debug" and OnlineResourcesDirectory.is_dir():
        CopyDirectory(OnlineResourcesDirectory, OutputDirectory / "Resources-EOS")

    ConvertLevels(ConvertLevelsScript, ResourcesDirectory)
