param(
    [string]$EvidenceRoot = 'artifacts\release-candidate',
    [string]$ExternalAlphaResult = 'artifacts\external-alpha\external-alpha-readiness.json',
    [switch]$GenerateSbom,
    [switch]$AllowNonMain,
    [switch]$AllowDirtyWorktree
)

$ErrorActionPreference = 'Stop'
$RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$EvidencePath = if ([IO.Path]::IsPathRooted($EvidenceRoot)) { $EvidenceRoot } else { Join-Path $RepoRoot $EvidenceRoot }
$ExternalPath = if ([IO.Path]::IsPathRooted($ExternalAlphaResult)) { $ExternalAlphaResult } else { Join-Path $RepoRoot $ExternalAlphaResult }
$Validator = Join-Path $RepoRoot 'scripts\validate-release-candidate-readiness.py'
$Assessor = Join-Path $RepoRoot 'scripts\assess-release-candidate-readiness.py'
$SbomGenerator = Join-Path $RepoRoot 'scripts\generate-release-candidate-sbom.py'
$ResultPath = Join-Path $EvidencePath 'release-candidate-readiness.json'

function Get-GitValue([string[]]$Args) {
    $Value = (& git -C $RepoRoot @Args 2>$null | Out-String).Trim()
    if ($LASTEXITCODE -ne 0) { return $null }
    return $Value
}

$Python = Get-Command python -ErrorAction SilentlyContinue
if ($null -eq $Python) { throw 'Python is required for Release Candidate certification.' }

& $Python.Source $Validator
if ($LASTEXITCODE -ne 0) { throw 'Release Candidate source contract validation failed.' }

& git -C $RepoRoot fetch origin main --quiet
if ($LASTEXITCODE -ne 0) { throw 'Unable to fetch origin/main for Release Candidate certification.' }
$Commit = Get-GitValue @('rev-parse','HEAD')
$Branch = Get-GitValue @('rev-parse','--abbrev-ref','HEAD')
$OriginMain = Get-GitValue @('rev-parse','origin/main')
if ([string]::IsNullOrWhiteSpace($Commit) -or $Commit.Length -ne 40) { throw 'Unable to resolve current git commit.' }
$WorkingTree = @(& git -C $RepoRoot status --porcelain --untracked-files=all)

if ($GenerateSbom) {
    $SbomPath = Join-Path $EvidencePath 'payloads\worldmakers.spdx.json'
    New-Item -ItemType Directory -Force -Path (Split-Path $SbomPath -Parent) | Out-Null
    & $Python.Source $SbomGenerator --commit $Commit --output $SbomPath
    if ($LASTEXITCODE -ne 0) { throw 'Release Candidate SBOM generation failed.' }
    Write-Host "Generated RC SBOM: $SbomPath"
}

New-Item -ItemType Directory -Force -Path $EvidencePath | Out-Null
if (Test-Path $ResultPath -PathType Leaf) { Remove-Item $ResultPath -Force }

& $Python.Source $Assessor --evidence-root $EvidencePath --external-alpha-result $ExternalPath --expected-commit $Commit --output $ResultPath
$AssessmentExit = $LASTEXITCODE
if (-not (Test-Path $ResultPath -PathType Leaf)) { throw 'Release Candidate assessor did not produce its canonical result.' }
$Assessment = Get-Content $ResultPath -Raw | ConvertFrom-Json

if ($AssessmentExit -ne 0 -or $Assessment.status -ne 'CERTIFIED' -or -not [bool]$Assessment.certified) {
    Write-Host ''
    Write-Host 'WORLD MAKERS RELEASE CANDIDATE READINESS: BLOCKED' -ForegroundColor Red
    Write-Host "Evidence: $ResultPath"
    throw 'Release Candidate evidence is incomplete, inconsistent, non-reproducible, regression-failing, unsafe, or not approved.'
}

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
    $Assessment.nextPhase = 'production-release-readiness'
    $Assessment | ConvertTo-Json -Depth 14 | Set-Content -Path $ResultPath -Encoding UTF8
    Write-Host ''
    Write-Host 'WORLD MAKERS RELEASE CANDIDATE READINESS: CERTIFIED' -ForegroundColor Green
    Write-Host "Version: $($Assessment.releaseVersion)"
    Write-Host "Evidence: $ResultPath"
    exit 0
}

$Assessment.status = 'NON_CERTIFYING_PASS'
$Assessment.certified = $false
$Reasons = @($Assessment.reasons)
$Reasons += 'All RC evidence passed, but certification requires clean current main synchronized with origin/main.'
$Assessment.reasons = $Reasons
$Assessment.nextPhase = $null
$Assessment | ConvertTo-Json -Depth 14 | Set-Content -Path $ResultPath -Encoding UTF8
Write-Host ''
Write-Host 'WORLD MAKERS RELEASE CANDIDATE READINESS: NON-CERTIFYING PASS' -ForegroundColor Yellow
Write-Host "Evidence: $ResultPath"
exit 0
