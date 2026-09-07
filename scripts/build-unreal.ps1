param(
    [Parameter(Mandatory = $true)]
    [string]$EngineRoot,
    [ValidateSet('Development', 'DebugGame', 'Shipping')]
    [string]$Configuration = 'Development'
)

$ErrorActionPreference = 'Stop'
$RepoRoot = Resolve-Path (Join-Path $PSScriptRoot '..')
$Project = Join-Path $RepoRoot 'game\WorldMakers.uproject'
$ExpectedVersionFile = Join-Path $RepoRoot 'game\UNREAL_ENGINE_VERSION'
$BuildVersionFile = Join-Path $EngineRoot 'Engine\Build\Build.version'
$BuildBat = Join-Path $EngineRoot 'Engine\Build\BatchFiles\Build.bat'

if (-not (Test-Path $ExpectedVersionFile)) {
    throw 'Missing game/UNREAL_ENGINE_VERSION certification lock.'
}
if (-not (Test-Path $BuildVersionFile)) {
    throw "Unreal Build.version not found under EngineRoot: $EngineRoot"
}
if (-not (Test-Path $BuildBat)) {
    throw "Unreal Build.bat not found under EngineRoot: $EngineRoot"
}

$ExpectedVersion = (Get-Content $ExpectedVersionFile -Raw).Trim()
$BuildVersion = Get-Content $BuildVersionFile -Raw | ConvertFrom-Json
$ActualVersion = "$($BuildVersion.MajorVersion).$($BuildVersion.MinorVersion).$($BuildVersion.PatchVersion)"
if ($ActualVersion -ne $ExpectedVersion) {
    throw "Unreal version mismatch. Expected $ExpectedVersion but runner has $ActualVersion."
}

Write-Host "Certified Unreal Engine version: $ActualVersion"
& $BuildBat WorldMakersEditor Win64 $Configuration $Project -WaitMutex -NoHotReloadFromIDE
if ($LASTEXITCODE -ne 0) {
    throw "Unreal build failed with exit code $LASTEXITCODE"
}
