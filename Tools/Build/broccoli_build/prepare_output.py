"""Clean generated runtime files without terminating running processes."""

from pathlib import Path

from .common import RemovePath


def PrepareOutput(OutputDirectory: Path) -> None:
  for Name in ("Logs", "Resources", "Log.txt"):
    RemovePath(OutputDirectory / Name)
