[CmdletBinding()]
param(
    [string]$EvidencePath = "artifacts\pre-unreal-handoff\cloud-e2e.json",
    [string]$OutputPath = "artifacts\pre-unreal-handoff\final-pre-unreal-handoff.json",
    [switch]$AllowNonMain,
    [switch]$AllowDirtyWorktree
)

$ErrorActionPreference = "Stop"
$RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
Set-Location $RepoRoot

function Invoke-PythonChecked {
    param([Parameter(Mandatory=$true)][string[]]$Arguments)
    & python @Arguments
    if ($LASTEXITCODE -ne 0) {
        throw "Python command failed ($LASTEXITCODE): python $($Arguments -join ' ')"
    }
}

Write-Host "== World Makers Final Pre-Unreal Handoff ==" -ForegroundColor Cyan
Write-Host "Validating all source-side pre-native boundaries..."
Invoke-PythonChecked @("scripts/validate-pre-unreal-content-platform.py")
Invoke-PythonChecked @("scripts/validate-cloud-runtime-readiness.py")
Invoke-PythonChecked @("scripts/validate-unreal-source-preflight.py")
Invoke-PythonChecked @("scripts/validate-unreal-production-baseline.py")
Invoke-PythonChecked @("scripts/validate-final-pre-unreal-handoff.py")

& git fetch origin main --quiet
if ($LASTEXITCODE -ne 0) { throw "git fetch origin main failed" }

$Head = (& git rev-parse HEAD).Trim()
$Branch = (& git branch --show-current).Trim()
$OriginMain = (& git rev-parse origin/main).Trim()
$DirtyRows = @(& git status --porcelain --untracked-files=all)
$IsDirty = $DirtyRows.Count -gt 0
$OnCurrentMain = ($Branch -eq "main" -and $Head -eq $OriginMain)

if (-not $AllowNonMain -and -not $OnCurrentMain) {
    Write-Warning "Official PRE_UNREAL_READY requires branch main at current origin/main. Current branch=$Branch head=$Head origin/main=$OriginMain"
}
if (-not $AllowDirtyWorktree -and $IsDirty) {
    Write-Warning "Official PRE_UNREAL_READY requires a clean worktree."
}

$CertifyingContext = $OnCurrentMain -and (-not $IsDirty) -and (-not $AllowNonMain) -and (-not $AllowDirtyWorktree)
$AssessorArgs = @(
    "scripts/assess-final-pre-unreal-handoff.py",
    "--evidence", $EvidencePath,
    "--expected-commit", $Head,
    "--output", $OutputPath
)
if ($CertifyingContext) {
    $AssessorArgs += "--certifying-context"
}

& python @AssessorArgs
$AssessorExit = $LASTEXITCODE

if (-not (Test-Path $OutputPath)) {
    throw "Final handoff assessor did not produce $OutputPath"
}

$Result = Get-Content $OutputPath -Raw | ConvertFrom-Json
if ($Result.status -eq "PRE_UNREAL_READY") {
    Write-Host "PRE_UNREAL_READY: all source and external pre-native handoff evidence passed on clean current main." -ForegroundColor Green
    exit 0
}
if ($Result.status -eq "NON_CERTIFYING_PASS") {
    Write-Host "NON_CERTIFYING_PASS: evidence passed, but this checkout is not an official certifying context." -ForegroundColor Yellow
    exit 0
}

Write-Host "BLOCKED: Final Pre-Unreal handoff is not complete." -ForegroundColor Red
foreach ($Reason in $Result.blockingReasons) {
    Write-Host " - $Reason" -ForegroundColor Red
}
if ($AssessorExit -eq 0) { exit 2 }
exit $AssessorExit
