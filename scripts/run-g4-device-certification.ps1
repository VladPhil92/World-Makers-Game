param(
    [string]$EvidenceRoot = 'artifacts\g4-device',
    [string]$G3Result = 'artifacts\g3-visual\g3-visual-fidelity.json',
    [switch]$AllowNonMain,
    [switch]$AllowDirtyWorktree
)

$ErrorActionPreference = 'Stop'
$RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$EvidencePath = Join-Path $RepoRoot $EvidenceRoot
$G3ResultPath = if ([System.IO.Path]::IsPathRooted($G3Result)) { $G3Result } else { Join-Path $RepoRoot $G3Result }
$ValidateScript = Join-Path $RepoRoot 'scripts\validate-g4-device-performance.py'
$AssessScript = Join-Path $RepoRoot 'scripts\assess-g4-device-performance.py'
$ResultPath = Join-Path $EvidencePath 'g4-device-performance.json'

New-Item -ItemType Directory -Force -Path $EvidencePath | Out-Null

function Get-GitValue([string[]]$Args) {
    $Value = (& git -C $RepoRoot @Args 2>$null | Out-String).Trim()
    if ($LASTEXITCODE -ne 0) { return $null }
    return $Value
}

$Python = Get-Command python -ErrorAction SilentlyContinue
if ($null -eq $Python) { throw 'Python is required for G4 certification.' }

& $Python.Source $ValidateScript
if ($LASTEXITCODE -ne 0) { throw 'G4 source contract validation failed.' }

& git -C $RepoRoot fetch origin main --quiet
if ($LASTEXITCODE -ne 0) { throw 'Unable to fetch origin/main for G4 certification.' }

$Commit = Get-GitValue @('rev-parse', 'HEAD')
$Branch = Get-GitValue @('rev-parse', '--abbrev-ref', 'HEAD')
$OriginMain = Get-GitValue @('rev-parse', 'origin/main')
if ([string]::IsNullOrWhiteSpace($Commit) -or $Commit.Length -ne 40) { throw 'Unable to resolve current commit.' }

if (Test-Path $ResultPath -PathType Leaf) { Remove-Item $ResultPath -Force }

& $Python.Source $AssessScript --evidence-root $EvidencePath --g3-result $G3ResultPath --expected-commit $Commit --output $ResultPath
$AssessmentExit = $LASTEXITCODE
if (-not (Test-Path $ResultPath -PathType Leaf)) {
    throw 'G4 assessor did not produce a canonical result.'
}
$Assessment = Get-Content $ResultPath -Raw | ConvertFrom-Json

if ($AssessmentExit -ne 0 -or $Assessment.status -ne 'CERTIFIED' -or -not [bool]$Assessment.certified) {
    Write-Host ''
    Write-Host 'WORLD MAKERS G4 DEVICE PERFORMANCE: BLOCKED' -ForegroundColor Red
    Write-Host "Evidence: $ResultPath"
    throw 'G4 representative-device evidence is incomplete, invalid, over budget, thermally unstable, or not tied to certified G3.'
}

$WorkingTree = @(& git -C $RepoRoot status --porcelain --untracked-files=all)
$CertifyingContext = (
    -not $AllowNonMain -and
    -not $AllowDirtyWorktree -and
    $Branch -eq 'main' -and
    $Commit -eq $OriginMain -and
    $WorkingTree.Count -eq 0
)

$Assessment | Add-Member -NotePropertyName branch -NotePropertyValue $Branch -Force
$Assessment | Add-Member -NotePropertyName originMainCommit -NotePropertyValue $OriginMain -Force
$Assessment | Add-Member -NotePropertyName generatedAtUtc -NotePropertyValue ((Get-Date).ToUniversalTime().ToString('o')) -Force

if ($CertifyingContext) {
    $Assessment.status = 'CERTIFIED'
    $Assessment.certified = $true
    $Assessment | Add-Member -NotePropertyName nextPhase -NotePropertyValue 'Production Alpha Readiness' -Force
    $Assessment | ConvertTo-Json -Depth 12 | Set-Content -Path $ResultPath -Encoding UTF8
    Write-Host ''
    Write-Host 'WORLD MAKERS G4 DEVICE PERFORMANCE & STABILITY: CERTIFIED' -ForegroundColor Green
    Write-Host "Evidence: $ResultPath"
    exit 0
}

$Assessment.status = 'NON_CERTIFYING_PASS'
$Assessment.certified = $false
$Reasons = @($Assessment.reasons)
$Reasons += 'All G4 evidence passed, but only clean current main synchronized with origin/main may certify G4.'
$Assessment.reasons = $Reasons
$Assessment | Add-Member -NotePropertyName nextPhase -NotePropertyValue $null -Force
$Assessment | ConvertTo-Json -Depth 12 | Set-Content -Path $ResultPath -Encoding UTF8
Write-Host ''
Write-Host 'WORLD MAKERS G4 DEVICE PERFORMANCE & STABILITY: NON-CERTIFYING PASS' -ForegroundColor Yellow
Write-Host "Evidence: $ResultPath"
exit 0
