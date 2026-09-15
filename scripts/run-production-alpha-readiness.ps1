param(
    [string]$EvidenceRoot = 'artifacts\production-alpha',
    [string]$G4Result = 'artifacts\g4-device\g4-device-performance.json',
    [switch]$AllowNonMain,
    [switch]$AllowDirtyWorktree
)

$ErrorActionPreference = 'Stop'
$RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$Validator = Join-Path $RepoRoot 'scripts\validate-production-alpha-readiness.py'
$Assessor = Join-Path $RepoRoot 'scripts\assess-production-alpha-readiness.py'
$EvidencePath = Join-Path $RepoRoot $EvidenceRoot
$G4Path = Join-Path $RepoRoot $G4Result
$ResultPath = Join-Path $EvidencePath 'production-alpha-readiness.json'

function Get-GitValue([string[]]$Args) {
    $Value = (& git -C $RepoRoot @Args 2>$null | Out-String).Trim()
    if ($LASTEXITCODE -ne 0) { return $null }
    return $Value
}

$Python = Get-Command python -ErrorAction SilentlyContinue
if ($null -eq $Python) { throw 'Python is required for Production Alpha readiness assessment.' }

& $Python.Source $Validator
if ($LASTEXITCODE -ne 0) { throw 'Production Alpha source validation failed.' }

& git -C $RepoRoot fetch origin main --quiet
if ($LASTEXITCODE -ne 0) { throw 'Unable to fetch origin/main for Production Alpha certification.' }
$Commit = Get-GitValue @('rev-parse','HEAD')
$Branch = Get-GitValue @('rev-parse','--abbrev-ref','HEAD')
$OriginMain = Get-GitValue @('rev-parse','origin/main')
$Dirty = @(& git -C $RepoRoot status --porcelain --untracked-files=all)
if ([string]::IsNullOrWhiteSpace($Commit) -or $Commit.Length -ne 40) { throw 'Unable to resolve current commit.' }

$ContextBlockers = @()
if ($Branch -ne 'main') { $ContextBlockers += "branch is '$Branch', not main" }
if ($Commit -ne $OriginMain) { $ContextBlockers += 'HEAD does not equal origin/main' }
if ($Dirty.Count -gt 0) { $ContextBlockers += 'worktree is not clean' }

if (-not $AllowNonMain -and $Branch -ne 'main') {
    throw 'Production Alpha certification must run on main. Use -AllowNonMain only for a non-certifying verification pass.'
}
if (-not $AllowDirtyWorktree -and $Dirty.Count -gt 0) {
    throw 'Production Alpha certification requires a clean worktree. Use -AllowDirtyWorktree only for non-certifying verification.'
}

New-Item -ItemType Directory -Force -Path $EvidencePath | Out-Null
if (Test-Path $ResultPath) { Remove-Item $ResultPath -Force }

& $Python.Source $Assessor --evidence-root $EvidencePath --expected-commit $Commit --g4-result $G4Path --output $ResultPath
$AssessExit = $LASTEXITCODE
if (-not (Test-Path $ResultPath -PathType Leaf)) { throw 'Production Alpha assessor did not produce its canonical result.' }
$Result = Get-Content $ResultPath -Raw | ConvertFrom-Json

if ($AssessExit -eq 0 -and [bool]$Result.certified) {
    $CertifyingContext = ($Branch -eq 'main' -and $Commit -eq $OriginMain -and $Dirty.Count -eq 0 -and -not $AllowNonMain -and -not $AllowDirtyWorktree)
    if ($CertifyingContext) {
        Write-Host ''
        Write-Host 'WORLD MAKERS PRODUCTION ALPHA READINESS: CERTIFIED' -ForegroundColor Green
        Write-Host "Evidence: $ResultPath"
        exit 0
    }

    $Result.status = 'NON_CERTIFYING_PASS'
    $Result.certified = $false
    $Result.nextPhase = $null
    if (-not $Result.PSObject.Properties['contextBlockers']) { $Result | Add-Member -NotePropertyName contextBlockers -NotePropertyValue @() }
    $Result.contextBlockers = @($ContextBlockers + 'Non-certifying override switches were used.')
    $Result | ConvertTo-Json -Depth 10 | Set-Content $ResultPath -Encoding UTF8
    Write-Host ''
    Write-Host 'WORLD MAKERS PRODUCTION ALPHA READINESS: NON-CERTIFYING PASS' -ForegroundColor Yellow
    Write-Host "Evidence: $ResultPath"
    exit 0
}

Write-Host ''
Write-Host 'WORLD MAKERS PRODUCTION ALPHA READINESS: BLOCKED' -ForegroundColor Red
Write-Host "Evidence: $ResultPath"
exit 2
