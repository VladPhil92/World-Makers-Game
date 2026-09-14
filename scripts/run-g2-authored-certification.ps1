param(
    [string]$EngineRoot,
    [string]$EvidenceDir = 'artifacts\g2-authored',
    [string]$RouteReviewFile = '',
    [switch]$AuthorMap,
    [switch]$AllowNonMain,
    [switch]$AllowDirtyWorktree,
    [switch]$StopBlockingProcesses
)

$ErrorActionPreference = 'Stop'
$RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$EvidencePath = Join-Path $RepoRoot $EvidenceDir
$ContractPath = Join-Path $RepoRoot 'content\production\g2-authored-vertical-slice-v1.json'
$ValidateScript = Join-Path $RepoRoot 'scripts\validate-g2-authored-vertical-slice.py'
$ResolveEngineScript = Join-Path $RepoRoot 'scripts\resolve-unreal-engine.ps1'
$G1Script = Join-Path $RepoRoot 'scripts\run-g1-native-certification.ps1'
$AuthorScript = Join-Path $RepoRoot 'scripts\unreal\author-g2-certification-map.py'
$InspectScript = Join-Path $RepoRoot 'scripts\unreal\inspect-g2-certification-map.py'
$MapDiskPath = Join-Path $RepoRoot 'game\Content\WorldMakers\Maps\WM_PrototypeCertification.umap'
$ResultPath = Join-Path $EvidencePath 'g2-authored-vertical-slice.json'
$InspectionPath = Join-Path $EvidencePath 'g2-native-map-inspection.json'
$AuthorReportPath = Join-Path $EvidencePath 'g2-author-report.json'
$G1EvidenceDir = Join-Path $EvidenceDir 'g1'
$G1ResultPath = Join-Path $RepoRoot (Join-Path $G1EvidenceDir 'g1-native-readiness.json')
$ExpectedVersionFile = Join-Path $RepoRoot 'game\UNREAL_ENGINE_VERSION'

New-Item -ItemType Directory -Force -Path $EvidencePath | Out-Null

function Read-JsonIfPresent([string]$Path) {
    if (Test-Path $Path -PathType Leaf) {
        return Get-Content $Path -Raw | ConvertFrom-Json
    }
    return $null
}

function Get-GitValue([string[]]$Args) {
    $Value = (& git -C $RepoRoot @Args 2>$null | Out-String).Trim()
    if ($LASTEXITCODE -ne 0) { return $null }
    return $Value
}

function Write-G2Result {
    param(
        [Parameter(Mandatory = $true)][string]$Status,
        [Parameter(Mandatory = $true)][bool]$Certified,
        [string[]]$Blockers = @()
    )

    $Inspection = Read-JsonIfPresent $InspectionPath
    $G1 = Read-JsonIfPresent $G1ResultPath
    $Review = if (-not [string]::IsNullOrWhiteSpace($RouteReviewFile)) { Read-JsonIfPresent $RouteReviewFile } else { $null }
    $Commit = Get-GitValue @('rev-parse', 'HEAD')
    $Branch = Get-GitValue @('rev-parse', '--abbrev-ref', 'HEAD')
    $OriginMain = Get-GitValue @('rev-parse', 'origin/main')

    [ordered]@{
        schema = 'worldmakers.g2-authored-vertical-slice-result.v1'
        generatedAtUtc = (Get-Date).ToUniversalTime().ToString('o')
        gate = 'G2'
        status = $Status
        certified = $Certified
        repositoryCommit = $Commit
        branch = $Branch
        originMainCommit = $OriginMain
        map = [ordered]@{
            packagePath = '/Game/WorldMakers/Maps/WM_PrototypeCertification'
            diskPath = 'game/Content/WorldMakers/Maps/WM_PrototypeCertification.umap'
            present = (Test-Path $MapDiskPath -PathType Leaf)
            sha256 = if (Test-Path $MapDiskPath -PathType Leaf) { (Get-FileHash -Algorithm SHA256 $MapDiskPath).Hash.ToLowerInvariant() } else { $null }
        }
        checks = [ordered]@{
            g1Certified = ($G1 -and [bool]$G1.certified -and $G1.status -eq 'CERTIFIED')
            nativeMapInspection = ($Inspection -and $Inspection.status -eq 'passed')
            manualRouteReview = ($Review -and $Review.status -eq 'passed')
            cleanCertificationMode = (-not $AllowNonMain -and -not $AllowDirtyWorktree)
            mainBranch = ($Branch -eq 'main')
            originMainMatch = ($Commit -and $Commit -eq $OriginMain)
        }
        evidence = [ordered]@{
            g1Result = if (Test-Path $G1ResultPath) { (Resolve-Path $G1ResultPath).Path } else { $null }
            nativeInspection = if (Test-Path $InspectionPath) { 'g2-native-map-inspection.json' } else { $null }
            routeReview = if ($Review) { $RouteReviewFile } else { $null }
        }
        blockers = @($Blockers)
        nextGate = if ($Certified) { 'G3' } else { $null }
    } | ConvertTo-Json -Depth 8 | Set-Content -Path $ResultPath -Encoding UTF8
}

