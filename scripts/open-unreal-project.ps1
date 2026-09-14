param(
    [string]$EngineRoot,
    [string]$EvidenceDir = 'artifacts\unreal-readiness',
    [switch]$AllowDirtyWorktree,
    [switch]$AllowNonMain,
    [switch]$StopBlockingProcesses
)

$ErrorActionPreference = 'Stop'
$RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$Project = Join-Path $RepoRoot 'game\WorldMakers.uproject'
$ReadinessScript = Join-Path $RepoRoot 'scripts\run-unreal-readiness-gate.ps1'
$EvidencePath = Join-Path $RepoRoot $EvidenceDir
$ReadinessResult = Join-Path $EvidencePath 'readiness-result.json'
$DoctorResult = Join-Path $EvidencePath 'workstation-doctor.json'
$FailureSummary = Join-Path $EvidencePath 'native-failure-summary.json'
$LaunchResult = Join-Path $EvidencePath 'editor-launch-result.json'

New-Item -ItemType Directory -Force -Path $EvidencePath | Out-Null

# Do not let evidence from an earlier attempt be presented as the reason for
# the current launch failure. The current readiness/build run recreates it.
foreach ($StaleEvidence in @($FailureSummary, $LaunchResult)) {
    if (Test-Path $StaleEvidence -PathType Leaf) {
        Remove-Item $StaleEvidence -Force
    }
}

if (-not (Test-Path $Project -PathType Leaf)) {
    throw "World Makers project file not found: $Project"
}
if (-not (Test-Path $ReadinessScript -PathType Leaf)) {
    throw 'Missing scripts/run-unreal-readiness-gate.ps1.'
}

$PowerShellExe = Get-Command powershell.exe -ErrorAction SilentlyContinue
if ($null -eq $PowerShellExe) {
    throw 'Windows PowerShell is required to run the isolated readiness gate.'
}

$ReadinessArgs = @(
    '-NoLogo',
    '-NoProfile',
    '-ExecutionPolicy', 'Bypass',
    '-File', $ReadinessScript,
    '-EvidenceDir', $EvidenceDir,
    '-CleanIntermediate'
)
if (-not [string]::IsNullOrWhiteSpace($EngineRoot)) {
    $ReadinessArgs += @('-EngineRoot', $EngineRoot)
}
if ($AllowDirtyWorktree) {
    $ReadinessArgs += '-AllowDirtyWorktree'
}
if ($AllowNonMain) {
    $ReadinessArgs += '-AllowNonMain'
}
if ($StopBlockingProcesses) {
    $ReadinessArgs += '-StopBlockingProcesses'
}

Write-Host 'Running World Makers native readiness before opening Unreal Editor...' -ForegroundColor Cyan
& $PowerShellExe.Source @ReadinessArgs
$ReadinessExitCode = $LASTEXITCODE

if ($ReadinessExitCode -ne 0) {
    Write-Host ''
    Write-Host 'UNREAL EDITOR NOT OPENED: native readiness is blocked.' -ForegroundColor Red

    $CurrentReadiness = $null
    if (Test-Path $ReadinessResult -PathType Leaf) {
        try {
            $CurrentReadiness = Get-Content $ReadinessResult -Raw | ConvertFrom-Json
        }
        catch {
            Write-Host "Readiness evidence exists but could not be parsed: $ReadinessResult" -ForegroundColor Yellow
        }
    }

    if ($null -ne $CurrentReadiness -and [string]$CurrentReadiness.nativeBuildStatus -eq 'failed' -and (Test-Path $FailureSummary -PathType Leaf)) {
        try {
            $Summary = Get-Content $FailureSummary -Raw | ConvertFrom-Json
            Write-Host "Primary native failure: $($Summary.primaryCategory)" -ForegroundColor Yellow
            Write-Host "Action: $($Summary.remediation)" -ForegroundColor Yellow
        }
        catch {
            Write-Host "Current native failure summary could not be parsed: $FailureSummary" -ForegroundColor Yellow
        }
    }
    elseif ($null -ne $CurrentReadiness -and @($CurrentReadiness.blockers).Count -gt 0) {
        Write-Host 'Current readiness blockers:' -ForegroundColor Yellow
        foreach ($Blocker in @($CurrentReadiness.blockers)) {
            Write-Host "  - $Blocker" -ForegroundColor Yellow
        }
    }

    Write-Host "Readiness evidence: $ReadinessResult"
    Write-Host "Workstation evidence: $DoctorResult"
    exit $ReadinessExitCode
}

if (-not (Test-Path $ReadinessResult -PathType Leaf)) {
    throw "Readiness gate returned success but did not produce: $ReadinessResult"
}
if (-not (Test-Path $DoctorResult -PathType Leaf)) {
    throw "Readiness gate returned success but did not produce: $DoctorResult"
}

$Readiness = Get-Content $ReadinessResult -Raw | ConvertFrom-Json
$Doctor = Get-Content $DoctorResult -Raw | ConvertFrom-Json
if ([string]$Readiness.status -ne 'ready') {
    throw "Readiness evidence is not ready: $($Readiness.status)"
}
if ([string]$Doctor.status -ne 'ready') {
    throw "Workstation doctor evidence is not ready: $($Doctor.status)"
}

$ResolvedEngineRoot = [string]$Doctor.engineRoot
if ([string]::IsNullOrWhiteSpace($ResolvedEngineRoot)) {
    throw 'Workstation doctor did not provide a resolved Unreal Engine root.'
}
$EditorExe = Join-Path $ResolvedEngineRoot 'Engine\Binaries\Win64\UnrealEditor.exe'
if (-not (Test-Path $EditorExe -PathType Leaf)) {
    throw "UnrealEditor.exe not found after readiness passed: $EditorExe"
}

$CommitSha = (& git -C $RepoRoot rev-parse HEAD 2>$null | Out-String).Trim()
$StartedAt = (Get-Date).ToUniversalTime()
$EditorProcess = Start-Process -FilePath $EditorExe -ArgumentList @($Project) -WorkingDirectory $RepoRoot -PassThru

[ordered]@{
    schemaVersion = 1
    status = 'launched'
    launchedAtUtc = $StartedAt.ToString('o')
    repositoryCommit = $CommitSha
    project = 'game/WorldMakers.uproject'
    unrealVersion = [string]$Doctor.actualUnrealVersion
    processId = $EditorProcess.Id
    readinessEvidence = 'readiness-result.json'
    workstationEvidence = 'workstation-doctor.json'
    nativeBuildEvidence = 'build-result.json'
    installPolicy = 'manual-only-no-auto-install'
} | ConvertTo-Json -Depth 4 | Set-Content -Path $LaunchResult -Encoding UTF8

Write-Host ''
Write-Host 'WORLD MAKERS UNREAL EDITOR: LAUNCHED' -ForegroundColor Green
Write-Host "Process: $($EditorProcess.Id)"
Write-Host "Evidence: $LaunchResult"
