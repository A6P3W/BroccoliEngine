param([switch]$EOSLogin, [switch]$EOSNoCredentials, [string]$Configuration = "Debug")
$ErrorActionPreference = "Stop"
$RepositoryRoot = (Resolve-Path (Join-Path $PSScriptRoot "../..")).Path
$Cache = Join-Path $RepositoryRoot "build/windows-x64-gcc26/CMakeCache.txt"
$CompilerEntry = Get-Content -LiteralPath $Cache | Where-Object { $_ -match '^CMAKE_CXX_COMPILER:[^=]+=(.+)$' } | Select-Object -First 1
if ($CompilerEntry -notmatch '^CMAKE_CXX_COMPILER:[^=]+=(.+)$') { throw "Build Debug first." }
$Compiler = $Matches[1]
$Output = Join-Path $RepositoryRoot "Bin/x64/$Configuration"
$env:PATH = "$(Split-Path -Parent $Compiler);$Output;$env:PATH"
$CompilerArguments = @("-std=c++26", "-freflection")
Get-ChildItem (Join-Path $RepositoryRoot "Engine/EngineCore") -Directory | ForEach-Object {
  $Public = Join-Path $_.FullName "Public"
  if (Test-Path -LiteralPath $Public) { $CompilerArguments += "-I$Public" }
}
$CompilerArguments += "-I$(Join-Path $RepositoryRoot 'Engine/ThirdParty')"
$CompilerArguments += "-L$Output"
$CompilerArguments += "-lBroccoliEngine"
& $Compiler (Join-Path $PSScriptRoot "LifecyclePlugin.cpp") @CompilerArguments -shared "-o$(Join-Path $Output 'LifecyclePlugin.dll')"
if ($LASTEXITCODE -ne 0) { throw "Lifecycle plugin compilation failed." }
foreach ($Fixture in @(@("BadVersionPlugin", "TEST_BAD_API"), @("ZFailurePlugin", "TEST_FAIL_LOAD"), @("WrongNamePlugin", "TEST_WRONG_NAME"))) {
  & $Compiler (Join-Path $PSScriptRoot "LifecyclePlugin.cpp") @CompilerArguments -shared "-D$($Fixture[1])" "-o$(Join-Path $Output ($Fixture[0] + '.dll'))"
  if ($LASTEXITCODE -ne 0) { throw "Plugin fixture compilation failed: $($Fixture[0])" }
}
if ($EOSLogin -or $EOSNoCredentials) {
  $CompilerArguments += "-DTEST_EOS_PLUGIN"
  if ($EOSNoCredentials) { $CompilerArguments += "-DTEST_EOS_NO_CREDENTIALS" }
  $CompilerArguments += "-I$(Join-Path $RepositoryRoot 'Engine/Plugins/EOSPlugin/Source/Public')"
  $CompilerArguments += "-lEOSPlugin"
}
$Executable = Join-Path $Output "NetworkTests.exe"
& $Compiler (Join-Path $PSScriptRoot "NetworkTests.cpp") @CompilerArguments "-o$Executable"
if ($LASTEXITCODE -ne 0) { throw "Network tests compilation failed." }
& $Executable
if ($LASTEXITCODE -ne 0) { throw "Network tests failed." }
