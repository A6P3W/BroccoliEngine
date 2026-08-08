"""Validate staged and packaged runtime artifacts."""

from pathlib import Path


def VerifyRuntime(
  OutputDirectory: Path, GameName: str, PublishDirectory: Path | None = None
) -> None:
  RequiredFiles = [
    OutputDirectory / "BroccoliEngine.dll",
  ]
  RequiredDirectories = [
    OutputDirectory / "Resources" / "Engine",
    OutputDirectory / "Resources" / GameName,
  ]
  MissingPaths = [PathValue for PathValue in RequiredFiles if not PathValue.is_file()]
  MissingPaths.extend(PathValue for PathValue in RequiredDirectories if not PathValue.is_dir())
  JsonFiles = sorted(OutputDirectory.glob("Resources/**/*.BLevel.json"))
  JsonFiles.extend(OutputDirectory.glob("Resources-EOS/**/*.BLevel.json"))

  if PublishDirectory is not None:
    PublishBinary = PublishDirectory / "Binaries" / f"{GameName}.exe"
    if not PublishBinary.is_file():
      MissingPaths.append(PublishBinary)
    JsonFiles.extend(PublishDirectory.glob("Resources/**/*.BLevel.json"))
    JsonFiles.extend(PublishDirectory.glob("Resources-EOS/**/*.BLevel.json"))

  Messages = []
  if MissingPaths:
    Messages.append("Missing required runtime artifacts:\n" + "\n".join(map(str, MissingPaths)))
  if JsonFiles:
    Messages.append("Unconverted .BLevel.json files remain:\n" + "\n".join(map(str, JsonFiles)))
  if Messages:
    raise ValueError("\n".join(Messages))
