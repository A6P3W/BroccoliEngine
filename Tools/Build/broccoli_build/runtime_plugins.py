"""Validate build-time plugin selections and manage their single runtime copies."""

import json
import re
from pathlib import Path

from .common import CopyFile, RemovePath


def ValidatePluginNames(Names: object) -> list[str]:
  if not isinstance(Names, list):
    raise ValueError("required_plugins must be an array")
  Seen: set[str] = set()
  for Name in Names:
    if not isinstance(Name, str) or not re.fullmatch(r"[A-Za-z0-9_-]+", Name) or Name in Seen:
      raise ValueError("Invalid or duplicate plugin name")
    Seen.add(Name)
  return Names


def LoadRuntimePlugins(Directory: Path) -> list[str]:
  Config = json.loads((Directory / "plugins.json").read_text(encoding="utf-8"))
  if (
    not isinstance(Config, dict)
    or type(Config.get("schema_version")) is not int
    or Config["schema_version"] != 1
  ):
    raise ValueError("Invalid plugins.json schema")
  return ValidatePluginNames(Config.get("required_plugins"))


def GenerateRuntimePlugins(Directory: Path, Names: list[str]) -> None:
  ValidatePluginNames(Names)
  Directory.mkdir(parents=True, exist_ok=True)
  (Directory / "plugins.json").write_text(
    json.dumps({"schema_version": 1, "required_plugins": Names}, indent=2) + "\n",
    encoding="utf-8",
    newline="\n",
  )


def StagePlugins(Directory: Path, Dependencies: list[Path]) -> None:
  Names = LoadRuntimePlugins(Directory)
  CurrentFiles = [f"{Name}.dll" for Name in Names] + [
    Dependency.name for Dependency in Dependencies
  ]
  OwnershipPath = Directory / ".plugin-artifacts.json"
  PreviousFiles = (
    json.loads(OwnershipPath.read_text(encoding="utf-8")) if OwnershipPath.is_file() else []
  )
  for Name in PreviousFiles:
    if (
      not isinstance(Name, str)
      or Path(Name).name != Name
      or "/" in Name
      or "\\" in Name
      or not Name.endswith(".dll")
    ):
      raise ValueError("Invalid managed plugin artifact name")
    if Name not in CurrentFiles:
      RemovePath(Directory / Name)
  # Remove only the legacy plugin directories owned by the build tools.
  LegacyDirectory = Directory / "Plugins"
  for Manifest in LegacyDirectory.glob("*/plugin.json"):
    RemovePath(Manifest.parent)
  for Dependency in Dependencies:
    CopyFile(Dependency, Directory)
  OwnershipPath.write_text(json.dumps(CurrentFiles), encoding="utf-8", newline="\n")


def VerifyPlugins(Directory: Path, Dependencies: list[Path]) -> None:
  for Name in LoadRuntimePlugins(Directory):
    if not (Directory / f"{Name}.dll").is_file():
      raise ValueError(f"Required plugin DLL missing: {Name}")
    if (Directory / "Plugins" / Name).exists():
      raise ValueError(f"Duplicate plugin directory: {Name}")
  for Dependency in Dependencies:
    if not (Directory / Dependency.name).is_file():
      raise ValueError(f"Required plugin dependency missing: {Dependency.name}")
  if list(Directory.rglob("plugin.json")):
    raise ValueError("Legacy plugin.json remains")