function Assert-RouteReview([object]$Review, [string]$ExpectedCommit) {
    if ($null -eq $Review) { throw 'G2 route review is missing.' }
    if ($Review.schema -ne 'worldmakers.g2-route-review.v1') { throw 'G2 route review schema mismatch.' }
    if ($Review.status -ne 'passed') { throw 'G2 route review status must be passed.' }
    if ($Review.mapPackagePath -ne '/Game/WorldMakers/Maps/WM_PrototypeCertification') { throw 'G2 route review map path mismatch.' }
    if ($Review.repositoryCommit -ne $ExpectedCommit) { throw "G2 route review commit mismatch. Expected $ExpectedCommit." }
    if ([string]::IsNullOrWhiteSpace([string]$Review.reviewer)) { throw 'G2 route review must name the reviewer.' }
    if ([string]::IsNullOrWhiteSpace([string]$Review.reviewedAtUtc)) { throw 'G2 route review must record reviewedAtUtc.' }
    if (@($Review.criticalIssues).Count -ne 0) { throw 'G2 route review contains critical issues.' }

    $RequiredCapabilities = @(
        'spawn-and-first-person-control',
        'observe-and-scan',
        'collect-resource',
        'perform-science-interaction',
        'craft-or-transform-material',
        'build-or-place-intervention',
        'trigger-visible-ecosystem-consequence',
        'complete-mission-evidence',
        'save-and-load-state'
    )
    foreach ($Capability in $RequiredCapabilities) {
        $Property = $Review.capabilities.PSObject.Properties[$Capability]
        if ($null -eq $Property -or -not [bool]$Property.Value.passed) {
            throw "G2 route capability did not pass: $Capability"
        }
    }

    foreach ($Property in $Review.routeChecks.PSObject.Properties) {
        if (-not [bool]$Property.Value) {
            throw "G2 route check did not pass: $($Property.Name)"
        }
    }
}

if (-not (Test-Path $ContractPath -PathType Leaf)) { throw 'Missing G2 production contract.' }
if (-not (Test-Path $ExpectedVersionFile -PathType Leaf)) { throw 'Missing Unreal version lock.' }
$ExpectedVersion = (Get-Content $ExpectedVersionFile -Raw).Trim()
if ($ExpectedVersion -ne '5.8.2') { throw "G2 requires UE 5.8.2; repository lock is '$ExpectedVersion'." }

$Python = Get-Command python -ErrorAction SilentlyContinue
if ($null -eq $Python) { throw 'Python is required for the G2 source validator.' }

& $Python.Source $ValidateScript
if ($LASTEXITCODE -ne 0) { throw 'G2 source contract validation failed.' }

if (-not (Test-Path $ResolveEngineScript -PathType Leaf)) { throw 'Missing Unreal engine resolver.' }
$Resolution = & $ResolveEngineScript -RequestedRoot $EngineRoot -ExpectedVersion $ExpectedVersion
if ($null -eq $Resolution -or [string]::IsNullOrWhiteSpace([string]$Resolution.path)) { throw 'Unable to resolve UE 5.8.2.' }
$ResolvedEngineRoot = [string]$Resolution.path
$EditorCmd = Join-Path $ResolvedEngineRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
if (-not (Test-Path $EditorCmd -PathType Leaf)) { throw "UnrealEditor-Cmd.exe missing under $ResolvedEngineRoot" }

if ($AuthorMap) {
    if (Test-Path $AuthorReportPath) { Remove-Item $AuthorReportPath -Force }
    $env:WM_G2_AUTHOR_REPORT = $AuthorReportPath
    & $EditorCmd (Join-Path $RepoRoot 'game\WorldMakers.uproject') -unattended -nop4 "-ExecutePythonScript=$AuthorScript" -log
    $AuthorExit = $LASTEXITCODE
    Remove-Item Env:WM_G2_AUTHOR_REPORT -ErrorAction SilentlyContinue
    $AuthorReport = Read-JsonIfPresent $AuthorReportPath
    if ($AuthorExit -ne 0 -or -not $AuthorReport -or $AuthorReport.status -ne 'passed' -or -not (Test-Path $MapDiskPath -PathType Leaf)) {
        throw "G2 map authoring failed. Inspect $AuthorReportPath."
    }
    Write-Host ''
    Write-Host 'WORLD MAKERS G2 MAP AUTHORING: COMPLETE' -ForegroundColor Green
    Write-Host 'The .umap now exists in the working tree. Review it in Unreal Editor, import/approve the nine P2 rainforest assets, commit the map/assets through Git LFS, then run G2 certification from clean current main.'
    exit 0
}

