"""Validate staged and packaged runtime artifacts."""

from pathlib import Path


def VerifyRuntime(
  OutputDirectory: Path,
  GameName: str,
  PublishDirectory: Path | None = None,
  RequiredPlugins: list[str] | None = None,
  MingwRuntime: list[Path] | None = None,
) -> None:
  RequiredPlugins = RequiredPlugins or []
  RequiredFiles = [
    OutputDirectory / "BroccoliEngine.dll",
  ]
  RequiredDirectories = [
    OutputDirectory / "Resources" / "Engine",
    OutputDirectory / "Resources" / GameName,
  ]
  MissingPaths = [PathValue for PathValue in RequiredFiles if not PathValue.is_file()]
  MissingPaths.extend(
    OutputDirectory / RuntimeBinary.name
    for RuntimeBinary in MingwRuntime or []
    if not (OutputDirectory / RuntimeBinary.name).is_file()
  )
  for PluginName in RequiredPlugins:
    PluginDirectory = OutputDirectory / "Plugins" / PluginName
    MissingPaths.extend(
      PathValue
      for PathValue in (PluginDirectory / "plugin.json", PluginDirectory / f"{PluginName}.dll")
      if not PathValue.is_file()
    )
    if MingwRuntime and not (OutputDirectory / f"{PluginName}.dll").is_file():
      MissingPaths.append(OutputDirectory / f"{PluginName}.dll")
  JsonFiles = sorted(OutputDirectory.glob("Resources/**/*.BLevel.json"))
  JsonFiles.extend(OutputDirectory.glob("Resources-EOS/**/*.BLevel.json"))

  if PublishDirectory is not None:
    PublishBinary = PublishDirectory / "Binaries" / f"{GameName}.exe"
    if not PublishBinary.is_file():
      MissingPaths.append(PublishBinary)
    MissingPaths.extend(
      PublishDirectory / "Binaries" / RuntimeBinary.name
      for RuntimeBinary in MingwRuntime or []
      if not (PublishDirectory / "Binaries" / RuntimeBinary.name).is_file()
    )
    for PluginName in RequiredPlugins:
      PluginDirectory = PublishDirectory / "Binaries" / "Plugins" / PluginName
      MissingPaths.extend(
        PathValue
        for PathValue in (PluginDirectory / "plugin.json", PluginDirectory / f"{PluginName}.dll")
        if not PathValue.is_file()
      )
      if MingwRuntime and not (PublishDirectory / "Binaries" / f"{PluginName}.dll").is_file():
        MissingPaths.append(PublishDirectory / "Binaries" / f"{PluginName}.dll")
    JsonFiles.extend(PublishDirectory.glob("Resources/**/*.BLevel.json"))
    JsonFiles.extend(PublishDirectory.glob("Resources-EOS/**/*.BLevel.json"))

  MissingPaths.extend(PathValue for PathValue in RequiredDirectories if not PathValue.is_dir())

  Messages = []
  if MissingPaths:
    Messages.append("Missing required runtime artifacts:\n" + "\n".join(map(str, MissingPaths)))
  if JsonFiles:
    Messages.append("Unconverted .BLevel.json files remain:\n" + "\n".join(map(str, JsonFiles)))
  if Messages:
    raise ValueError("\n".join(Messages))
