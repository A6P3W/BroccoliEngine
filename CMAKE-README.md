# CMake usage

CMake Presets separate repository settings from developer-local configuration.

- `CMakePresets.json` is shared and tracked by Git.
- `CMakeUserPresets.json` contains the local vcpkg toolchain path and is ignored by Git.

Set `CMAKE_TOOLCHAIN_FILE` in `CMakeUserPresets.json` to the vcpkg installation on your machine.

Here is an example `CMakeUserPresets.json` file:

```json
{
  "version": 6,
  "configurePresets": [
    {
      "name": "local-windows-x64",
      "inherits": "windows-x64",
      "cacheVariables": {
        "CMAKE_TOOLCHAIN_FILE": "C:/path/to/vcpkg/scripts/buildsystems/vcpkg.cmake"
      }
    }
  ],
  "buildPresets": [
    {
      "name": "local-debug",
      "inherits": "debug",
      "configurePreset": "local-windows-x64"
    },
    {
      "name": "local-editor",
      "inherits": "editor",
      "configurePreset": "local-windows-x64"
    },
    {
      "name": "local-release",
      "inherits": "release",
      "configurePreset": "local-windows-x64"
    }
  ]
}
```

## Command line

Configure with the local preset:

```powershell
cmake --preset local-windows-x64
```

Build one of the available configurations:

```powershell
cmake --build --preset local-debug
cmake --build --preset local-editor
cmake --build --preset local-release
```

## VS Code

With CMake Tools, select the following presets:

- Configure Preset: `Local Windows x64`
- Build Preset: `Local Debug`, `Local Editor`, or `Local Release`

## Source discovery

`EngineCore`, `EngineSide`, `Editor`, and `Launcher/Source` use
`file(GLOB_RECURSE ... CONFIGURE_DEPENDS)`. `Engine/ThirdParty` is intentionally
limited to the source files used by BroccoliEngine.
