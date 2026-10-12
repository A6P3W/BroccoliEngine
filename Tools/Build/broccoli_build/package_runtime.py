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
from .runtime_plugins import LoadRuntimePlugins, VerifyPlugins


def PackageRuntime(
  Configuration: str,
  OutputDirectory: Path,
  PublishDirectory: Path,
  GameBinary: Path,
  EngineBinary: Path,
  GameName: str,
  ConvertLevelsScript: Path,
  BootstrapBinary: Path,
  PluginDependencies: list[Path] | None = None,
  MingwRuntime: list[Path] | None = None,
) -> None:
  if Configuration.casefold() == "editor":
    print("Editor configuration: runtime packaging skipped.")
    return

  RequireFile(GameBinary, "Game executable")
  RequireFile(EngineBinary, "BroccoliEngine.dll")
  RequireFile(BootstrapBinary, "BroccoliBootstrap.exe")
  ResourcesDirectory = OutputDirectory / "Resources"
  RequiredPlugins = LoadRuntimePlugins(OutputDirectory)
  VerifyPlugins(OutputDirectory, PluginDependencies or [])
  RequireDirectory(ResourcesDirectory, "Staged resources directory")
  RequireFile(ConvertLevelsScript, "ConvertLevels.py")
  RemovePath(PublishDirectory)
  BinariesDirectory = PublishDirectory / "Binaries"
  EnsureDirectory(BinariesDirectory)
  CopyFile(GameBinary, BinariesDirectory)
  (BinariesDirectory / GameBinary.name).replace(BinariesDirectory / f"{GameName}.exe")
  CopyFile(EngineBinary, BinariesDirectory)
  for RuntimeBinary in MingwRuntime or []:
    CopyFile(OutputDirectory / RuntimeBinary.name, BinariesDirectory)
  CopyFile(OutputDirectory / "plugins.json", BinariesDirectory)
  for PluginName in RequiredPlugins:
    CopyFile(OutputDirectory / f"{PluginName}.dll", BinariesDirectory)
  for Dependency in PluginDependencies or []:
    CopyFile(OutputDirectory / Dependency.name, BinariesDirectory)
  CopyFile(BootstrapBinary, PublishDirectory)
  (PublishDirectory / BootstrapBinary.name).replace(PublishDirectory / f"{GameName}.exe")
  CopyDirectory(ResourcesDirectory, PublishDirectory / "Resources")