$StaleFiles = @('g2-authored-vertical-slice.json', 'g2-native-map-inspection.json')
foreach ($Name in $StaleFiles) {
    $Path = Join-Path $EvidencePath $Name
    if (Test-Path $Path -PathType Leaf) { Remove-Item $Path -Force }
}

try {
    & git -C $RepoRoot fetch origin main --quiet
    if ($LASTEXITCODE -ne 0) { throw 'Unable to fetch origin/main for G2 certification.' }

    & $Python.Source $ValidateScript --require-authored-map
    if ($LASTEXITCODE -ne 0) { throw 'G2 authored-map source validation failed.' }

    $G1Args = @('-EngineRoot', $ResolvedEngineRoot, '-EvidenceDir', $G1EvidenceDir)
    if ($AllowNonMain) { $G1Args += '-AllowNonMain' }
    if ($AllowDirtyWorktree) { $G1Args += '-AllowDirtyWorktree' }
    if ($StopBlockingProcesses) { $G1Args += '-StopBlockingProcesses' }
    & $G1Script @G1Args

    $G1 = Read-JsonIfPresent $G1ResultPath
    if ($null -eq $G1 -or ($G1.status -ne 'CERTIFIED' -and $G1.status -ne 'NON_CERTIFYING_PASS')) {
        throw 'G1 did not produce an acceptable native pass for G2.'
    }

    $env:WM_G2_INSPECTION_REPORT = $InspectionPath
    & $EditorCmd (Join-Path $RepoRoot 'game\WorldMakers.uproject') -unattended -nop4 -nullrhi "-ExecutePythonScript=$InspectScript" -log
    $InspectExit = $LASTEXITCODE
    Remove-Item Env:WM_G2_INSPECTION_REPORT -ErrorAction SilentlyContinue
    $Inspection = Read-JsonIfPresent $InspectionPath
    if ($InspectExit -ne 0 -or $null -eq $Inspection -or $Inspection.status -ne 'passed') {
        throw "G2 native map inspection failed. Inspect $InspectionPath."
    }

    if ([string]::IsNullOrWhiteSpace($RouteReviewFile)) {
        $RouteReviewFile = Join-Path $EvidencePath 'g2-route-review.json'
    }
    $Review = Read-JsonIfPresent $RouteReviewFile
    $Commit = Get-GitValue @('rev-parse', 'HEAD')
    Assert-RouteReview $Review $Commit

    $Branch = Get-GitValue @('rev-parse', '--abbrev-ref', 'HEAD')
    $OriginMain = Get-GitValue @('rev-parse', 'origin/main')
    $WorkingTree = @(& git -C $RepoRoot status --porcelain --untracked-files=all)
    $CertifyingContext = (
        -not $AllowNonMain -and
        -not $AllowDirtyWorktree -and
        $Branch -eq 'main' -and
        $Commit -eq $OriginMain -and
        $WorkingTree.Count -eq 0 -and
        [bool]$G1.certified -and
        $G1.status -eq 'CERTIFIED'
    )

    if ($CertifyingContext) {
        Write-G2Result -Status 'CERTIFIED' -Certified $true
        Write-Host ''
        Write-Host 'WORLD MAKERS G2 AUTHORED VERTICAL SLICE: CERTIFIED' -ForegroundColor Green
        Write-Host "Evidence: $ResultPath"
    }
    else {
        Write-G2Result -Status 'NON_CERTIFYING_PASS' -Certified $false -Blockers @('All G2 checks passed, but only clean current main with a certified G1 result may certify G2.')
        Write-Host ''
        Write-Host 'WORLD MAKERS G2 AUTHORED VERTICAL SLICE: NON-CERTIFYING PASS' -ForegroundColor Yellow
        Write-Host "Evidence: $ResultPath"
    }
}
catch {
    Write-G2Result -Status 'BLOCKED' -Certified $false -Blockers @($_.Exception.Message)
    throw "G2 authored vertical slice certification is BLOCKED. Inspect $ResultPath and related evidence. Root cause: $($_.Exception.Message)"
}
