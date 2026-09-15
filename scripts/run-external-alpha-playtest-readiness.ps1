param(
    [string]$EvidenceRoot = 'artifacts\external-alpha',
    [string]$ProductionAlphaResult = 'artifacts\production-alpha\production-alpha-readiness.json',
    [switch]$AllowNonMain,
    [switch]$AllowDirtyWorktree
)

$ErrorActionPreference = 'Stop'
$RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$EvidencePath = if ([IO.Path]::IsPathRooted($EvidenceRoot)) { $EvidenceRoot } else { Join-Path $RepoRoot $EvidenceRoot }
$ProductionAlphaPath = if ([IO.Path]::IsPathRooted($ProductionAlphaResult)) { $ProductionAlphaResult } else { Join-Path $RepoRoot $ProductionAlphaResult }
$Validator = Join-Path $RepoRoot 'scripts\validate-external-alpha-playtest.py'
$Assessor = Join-Path $RepoRoot 'scripts\assess-external-alpha-playtest.py'
$ResultPath = Join-Path $EvidencePath 'external-alpha-readiness.json'

function Get-GitValue([string[]]$Args) {
    $Value = (& git -C $RepoRoot @Args 2>$null | Out-String).Trim()
    if ($LASTEXITCODE -ne 0) { return $null }
    return $Value
}

$Python = Get-Command python -ErrorAction SilentlyContinue
if ($null -eq $Python) { throw 'Python is required for External Alpha certification.' }

& $Python.Source $Validator
if ($LASTEXITCODE -ne 0) { throw 'External Alpha source contract validation failed.' }

& git -C $RepoRoot fetch origin main --quiet
if ($LASTEXITCODE -ne 0) { throw 'Unable to fetch origin/main for External Alpha certification.' }

$Commit = Get-GitValue @('rev-parse','HEAD')
$Branch = Get-GitValue @('rev-parse','--abbrev-ref','HEAD')
$OriginMain = Get-GitValue @('rev-parse','origin/main')
if ([string]::IsNullOrWhiteSpace($Commit) -or $Commit.Length -ne 40) { throw 'Unable to resolve current commit.' }

New-Item -ItemType Directory -Force -Path $EvidencePath | Out-Null
if (Test-Path $ResultPath -PathType Leaf) { Remove-Item $ResultPath -Force }

& $Python.Source $Assessor --evidence-root $EvidencePath --production-alpha-result $ProductionAlphaPath --expected-commit $Commit --output $ResultPath
$AssessmentExit = $LASTEXITCODE
if (-not (Test-Path $ResultPath -PathType Leaf)) { throw 'External Alpha assessor did not produce a canonical result.' }
$Assessment = Get-Content $ResultPath -Raw | ConvertFrom-Json

if ($AssessmentExit -ne 0 -or $Assessment.status -ne 'CERTIFIED' -or -not [bool]$Assessment.certified) {
    Write-Host ''
    Write-Host 'WORLD MAKERS EXTERNAL ALPHA PLAYTEST: BLOCKED' -ForegroundColor Red
    Write-Host "Evidence: $ResultPath"
    throw 'External Alpha evidence has not met cohort, reliability, safety, issue or promotion requirements.'
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
    $Assessment | Add-Member -NotePropertyName nextPhase -NotePropertyValue 'Release Candidate Readiness' -Force
    $Assessment | ConvertTo-Json -Depth 12 | Set-Content -Path $ResultPath -Encoding UTF8
    Write-Host ''
    Write-Host 'WORLD MAKERS EXTERNAL ALPHA PLAYTEST: CERTIFIED FOR RELEASE CANDIDATE READINESS' -ForegroundColor Green
    Write-Host "Evidence: $ResultPath"
    exit 0
}

$Assessment.status = 'NON_CERTIFYING_PASS'
$Assessment.certified = $false
$Reasons = @($Assessment.reasons)
$Reasons += 'External Alpha evidence passed, but only clean current main synchronized with origin/main may certify promotion readiness.'
$Assessment.reasons = $Reasons
$Assessment | Add-Member -NotePropertyName nextPhase -NotePropertyValue $null -Force
$Assessment | ConvertTo-Json -Depth 12 | Set-Content -Path $ResultPath -Encoding UTF8
Write-Host ''
Write-Host 'WORLD MAKERS EXTERNAL ALPHA PLAYTEST: NON-CERTIFYING PASS' -ForegroundColor Yellow
Write-Host "Evidence: $ResultPath"
exit 0
