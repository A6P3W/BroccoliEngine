"""Create a reproducible distributable runtime package."""

from pathlib import Path

from .common import (
  ConvertLevels,
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
  RequiredPlugins: list[str] | None = None,
) -> None:
  if Configuration.casefold() == "editor":
    print("Editor configuration: runtime packaging skipped.")
    return

  RequireFile(GameBinary, "Game executable")
  RequireFile(EngineBinary, "BroccoliEngine.dll")
  RequireFile(BootstrapBinary, "BroccoliBootstrap.exe")
  ResourcesDirectory = OutputDirectory / "Resources"
  PluginsDirectory = OutputDirectory / "Plugins"
  RequiredPlugins = RequiredPlugins or []
  RequireDirectory(ResourcesDirectory, "Staged resources directory")
  RequireFile(ConvertLevelsScript, "ConvertLevels.py")
  for PluginName in RequiredPlugins:
    PluginDirectory = PluginsDirectory / PluginName
    RequireDirectory(PluginDirectory, f"Required plugin directory '{PluginName}'")
    RequireFile(PluginDirectory / "plugin.json", f"Required plugin manifest '{PluginName}'")
    RequireFile(PluginDirectory / f"{PluginName}.dll", f"Required plugin DLL '{PluginName}'")

  RemovePath(PublishDirectory)
  BinariesDirectory = PublishDirectory / "Binaries"
  EnsureDirectory(BinariesDirectory)
  CopyFile(GameBinary, BinariesDirectory)
  (BinariesDirectory / GameBinary.name).replace(BinariesDirectory / f"{GameName}.exe")
  CopyFile(EngineBinary, BinariesDirectory)
  if EosBinary.is_file():
    CopyFile(EosBinary, BinariesDirectory)
  CopyFile(BootstrapBinary, PublishDirectory)
  (PublishDirectory / BootstrapBinary.name).replace(PublishDirectory / f"{GameName}.exe")
  if PluginsDirectory.is_dir():
    CopyDirectory(PluginsDirectory, BinariesDirectory / "Plugins")
  CopyDirectory(ResourcesDirectory, PublishDirectory / "Resources")
  if OnlineResourcesDirectory.is_dir():
    PublishedOnlineResourcesDirectory = PublishDirectory / "Resources-EOS"
    CopyDirectory(OnlineResourcesDirectory, PublishedOnlineResourcesDirectory)
    ConvertLevels(ConvertLevelsScript, PublishedOnlineResourcesDirectory)
