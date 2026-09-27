# Reflection tests

Build the Debug configuration, then run the standalone checks from the repository root:

```powershell
./broccoli.bat build Debug
pwsh -NoProfile -File Tests/Reflection/Run-ReflectionTests.ps1
```

The test executable links against `Bin/x64/Debug/BroccoliEngine.dll`. It covers all six reflected
property types, clamping and metadata, callback old values and reentry, callback exception logging
and value retention, UTF-8 scalar limits, invalid types and values, inheritance, registry cleanup,
level serialization versions 1 through 3, and ExamplePlugin unload handling. The runner also checks
three expected compile failures for annotation type mismatch, callback argument mismatch, and
reversed bounds. Its `--live-actor-exit` pass leaves a test actor registered until process teardown
to check that PluginHost keeps the plugin DLL loaded while that actor remains alive.

The C++26 reflection compiler for this repository is GCC 16.2. On the development machine it is
located at `C:/msys64/mingw64/bin/g++.exe`.
