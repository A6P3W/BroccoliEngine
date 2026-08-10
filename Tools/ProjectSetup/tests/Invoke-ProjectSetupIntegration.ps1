param(
  [string]$EngineRoot = (Resolve-Path (Join-Path $PSScriptRoot "..\..\..")).Path,
  [Parameter(Mandatory)]
  [string]$ToolchainFile,
  [string]$ProjectName = "BroccoliSetupTest",
  [switch]$KeepArtifacts,
  [switch]$SkipLaunch
)

$ErrorActionPreference = "Stop"
$TestRoot = Join-Path ([System.IO.Path]::GetTempPath()) (
  "BroccoliProjectSetup-" + [Guid]::NewGuid().ToString("N")
)

$ResetBackupRoot = $TestRoot + "-reset-backup"
function Invoke-NativeCommand {
  param(
    [Parameter(Mandatory)]
    [scriptblock]$Command,
    [Parameter(Mandatory)]
    [string]$Description
  )

  & $Command
  if ($LASTEXITCODE -ne 0) {
    throw "$Description failed with exit code $LASTEXITCODE"
  }
}

function Stop-TestProcess {
  param(
    [Parameter(Mandatory)]
    [System.Diagnostics.Process]$Process
  )

  if (-not $Process.HasExited) {
    Stop-Process -Id $Process.Id -Force
    $Process.WaitForExit()
  }
}

function Assert-LongRunningExecutable {
  param(
    [Parameter(Mandatory)]
    [string]$Executable,
    [Parameter(Mandatory)]
    [string]$WorkingDirectory
  )

  $Process = Start-Process -FilePath $Executable -WorkingDirectory $WorkingDirectory -WindowStyle Hidden -PassThru
  try {
    if ($Process.WaitForExit(5000)) {
      throw "$Executable exited early with code $($Process.ExitCode)"
    }
  } finally {
    Stop-TestProcess -Process $Process
  }
}

function Get-LatestLog {
  param(
    [Parameter(Mandatory)]
    [string]$LogsDirectory
  )

  $LogPath = Get-ChildItem -LiteralPath $LogsDirectory -File -Filter "*.log" |
    Sort-Object LastWriteTime -Descending |
    Select-Object -First 1 -ExpandProperty FullName
  if (-not $LogPath) {
    throw "No runtime log was created in $LogsDirectory"
  }
  return $LogPath
}

if (-not (Test-Path -LiteralPath $ToolchainFile -PathType Leaf)) {
  throw "ToolchainFile must point to vcpkg.cmake."
}
$ToolchainFile = (Resolve-Path -LiteralPath $ToolchainFile).Path
$EngineRoot = (Resolve-Path -LiteralPath $EngineRoot).Path
if (git -C $EngineRoot status --porcelain) {
  throw "EngineRoot must be clean so the clone contains the implementation under test."
}

