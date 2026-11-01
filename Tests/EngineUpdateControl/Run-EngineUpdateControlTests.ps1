$ErrorActionPreference = "Stop"

$RepositoryRoot = (Resolve-Path (Join-Path $PSScriptRoot "../..")).Path
$CMakeCache = Join-Path $RepositoryRoot "build/windows-x64-gcc26/CMakeCache.txt"
if (-not (Test-Path -LiteralPath $CMakeCache)) {
  throw "Build Debug first so CMakeCache.txt identifies the C++26 compiler."
}
$CompilerEntry = Get-Content -LiteralPath $CMakeCache |
  Where-Object { $_ -match '^CMAKE_CXX_COMPILER:[^=]+=(.+)$' } |
  Select-Object -First 1
if ($CompilerEntry -notmatch '^CMAKE_CXX_COMPILER:[^=]+=(.+)$') {
  throw "The configured C++ compiler was not found in CMakeCache.txt."
}
$Compiler = $Matches[1]
$TestDirectory = Join-Path $RepositoryRoot "Bin/x64/Debug"
$TestExecutable = Join-Path $TestDirectory "EngineUpdateControlTests.exe"

$IncludeDirectories = @(
  "Engine/EngineCore/Core/Public",
  "Engine/EngineCore/Automation/Public",
  "Engine/ThirdParty"
)
$CompilerArguments = @("-std=c++26", "-DBROCCOLI_ENGINE_BUILD")
foreach ($IncludeDirectory in $IncludeDirectories) {
  $CompilerArguments += "-I$IncludeDirectory"
}

$env:PATH = "$(Split-Path -Parent $Compiler);$TestDirectory;$env:PATH"
Push-Location $RepositoryRoot
try {
  & $Compiler @CompilerArguments `
    "Tests/EngineUpdateControl/EngineUpdateControlTests.cpp" `
    "Engine/EngineCore/Core/Private/EngineUpdateControl.cpp" `
    "Engine/EngineCore/Automation/Private/Runtime/Types.cpp" `
    "-o$TestExecutable"
  if ($LASTEXITCODE -ne 0) {
    throw "Engine update control test compilation failed with exit code $LASTEXITCODE."
  }

  & $TestExecutable
  if ($LASTEXITCODE -ne 0) {
    throw "Engine update control tests failed with exit code $LASTEXITCODE."
  }
  Write-Output "Engine update control tests passed."
} finally {
  Pop-Location
  if (Test-Path -LiteralPath $TestExecutable) {
    Remove-Item -LiteralPath $TestExecutable -Force
  }
}
