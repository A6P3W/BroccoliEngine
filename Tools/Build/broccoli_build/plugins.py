"""Generate CMake plugin selection from the project settings."""

from __future__ import annotations

import json
import shutil
from pathlib import Path

PROJECT_SETTINGS_FILE_NAME = ".broccoli-project.json"
GENERATED_PLUGINS_FILE = Path("Intermediate") / "Generated" / "Plugins.cmake"
EXAMPLE_PLUGIN_NAME = "ExamplePlugin"
CONFIGURATIONS = ("Debug", "Editor", "Release")


def LoadPluginConfigurations(ProjectDirectory: Path) -> dict[str, set[str]]:
  SettingsPath = ProjectDirectory / PROJECT_SETTINGS_FILE_NAME
  if not SettingsPath.is_file():
    raise ValueError(f"Project settings do not exist: {SettingsPath}")

  try:
    Settings = json.loads(SettingsPath.read_text(encoding="utf-8"))
  except (OSError, json.JSONDecodeError) as Error:
    raise ValueError(f"Could not read project settings '{SettingsPath}': {Error}") from Error

  if not isinstance(Settings, dict):
    raise ValueError(f"Project settings must contain a JSON object: {SettingsPath}")

  Plugins = Settings.get("plugins")
  if not isinstance(Plugins, list):
    raise ValueError(f"Project setting 'plugins' must be an array: {SettingsPath}")

  PluginConfigurations: dict[str, set[str]] = {}
  for Plugin in Plugins:
    if not isinstance(Plugin, dict):
      raise ValueError(f"Each plugin setting must be an object: {SettingsPath}")

    Name = Plugin.get("name")
    if not isinstance(Name, str) or not Name.strip():
      raise ValueError(f"Plugin setting 'name' must be a non-empty string: {SettingsPath}")
    if Name in PluginConfigurations:
      raise ValueError(f"Plugin '{Name}' is defined more than once: {SettingsPath}")
    if "enabled" in Plugin:
      raise ValueError(f"Plugin setting 'enabled' is not supported: {SettingsPath}")

    PluginConfigurationList = Plugin.get("configurations")
    if not isinstance(PluginConfigurationList, list):
      raise ValueError(f"Plugin '{Name}' setting 'configurations' must be an array: {SettingsPath}")

    Configurations: set[str] = set()
    for Configuration in PluginConfigurationList:
      if not isinstance(Configuration, str):
        raise ValueError(f"Plugin '{Name}' configuration must be a string: {SettingsPath}")
      if Configuration not in CONFIGURATIONS:
        raise ValueError(
          f"Plugin '{Name}' has an unknown configuration '{Configuration}': {SettingsPath}"
        )
      if Configuration in Configurations:
        raise ValueError(
          f"Plugin '{Name}' configuration '{Configuration}' is duplicated: {SettingsPath}"
        )
      Configurations.add(Configuration)
    PluginConfigurations[Name] = Configurations

  return PluginConfigurations


def FormatCmakeList(VariableName: str, Plugins: list[str]) -> str:
  if not Plugins:
    return f"set({VariableName})"
  return "\n".join([f"set({VariableName}", *(f"  {Plugin}" for Plugin in Plugins), ")"])


def CreatePluginsCmake(PluginConfigurations: dict[str, set[str]]) -> str:
  Sections = [
    FormatCmakeList("BROCCOLI_PLUGINS", sorted(PluginConfigurations)),
  ]
  for Configuration in CONFIGURATIONS:
    EnabledPlugins = sorted(
      Name for Name, Configurations in PluginConfigurations.items() if Configuration in Configurations
    )
    Sections.append(FormatCmakeList(f"BROCCOLI_PLUGINS_{Configuration.upper()}", EnabledPlugins))
  return "\n\n".join(Sections) + "\n"


def GeneratePluginsCmake(
  ProjectDirectory: Path, PluginConfigurations: dict[str, set[str]] | None = None
) -> bool:
  if PluginConfigurations is None:
    PluginConfigurations = LoadPluginConfigurations(ProjectDirectory)
  GeneratedFile = ProjectDirectory / GENERATED_PLUGINS_FILE
  Content = CreatePluginsCmake(PluginConfigurations)

  if GeneratedFile.is_file() and GeneratedFile.read_text(encoding="utf-8") == Content:
    return False

  GeneratedFile.parent.mkdir(parents=True, exist_ok=True)
  GeneratedFile.write_text(Content, encoding="utf-8", newline="\n")
  return True


def RemoveDisabledExamplePluginArtifacts(
  ProjectDirectory: Path, PluginConfigurations: dict[str, set[str]] | None = None
) -> None:
  if PluginConfigurations is None:
    PluginConfigurations = LoadPluginConfigurations(ProjectDirectory)
  EnabledConfigurations = PluginConfigurations.get(EXAMPLE_PLUGIN_NAME, set())

  for Configuration in CONFIGURATIONS:
    if Configuration in EnabledConfigurations:
      continue
    ArtifactDirectory = (
      ProjectDirectory / "Bin" / "x64" / Configuration / "Plugins" / EXAMPLE_PLUGIN_NAME
    )
    if ArtifactDirectory.is_dir():
      shutil.rmtree(ArtifactDirectory)
