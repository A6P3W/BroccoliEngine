$ErrorActionPreference = "Stop"

$RepositoryRoot = (Resolve-Path (Join-Path $PSScriptRoot "../..")).Path
$CMakeCache = Join-Path $RepositoryRoot "build/windows-x64-gcc26/CMakeCache.txt"
$Compiler = $null
if (Test-Path -LiteralPath $CMakeCache) {
  $CompilerEntry = Get-Content -LiteralPath $CMakeCache |
    Where-Object { $_ -match '^CMAKE_CXX_COMPILER:[^=]+=(.+)$' } |
    Select-Object -First 1
  if ($CompilerEntry -match '^CMAKE_CXX_COMPILER:[^=]+=(.+)$') {
    $Compiler = $Matches[1]
  }
}
if (-not $Compiler) {
  $CompilerCommand = Get-Command g++ -ErrorAction SilentlyContinue
  if ($CompilerCommand) {
    $Compiler = $CompilerCommand.Source
  }
}
if (-not $Compiler) {
  throw "GCC compiler was not found in CMakeCache.txt or PATH."
}
if (-not [System.IO.Path]::IsPathRooted($Compiler)) {
  $Compiler = (Get-Command $Compiler -ErrorAction Stop).Source
}
$Compiler = [System.IO.Path]::GetFullPath($Compiler)
$CompilerDirectory = Split-Path -Parent $Compiler
$DebugDirectory = Join-Path $RepositoryRoot "Bin/x64/Debug"
$TestExecutable = Join-Path $DebugDirectory "ReflectionTests.exe"
$TestTempDirectory = Join-Path ([System.IO.Path]::GetTempPath()) `
  ("BroccoliReflectionCompileTests-" + [guid]::NewGuid().ToString("N"))
$TemporaryFiles = [System.Collections.Generic.List[string]]::new()
$IncludeDirectories = @(
  "Engine/EngineCore",
  "Engine/EngineCore/Core/Public",
  "Engine/EngineCore/Plugin/Public",
  "Engine/EngineCore/Systems/Public",
  "Engine/EngineCore/Reflection/Public",
  "Engine/EngineCore/Components/Public",
  "Engine/EngineCore/Network/Public",
  "Engine/EngineCore/Automation/Public",
  "Engine/EngineCore/GameFramework/Public",
  "Engine/EngineCore/Utils/Public",
  "Engine/EngineCore/Online/Public",
  "Engine/EngineSide",
  "Engine/EngineSide/Default/Public",
  "Engine/Editor/Public",
  "Engine/ThirdParty"
)
$CompilerArguments = @("-std=c++26", "-freflection")
foreach ($IncludeDirectory in $IncludeDirectories) {
  $CompilerArguments += "-I$IncludeDirectory"
}

if (-not (Test-Path -LiteralPath $Compiler)) {
  throw "GCC 16 compiler was not found: $Compiler"
}
if (-not (Test-Path -LiteralPath (Join-Path $DebugDirectory "BroccoliEngine.dll"))) {
  throw "Build Debug first so Bin/x64/Debug/BroccoliEngine.dll exists."
}

$env:PATH = "$CompilerDirectory;$DebugDirectory;$env:PATH"
New-Item -ItemType Directory -Path $TestTempDirectory | Out-Null
Push-Location $RepositoryRoot
try {
  & $Compiler @CompilerArguments `
    "-IEngine/EngineCore/Automation/Public" `
    "Tests/Reflection/FunctionReflectionProbe.cpp" `
    "-o$(Join-Path $TestTempDirectory 'FunctionReflectionProbe.exe')"
  if ($LASTEXITCODE -ne 0) {
    throw "Function reflection probe compilation failed with exit code $LASTEXITCODE."
  }
  & (Join-Path $TestTempDirectory "FunctionReflectionProbe.exe")
  if ($LASTEXITCODE -ne 0) {
    throw "Function reflection probe failed with exit code $LASTEXITCODE."
  }

  & $Compiler @CompilerArguments "Tests/Reflection/ReflectionTests.cpp" `
    "-L$DebugDirectory" "-lBroccoliEngine" "-o$TestExecutable"
  if ($LASTEXITCODE -ne 0) {
    throw "Reflection test compilation failed with exit code $LASTEXITCODE."
  }

  & $TestExecutable
  if ($LASTEXITCODE -ne 0) {
    throw "Reflection runtime tests failed with exit code $LASTEXITCODE."
  }

  & $TestExecutable --live-actor-exit
  if ($LASTEXITCODE -ne 0) {
    throw "Plugin live-actor process-exit regression failed with exit code $LASTEXITCODE."
  }

  $RegistrationCompileSource =
    "Tests/Reflection/CompileSuccess/RegistrationOnlyIncludeComponent.cpp"
  $RegistrationCompileObject = Join-Path $TestTempDirectory "RegistrationOnlyIncludeComponent.o"
  $TemporaryFiles.Add($RegistrationCompileObject)
  & $Compiler @CompilerArguments -c $RegistrationCompileSource "-o$RegistrationCompileObject"
  if ($LASTEXITCODE -ne 0) {
    throw "Component registration include-contract compilation failed with exit code $LASTEXITCODE."
  }
  Write-Output "Component registration include contract verified."

  $CompileFailureCases = @(
    [pscustomobject]@{
      Source = "AnnotationTypeMismatch.cpp"
      Expected = "Unsupported editor property member type"
    },
    [pscustomobject]@{
      Source = "InvalidMetadataForInt.cpp"
      Expected = "MaxLength is only valid for string properties"
    },
    [pscustomobject]@{
      Source = "InvalidMetadataForBool.cpp"
      Expected = "Numeric metadata is only valid for int and float properties"
    },
    [pscustomobject]@{
      Source = "InvalidPathFilterForString.cpp"
      Expected = "PathFilter is only valid for FPath properties"
    },
    [pscustomobject]@{
      Source = "FractionalIntRange.cpp"
      Expected = "Integer property metadata must be finite, integral, and within int range"
    },
    [pscustomobject]@{
      Source = "CallbackArgumentMismatch.cpp"
      Expected = "OnEditorChanged must be void(T OldValue)"
    },
    [pscustomobject]@{
      Source = "ReversedRange.cpp"
      Expected = "Reversed Min/Max"
    },
    [pscustomobject]@{
      Source = "ReversedSliderRange.cpp"
      Expected = "Reversed SliderMin/SliderMax"
    },
    [pscustomobject]@{
      Source = "PrivateDirectAccess.cpp"
      Expected = "is private within this context"
    },
    [pscustomobject]@{
      Source = "AbstractComponentFactory.cpp"
      Expected = "!std::is_abstract_v<T>"
    },
    [pscustomobject]@{
      Source = "NonDefaultComponentFactory.cpp"
      Expected = "std::is_default_constructible_v<T>"
    }
  )
  foreach ($Case in $CompileFailureCases) {
    $SourcePath = "Tests/Reflection/CompileFailures/$($Case.Source)"
    $ObjectPath = Join-Path $TestTempDirectory "$($Case.Source).o"
    $LogPath = Join-Path $TestTempDirectory "$($Case.Source).log"
    $TemporaryFiles.Add($ObjectPath)
    $TemporaryFiles.Add($LogPath)
    & $Compiler @CompilerArguments -c $SourcePath "-o$ObjectPath" *> $LogPath
    $CompileExitCode = $LASTEXITCODE
    $Diagnostics = Get-Content -LiteralPath $LogPath -Raw
    Remove-Item -LiteralPath $ObjectPath, $LogPath -Force -ErrorAction SilentlyContinue
    if ($CompileExitCode -eq 0) {
      throw "$SourcePath unexpectedly compiled successfully."
    }
    if ($Diagnostics -notmatch [regex]::Escape($Case.Expected)) {
      throw "$SourcePath failed for an unexpected reason. Expected diagnostic: $($Case.Expected)"
    }
    Write-Output "Expected compile failure verified: $($Case.Source)"
  }
} finally {
  Pop-Location
  foreach ($TemporaryFile in $TemporaryFiles) {
    Remove-Item -LiteralPath $TemporaryFile -Force -ErrorAction SilentlyContinue
  }
  $CanonicalTempDirectory = (Resolve-Path -LiteralPath $TestTempDirectory).Path
  $CanonicalTempParent = (Resolve-Path -LiteralPath (Split-Path -Parent $TestTempDirectory)).Path
  if (
    [System.IO.Path]::GetFullPath($CanonicalTempDirectory).Equals(
      [System.IO.Path]::GetFullPath($TestTempDirectory),
      [System.StringComparison]::OrdinalIgnoreCase
    ) -and
    [System.IO.Path]::GetFullPath((Split-Path -Parent $CanonicalTempDirectory)).Equals(
      [System.IO.Path]::GetFullPath($CanonicalTempParent),
      [System.StringComparison]::OrdinalIgnoreCase
    ) -and
    (Split-Path -Leaf $CanonicalTempDirectory).StartsWith(
      "BroccoliReflectionCompileTests-", [System.StringComparison]::Ordinal
    )
  ) {
    Remove-Item -LiteralPath $CanonicalTempDirectory -Force -Recurse
  } else {
    throw "Refusing to remove an unexpected reflection compile-test temp directory."
  }
}

exit 0
