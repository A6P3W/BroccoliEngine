"""Validate staged and packaged runtime artifacts."""

from pathlib import Path

from .runtime_plugins import LoadRuntimePlugins, VerifyPlugins


def VerifyRuntime(
  OutputDirectory: Path,
  GameName: str,
  PublishDirectory: Path | None = None,
  PluginDependencies: list[Path] | None = None,
  MingwRuntime: list[Path] | None = None,
  Configuration: str | None = None,
) -> None:
  VerifyPlugins(OutputDirectory, PluginDependencies or [])
  Directories = [OutputDirectory]
  if PublishDirectory is not None:
    Binaries = PublishDirectory / "Binaries"
    VerifyPlugins(Binaries, PluginDependencies or [])
    if LoadRuntimePlugins(Binaries) != LoadRuntimePlugins(OutputDirectory):
      raise ValueError("Packaged plugin selection does not match staged selection")
    Directories.append(Binaries)
    if (
      not (Binaries / f"{GameName}.exe").is_file()
      or not (PublishDirectory / f"{GameName}.exe").is_file()
    ):
      raise ValueError("Packaged executable is missing")
  for Directory in Directories:
    for Name in ["BroccoliEngine.dll", *[Binary.name for Binary in MingwRuntime or []]]:
      if not (Directory / Name).is_file():
        raise ValueError(f"Missing required runtime artifact: {Directory / Name}")
  if Configuration is None or Configuration.casefold() != "editor":
    for Root in [OutputDirectory, *([PublishDirectory] if PublishDirectory else [])]:
      for Name in ["Engine", GameName]:
        if not (Root / "Resources" / Name).is_dir():
          raise ValueError(f"Missing resource directory: {Name}")
      if list(Root.glob("Resources/**/*.BLevel.json")):
        raise ValueError("Unconverted .BLevel.json files remain")
