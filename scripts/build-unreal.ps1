param(
    [Parameter(Mandatory = $true)]
    [string]$EngineRoot,
    [ValidateSet('Development', 'DebugGame', 'Shipping')]
    [string]$Configuration = 'Development'
)

$ErrorActionPreference = 'Stop'
$RepoRoot = Resolve-Path (Join-Path $PSScriptRoot '..')
$Project = Join-Path $RepoRoot 'game\WorldMakers.uproject'
$BuildBat = Join-Path $EngineRoot 'Engine\Build\BatchFiles\Build.bat'

if (-not (Test-Path $BuildBat)) {
    throw "Unreal Build.bat not found under EngineRoot: $EngineRoot"
}

& $BuildBat WorldMakersEditor Win64 $Configuration $Project -WaitMutex -NoHotReloadFromIDE
if ($LASTEXITCODE -ne 0) {
    throw "Unreal build failed with exit code $LASTEXITCODE"
}
