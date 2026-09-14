param(
    [string]$EngineRoot,
    [ValidateSet('Development', 'DebugGame', 'Shipping')]
    [string]$Configuration = 'Development',
    [string]$EvidenceDir = 'artifacts\certification',
    [switch]$StopBlockingProcesses
)

$ErrorActionPreference = 'Stop'
$RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$Project = Join-Path $RepoRoot 'game\WorldMakers.uproject'
$ExpectedVersionFile = Join-Path $RepoRoot 'game\UNREAL_ENGINE_VERSION'
$EngineResolver = Join-Path $RepoRoot 'scripts\resolve-unreal-engine.ps1'
$EvidencePath = Join-Path $RepoRoot $EvidenceDir
New-Item -ItemType Directory -Force -Path $EvidencePath | Out-Null
$LogPath = Join-Path $EvidencePath 'build.log'
$ResultPath = Join-Path $EvidencePath 'build-result.json'

function Get-BlockingUnrealProcesses {
    $Names = @('UnrealEditor', 'LiveCodingConsole')
    return @(Get-Process -ErrorAction SilentlyContinue | Where-Object { $Names -contains $_.ProcessName })
}

function Write-BuildResult {
    param(
        [string]$Status,
        [Nullable[int]]$ExitCode,
        [string]$Blocker,
        [object[]]$BlockingProcesses,
        [string]$UnrealVersion,
        [string]$ResolvedEngineRoot,
        [datetime]$Started,
        [datetime]$Finished
    )

    [ordered]@{
        schemaVersion = 2
        status = $Status
        exitCode = $ExitCode
        blocker = $Blocker
        blockingProcesses = @($BlockingProcesses | ForEach-Object { [ordered]@{ name = $_.ProcessName; pid = $_.Id } })
        configuration = $Configuration
        target = 'WorldMakersEditor'
        platform = 'Win64'
        unrealVersion = $UnrealVersion
        engineRoot = $ResolvedEngineRoot
        startedAtUtc = if ($Started) { $Started.ToUniversalTime().ToString('o') } else { $null }
        finishedAtUtc = if ($Finished) { $Finished.ToUniversalTime().ToString('o') } else { $null }
        log = 'build.log'
    } | ConvertTo-Json -Depth 6 | Set-Content -Path $ResultPath -Encoding UTF8
}

if (-not (Test-Path $ExpectedVersionFile -PathType Leaf)) { throw 'Missing game/UNREAL_ENGINE_VERSION certification lock.' }
if (-not (Test-Path $EngineResolver -PathType Leaf)) { throw 'Missing scripts/resolve-unreal-engine.ps1.' }

$ExpectedVersion = (Get-Content $ExpectedVersionFile -Raw).Trim()
$Resolution = & $EngineResolver -RequestedRoot $EngineRoot -ExpectedVersion $ExpectedVersion
if ($null -eq $Resolution -or [string]::IsNullOrWhiteSpace([string]$Resolution.path)) {
    throw 'Unable to resolve the Unreal Engine root.'
}
$ResolvedEngineRoot = [string]$Resolution.path
$BuildVersionFile = Join-Path $ResolvedEngineRoot 'Engine\Build\Build.version'
$BuildBat = Join-Path $ResolvedEngineRoot 'Engine\Build\BatchFiles\Build.bat'

if (-not (Test-Path $BuildVersionFile -PathType Leaf)) { throw "Unreal Build.version not found under EngineRoot: $ResolvedEngineRoot" }
if (-not (Test-Path $BuildBat -PathType Leaf)) { throw "Unreal Build.bat not found under EngineRoot: $ResolvedEngineRoot" }

$BuildVersion = Get-Content $BuildVersionFile -Raw | ConvertFrom-Json
$ActualVersion = "$($BuildVersion.MajorVersion).$($BuildVersion.MinorVersion).$($BuildVersion.PatchVersion)"
if ($ActualVersion -ne $ExpectedVersion) { throw "Unreal version mismatch. Expected $ExpectedVersion but runner has $ActualVersion." }

$BlockingProcesses = Get-BlockingUnrealProcesses
if ($BlockingProcesses.Count -gt 0 -and $StopBlockingProcesses) {
    foreach ($Process in $BlockingProcesses) {
        Write-Host "Stopping blocking process $($Process.ProcessName) (PID $($Process.Id))..." -ForegroundColor Yellow
        Stop-Process -Id $Process.Id -Force -ErrorAction Stop
    }
    Start-Sleep -Seconds 2
    $BlockingProcesses = Get-BlockingUnrealProcesses
}

if ($BlockingProcesses.Count -gt 0) {
    Write-BuildResult -Status 'blocked' -ExitCode $null -Blocker 'blocking-unreal-process' -BlockingProcesses $BlockingProcesses -UnrealVersion $ActualVersion -ResolvedEngineRoot $ResolvedEngineRoot -Started $null -Finished $null
    $Summary = (@($BlockingProcesses | ForEach-Object { "$($_.ProcessName) (PID $($_.Id))" }) -join ', ')
    throw "Native build blocked because Unreal Editor or Live Coding is running: $Summary. Save your work and close those processes, or explicitly use -StopBlockingProcesses. This is not an installation failure."
}

$Started = (Get-Date).ToUniversalTime()
Write-Host "Certified Unreal Engine version: $ActualVersion"
Write-Host "Resolved engine root: $ResolvedEngineRoot"
& $BuildBat WorldMakersEditor Win64 $Configuration $Project -WaitMutex -NoHotReloadFromIDE 2>&1 | Tee-Object -FilePath $LogPath
$ExitCode = $LASTEXITCODE
$Finished = (Get-Date).ToUniversalTime()

Write-BuildResult -Status $(if ($ExitCode -eq 0) { 'passed' } else { 'failed' }) -ExitCode $ExitCode -Blocker $null -BlockingProcesses @() -UnrealVersion $ActualVersion -ResolvedEngineRoot $ResolvedEngineRoot -Started $Started -Finished $Finished

if ($ExitCode -ne 0) { throw "Unreal build failed with exit code $ExitCode. Inspect $LogPath; reinstalling Unreal is not the default corrective action." }
