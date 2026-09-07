param(
    [Parameter(Mandatory = $true)]
    [string]$EngineRoot,
    [ValidateSet('Development', 'DebugGame', 'Shipping')]
    [string]$Configuration = 'Development',
    [string]$EvidenceDir = 'artifacts\certification'
)

$ErrorActionPreference = 'Stop'
$RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$Project = Join-Path $RepoRoot 'game\WorldMakers.uproject'
$ExpectedVersionFile = Join-Path $RepoRoot 'game\UNREAL_ENGINE_VERSION'
$BuildVersionFile = Join-Path $EngineRoot 'Engine\Build\Build.version'
$BuildBat = Join-Path $EngineRoot 'Engine\Build\BatchFiles\Build.bat'
$EvidencePath = Join-Path $RepoRoot $EvidenceDir
New-Item -ItemType Directory -Force -Path $EvidencePath | Out-Null
$LogPath = Join-Path $EvidencePath 'build.log'
$ResultPath = Join-Path $EvidencePath 'build-result.json'

if (-not (Test-Path $ExpectedVersionFile)) { throw 'Missing game/UNREAL_ENGINE_VERSION certification lock.' }
if (-not (Test-Path $BuildVersionFile)) { throw "Unreal Build.version not found under EngineRoot: $EngineRoot" }
if (-not (Test-Path $BuildBat)) { throw "Unreal Build.bat not found under EngineRoot: $EngineRoot" }

$ExpectedVersion = (Get-Content $ExpectedVersionFile -Raw).Trim()
$BuildVersion = Get-Content $BuildVersionFile -Raw | ConvertFrom-Json
$ActualVersion = "$($BuildVersion.MajorVersion).$($BuildVersion.MinorVersion).$($BuildVersion.PatchVersion)"
if ($ActualVersion -ne $ExpectedVersion) { throw "Unreal version mismatch. Expected $ExpectedVersion but runner has $ActualVersion." }

$Started = (Get-Date).ToUniversalTime()
Write-Host "Certified Unreal Engine version: $ActualVersion"
& $BuildBat WorldMakersEditor Win64 $Configuration $Project -WaitMutex -NoHotReloadFromIDE 2>&1 | Tee-Object -FilePath $LogPath
$ExitCode = $LASTEXITCODE
$Finished = (Get-Date).ToUniversalTime()

[ordered]@{
    schemaVersion = 1
    status = if ($ExitCode -eq 0) { 'passed' } else { 'failed' }
    exitCode = $ExitCode
    configuration = $Configuration
    target = 'WorldMakersEditor'
    platform = 'Win64'
    unrealVersion = $ActualVersion
    startedAtUtc = $Started.ToString('o')
    finishedAtUtc = $Finished.ToString('o')
    log = 'build.log'
} | ConvertTo-Json -Depth 4 | Set-Content -Path $ResultPath -Encoding UTF8

if ($ExitCode -ne 0) { throw "Unreal build failed with exit code $ExitCode" }
