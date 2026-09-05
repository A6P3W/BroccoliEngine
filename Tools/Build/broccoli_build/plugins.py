"""Generate CMake plugin selection from the project settings."""

from __future__ import annotations

import json
import shutil
from pathlib import Path

PROJECT_SETTINGS_FILE_NAME = ".broccoli-project.json"
GENERATED_PLUGINS_FILE = Path("Intermediate") / "Generated" / "Plugins.cmake"
EXAMPLE_PLUGIN_NAME = "ExamplePlugin"
CONFIGURATIONS = ("Debug", "Editor", "Release")


def LoadExamplePluginEnabled(ProjectDirectory: Path) -> bool:
  SettingsPath = ProjectDirectory / PROJECT_SETTINGS_FILE_NAME
  if not SettingsPath.is_file():
    return True

  try:
    Settings = json.loads(SettingsPath.read_text(encoding="utf-8"))
  except (OSError, json.JSONDecodeError) as Error:
    raise ValueError(f"Could not read project settings '{SettingsPath}': {Error}") from Error

  if not isinstance(Settings, dict):
    raise ValueError(f"Project settings must contain a JSON object: {SettingsPath}")

  Plugins = Settings.get("plugins", {})
  if not isinstance(Plugins, dict):
    raise ValueError(f"Project setting 'plugins' must be an object: {SettingsPath}")

  Enabled = Plugins.get(EXAMPLE_PLUGIN_NAME, True)
  if not isinstance(Enabled, bool):
    raise ValueError(
      f"Project setting 'plugins.{EXAMPLE_PLUGIN_NAME}' must be true or false: {SettingsPath}"
    )
  return Enabled


def GeneratePluginsCmake(ProjectDirectory: Path) -> bool:
  Enabled = LoadExamplePluginEnabled(ProjectDirectory)
  GeneratedFile = ProjectDirectory / GENERATED_PLUGINS_FILE
  Content = f"set(BROCCOLI_BUILD_EXAMPLE_PLUGIN {'ON' if Enabled else 'OFF'})\n"

  if GeneratedFile.is_file() and GeneratedFile.read_text(encoding="utf-8") == Content:
    return False

  GeneratedFile.parent.mkdir(parents=True, exist_ok=True)
  GeneratedFile.write_text(Content, encoding="utf-8", newline="\n")
  return True


def RemoveDisabledExamplePluginArtifacts(ProjectDirectory: Path) -> None:
  if LoadExamplePluginEnabled(ProjectDirectory):
    return

  for Configuration in CONFIGURATIONS:
    ArtifactDirectory = (
      ProjectDirectory / "Bin" / "x64" / Configuration / "Plugins" / EXAMPLE_PLUGIN_NAME
    )
    if ArtifactDirectory.is_dir():
      shutil.rmtree(ArtifactDirectory)
