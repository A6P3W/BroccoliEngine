"""Create a reproducible distributable runtime package."""

from pathlib import Path

from .common import (
    CopyDirectory,
    CopyFile,
    EnsureDirectory,
    RemovePath,
    RequireDirectory,
    RequireFile,
)


def PackageRuntime(
    Configuration: str,
    OutputDirectory: Path,
    PublishDirectory: Path,
    GameBinary: Path,
    EngineBinary: Path,
    GameName: str,
    EosBinary: Path,
    OnlineResourcesDirectory: Path,
    ConvertLevelsScript: Path,
    BootstrapBinary: Path,
) -> None:
    if Configuration.casefold() == "editor":
        print("Editor configuration: runtime packaging skipped.")
        return

    RequireFile(GameBinary, "Game executable")
    RequireFile(EngineBinary, "BroccoliEngine.dll")
    RequireFile(BootstrapBinary, "BroccoliBootstrap.exe")
    ResourcesDirectory = OutputDirectory / "Resources"
    RequireDirectory(ResourcesDirectory, "Staged resources directory")
    RequireFile(ConvertLevelsScript, "ConvertLevels.py")

    RemovePath(PublishDirectory)
    BinariesDirectory = PublishDirectory / "Binaries"
    EnsureDirectory(BinariesDirectory)
    CopyFile(GameBinary, BinariesDirectory)
    (BinariesDirectory / GameBinary.name).replace(BinariesDirectory / f"{GameName}.exe")
    CopyFile(EngineBinary, BinariesDirectory)
    if EosBinary.is_file():
        CopyFile(EosBinary, BinariesDirectory)
    CopyFile(BootstrapBinary, PublishDirectory)
    (PublishDirectory / BootstrapBinary.name).replace(
        PublishDirectory / f"{GameName}.exe"
    )
    CopyDirectory(ResourcesDirectory, PublishDirectory / "Resources")
    if OnlineResourcesDirectory.is_dir():
        CopyDirectory(OnlineResourcesDirectory, PublishDirectory / "Resources-EOS")
