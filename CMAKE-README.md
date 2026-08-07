# CMake usage

CMake Presets separate repository settings from developer-local configuration.

- `Engine/CMakePresets.json` is shared and tracked by Git.
- `Engine/CMakeUserPresets.json` contains the local vcpkg toolchain path and is ignored by Git.
The shared preset intentionally provides no `CMAKE_TOOLCHAIN_FILE` default.
Each user must enter the absolute path in the ignored user preset.

Set `CMAKE_TOOLCHAIN_FILE` in `Engine/CMakeUserPresets.json` to the vcpkg installation on your machine.

Here is an example `CMakeUserPresets.json` file:

```json
{
  "version": 6,
  "configurePresets": [
    {
      "name": "local-windows-x64",
      "inherits": "windows-x64",
      "cacheVariables": {
        "CMAKE_TOOLCHAIN_FILE": "<Enter the absolute path to vcpkg.cmake>"
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
cd Engine

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

`EngineCore`, `EngineSide`, and `Editor` use
`file(GLOB_RECURSE ... CONFIGURE_DEPENDS)`. Generated games use the same approach
for `<ProjectName>/Source`. `Engine/ThirdParty` is intentionally
limited to the source files used by BroccoliEngine.
