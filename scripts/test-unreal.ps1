param(
    [string]$EngineRoot,
    [string]$TestFilter = 'WorldMakers.',
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
$LogPath = Join-Path $EvidencePath 'automation.log'
$ResultPath = Join-Path $EvidencePath 'automation-result.json'

function Get-BlockingUnrealProcesses {
    $Names = @('UnrealEditor', 'LiveCodingConsole')
    return @(Get-Process -ErrorAction SilentlyContinue | Where-Object { $Names -contains $_.ProcessName })
}

function Write-AutomationResult {
    param(
        [string]$Status,
        [Nullable[int]]$ExitCode,
        [string]$Blocker,
        [object[]]$BlockingProcesses,
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
        testFilter = $TestFilter
        engineRoot = $ResolvedEngineRoot
        startedAtUtc = if ($Started) { $Started.ToUniversalTime().ToString('o') } else { $null }
        finishedAtUtc = if ($Finished) { $Finished.ToUniversalTime().ToString('o') } else { $null }
        log = 'automation.log'
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
$EditorCmd = Join-Path $ResolvedEngineRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
if (-not (Test-Path $EditorCmd -PathType Leaf)) { throw "UnrealEditor-Cmd.exe not found under EngineRoot: $ResolvedEngineRoot" }

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
    Write-AutomationResult -Status 'blocked' -ExitCode $null -Blocker 'blocking-unreal-process' -BlockingProcesses $BlockingProcesses -ResolvedEngineRoot $ResolvedEngineRoot -Started $null -Finished $null
    $Summary = (@($BlockingProcesses | ForEach-Object { "$($_.ProcessName) (PID $($_.Id))" }) -join ', ')
    throw "Native automation blocked because Unreal Editor or Live Coding is already running: $Summary. Close it first or explicitly use -StopBlockingProcesses."
}

$Started = (Get-Date).ToUniversalTime()
$ExecCmds = "Automation RunTests $TestFilter; Quit"
& $EditorCmd $Project "-ExecCmds=$ExecCmds" -unattended -nop4 -nosplash -nullrhi '-TestExit=Automation Test Queue Empty' 2>&1 | Tee-Object -FilePath $LogPath
$ExitCode = $LASTEXITCODE
$Finished = (Get-Date).ToUniversalTime()

Write-AutomationResult -Status $(if ($ExitCode -eq 0) { 'passed' } else { 'failed' }) -ExitCode $ExitCode -Blocker $null -BlockingProcesses @() -ResolvedEngineRoot $ResolvedEngineRoot -Started $Started -Finished $Finished

if ($ExitCode -ne 0) { throw "Unreal automation tests failed with exit code $ExitCode. Inspect $LogPath before changing workstation dependencies." }
