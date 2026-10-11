param([switch]$IncludeEditorAndRelease)
$ErrorActionPreference = "Stop"
$RepositoryRoot = (Resolve-Path (Join-Path $PSScriptRoot "../..")).Path
$SettingsPath = Join-Path $RepositoryRoot ".broccoli-project.json"
$OriginalSettings = [IO.File]::ReadAllText($SettingsPath)
$SDKPath = [IO.Path]::GetFullPath((Join-Path $RepositoryRoot "Engine/Plugins/EOSPlugin/ThirdParty/EOS/SDK"))
$HiddenSDKPath = [IO.Path]::GetFullPath((Join-Path $RepositoryRoot "Intermediate/CoreOnlyHiddenSDK"))
foreach ($Target in @($SDKPath, $HiddenSDKPath)) {
  if (-not $Target.StartsWith($RepositoryRoot + [IO.Path]::DirectorySeparatorChar, [StringComparison]::OrdinalIgnoreCase)) { throw "SDK move target is outside the repository." }
}
if (Test-Path -LiteralPath $HiddenSDKPath) { throw "Hidden SDK path is already occupied." }
$SDKMoved = $false
Push-Location $RepositoryRoot
try {
  $Settings = $OriginalSettings | ConvertFrom-Json
  foreach ($Plugin in $Settings.plugins) {
    if ($Plugin.name -eq "EOSPlugin") { $Plugin.configurations = @() }
  }
  [IO.File]::WriteAllText($SettingsPath, ($Settings | ConvertTo-Json -Depth 10) + "`n", [Text.UTF8Encoding]::new($false))
  New-Item -ItemType Directory -Path (Split-Path -Parent $HiddenSDKPath) -Force | Out-Null
  if (Test-Path -LiteralPath $SDKPath) {
    Move-Item -LiteralPath $SDKPath -Destination $HiddenSDKPath
    $SDKMoved = $true
  }
  $Configurations = @("Debug")
  if ($IncludeEditorAndRelease) { $Configurations += @("Editor", "Release") }
  foreach ($Configuration in $Configurations) {
    & ./broccoli.bat build $Configuration --reconfigure
    if ($LASTEXITCODE -ne 0) { throw "Core-only $Configuration build failed." }
    $Output = Join-Path $RepositoryRoot "Bin/x64/$Configuration"
    foreach ($Name in @("EOSPlugin.dll", "EOSSDK-Win64-Shipping.dll")) {
      if (Test-Path -LiteralPath (Join-Path $Output $Name)) { throw "Disabled EOS artifact remained: $Name" }
    }
    & pwsh -NoProfile -File Tests/Network/Run-NetworkTests.ps1 -Configuration $Configuration
    if ($LASTEXITCODE -ne 0) { throw "Core-only $Configuration network tests failed." }
  }
} finally {
  if ($SDKMoved) { Move-Item -LiteralPath $HiddenSDKPath -Destination $SDKPath }
  [IO.File]::WriteAllText($SettingsPath, $OriginalSettings, [Text.UTF8Encoding]::new($false))
  Pop-Location
}