try {
  Invoke-NativeCommand -Description "git clone" -Command {
    git -c "safe.directory=$EngineRoot/.git" clone --no-hardlinks $EngineRoot $TestRoot
  }

  Push-Location $TestRoot
  try {
    Invoke-NativeCommand -Description "in-place project setup" -Command {
      python SetupProject.py --name $ProjectName --in-place
    }

    $RequiredPaths = @(
      ".git",
      ".gitmodules",
      ".gitignore",
      ".broccoli-project.json",
      "CMakeLists.txt",
      "CMakePresets.json",
      "build.bat",
      "CMakeUserPresets.json",
      "run.bat",
      "BroccoliEngine",
      "$ProjectName\CMakeLists.txt",
      "BroccoliEngine\Engine\CMakeLists.txt",
      "$ProjectName\Source\Entry.cpp",
      "$ProjectName\Source\BasicGameplay\BasicGameplayGameMode.cpp",
      "$ProjectName\Resources\BasicGameplay.BLevel.json"
    )
    foreach ($RequiredPath in $RequiredPaths) {
      if (-not (Test-Path -LiteralPath $RequiredPath)) {
        throw "Required generated path is missing: $RequiredPath"
      }
    }

    $UserPresetsPath = Join-Path $TestRoot "CMakeUserPresets.json"
    $UserPresets = Get-Content -Raw -LiteralPath $UserPresetsPath
    if (-not $UserPresets.Contains("{YOUR_VCPKG_ROOT_DIRECTORY}")) {
      throw "CMakeUserPresets.json does not contain the VCPKG_ROOT placeholder."
    }
    $VcpkgRootPath = Split-Path (Split-Path (Split-Path $ToolchainFile))
    if (-not $VcpkgRootPath -or -not (Test-Path -LiteralPath $VcpkgRootPath)) {
      $VcpkgRootPath = Split-Path -Parent $ToolchainFile
    }
    $NormalizedVcpkgRoot = $VcpkgRootPath.Replace("\", "/")
    $UserPresets = $UserPresets.Replace(
      "{YOUR_VCPKG_ROOT_DIRECTORY}",
      $NormalizedVcpkgRoot
    )
    [System.IO.File]::WriteAllText(
      $UserPresetsPath,
      $UserPresets,
      [System.Text.UTF8Encoding]::new($false)
    )

    $Unresolved = Get-ChildItem -LiteralPath $ProjectName -File -Recurse |
      Select-String -Pattern "{PROJECT_NAME}", "{GAME_MODE_CLASS}" -SimpleMatch
    if ($Unresolved) {
      throw "Unresolved template placeholders remain."
    }

    $Stage = (git ls-files --stage BroccoliEngine | Out-String).Trim()
    if (-not $Stage.StartsWith("160000 ")) {
      throw "BroccoliEngine is not a gitlink: $Stage"
    }

    $SubmoduleStatus = (git submodule status BroccoliEngine | Select-Object -First 1) -as [string]
    if (-not $SubmoduleStatus -or -not $SubmoduleStatus.StartsWith(" ")) {
      throw "BroccoliEngine submodule is not initialized: $SubmoduleStatus"
    }

    Invoke-NativeCommand -Description "Debug build" -Command {
      & ".\build.bat" Debug
    }
    Invoke-NativeCommand -Description "Editor build" -Command {
      & ".\build.bat" Editor
    }

    if (-not $SkipLaunch) {
      $EditorExecutable = Join-Path $TestRoot "Bin\x64\Editor\$ProjectName-game.exe"
      Assert-LongRunningExecutable -Executable $EditorExecutable -WorkingDirectory (Split-Path $EditorExecutable)
      $EditorLog = Get-LatestLog -LogsDirectory (Join-Path $TestRoot "Logs")
      $EditorResourceRoot = $TestRoot.Replace("\", "/") + "/$ProjectName/Resources/"
      if (-not (Select-String -LiteralPath $EditorLog -Pattern $EditorResourceRoot -SimpleMatch)) {
        throw "Editor Game resource root was not resolved as expected."
      }

      $PublishRoot = Join-Path $TestRoot "Publish\Debug"
      $DebugExecutable = Join-Path $PublishRoot "Binaries\$ProjectName.exe"
      Assert-LongRunningExecutable -Executable $DebugExecutable -WorkingDirectory $PublishRoot
      $DebugLog = Get-LatestLog -LogsDirectory (Join-Path $PublishRoot "Logs")
      $PackagedLevel = "Resources/$ProjectName/BasicGameplay.BLevel"
      if (-not (Select-String -LiteralPath $DebugLog -Pattern $PackagedLevel -SimpleMatch)) {
        throw "Packaged BasicGameplay level was not loaded."
      }
    }

    Invoke-NativeCommand -Description "project reset" -Command {
      python BroccoliEngine\SetupProject.py --reset-project --backup-output $ResetBackupRoot
    }

    $ResetRequiredPaths = @(
      ".git",
      "Engine\CMakeLists.txt",
      "Engine\CMakePresets.json",
      "SetupProject.py"
    )
    foreach ($RequiredPath in $ResetRequiredPaths) {
      if (-not (Test-Path -LiteralPath $RequiredPath)) {
        throw "Reset engine path is missing: $RequiredPath"
      }
    }
    if (Test-Path -LiteralPath "BroccoliEngine") {
      throw "BroccoliEngine directory remained after reset."
    }
    if (-not (Test-Path -LiteralPath (Join-Path $ResetBackupRoot $ProjectName))) {
      throw "Game workspace was not preserved in the reset backup."
    }
    if (git status --porcelain) {
      throw "Restored engine repository is not clean."
    }
    Write-Output "Project setup integration test passed: $TestRoot"
  } finally {
    Pop-Location

  }
} finally {
  if (-not $KeepArtifacts) {
    $TemporaryRoot = [System.IO.Path]::GetFullPath(
      [System.IO.Path]::GetTempPath()
    ).TrimEnd("\")
    foreach ($ArtifactRoot in @($TestRoot, $ResetBackupRoot)) {
      if (-not (Test-Path -LiteralPath $ArtifactRoot)) {
        continue
      }
      $ResolvedArtifactRoot = (Resolve-Path -LiteralPath $ArtifactRoot).Path
      if (
        -not $ResolvedArtifactRoot.StartsWith(
          $TemporaryRoot + "\BroccoliProjectSetup-",
          [StringComparison]::OrdinalIgnoreCase
        )
      ) {
        throw "Refusing to clean unexpected test path: $ResolvedArtifactRoot"
      }
      Remove-Item -LiteralPath $ResolvedArtifactRoot -Recurse -Force
    }
  }
}
