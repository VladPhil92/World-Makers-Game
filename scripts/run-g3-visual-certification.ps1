param(
    [string]$EngineRoot,
    [string]$EvidenceDir = 'artifacts\g3-visual',
    [string]$ReviewFile = '',
    [string]$G2RouteReviewFile = '',
    [switch]$StopBlockingProcesses
)

$ErrorActionPreference = 'Stop'
$RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$EvidencePath = Join-Path $RepoRoot $EvidenceDir
$ValidateScript = Join-Path $RepoRoot 'scripts\validate-g3-visual-fidelity.py'
$ResolveEngineScript = Join-Path $RepoRoot 'scripts\resolve-unreal-engine.ps1'
$G2Script = Join-Path $RepoRoot 'scripts\run-g2-authored-certification.ps1'
$P4Generator = Join-Path $RepoRoot 'scripts\generate-p4-authored-motion-vfx-presentation.py'
$InventoryCollector = Join-Path $RepoRoot 'scripts\unreal\collect-p5-native-inventory.py'
$MapInspector = Join-Path $RepoRoot 'scripts\unreal\inspect-g3-visual-map.py'
$Assessor = Join-Path $RepoRoot 'scripts\assess-g3-visual-fidelity.py'
$ExpectedVersionFile = Join-Path $RepoRoot 'game\UNREAL_ENGINE_VERSION'
$Project = Join-Path $RepoRoot 'game\WorldMakers.uproject'
$ResultPath = Join-Path $EvidencePath 'g3-visual-fidelity.json'
$InventoryPath = Join-Path $EvidencePath 'native-inventory.json'
$InspectionPath = Join-Path $EvidencePath 'g3-native-map-inspection.json'
$P4BundlePath = Join-Path $EvidencePath 'p4-source-bundle.json'
$G2EvidenceDir = Join-Path $EvidenceDir 'g2'
$G2ResultPath = Join-Path $RepoRoot (Join-Path $G2EvidenceDir 'g2-authored-vertical-slice.json')

New-Item -ItemType Directory -Force -Path $EvidencePath | Out-Null

function Get-GitValue([string[]]$Args) {
    $Value = (& git -C $RepoRoot @Args 2>$null | Out-String).Trim()
    if ($LASTEXITCODE -ne 0) { return $null }
    return $Value
}

function Write-BlockedResult([string]$Reason) {
    $Commit = Get-GitValue @('rev-parse', 'HEAD')
    [ordered]@{
        schema = 'worldmakers.g3-visual-fidelity-result.v1'
        gate = 'G3'
        status = 'BLOCKED'
        certified = $false
        repositoryCommit = $Commit
        reasons = @($Reason)
    } | ConvertTo-Json -Depth 8 | Set-Content -Path $ResultPath -Encoding UTF8
}

$Python = Get-Command python -ErrorAction SilentlyContinue
if ($null -eq $Python) { throw 'Python is required for G3.' }
if (-not (Test-Path $ExpectedVersionFile -PathType Leaf)) { throw 'Missing Unreal version lock.' }
$ExpectedVersion = (Get-Content $ExpectedVersionFile -Raw).Trim()
if ($ExpectedVersion -ne '5.8.2') { throw "G3 requires UE 5.8.2; repository lock is '$ExpectedVersion'." }

& $Python.Source $ValidateScript
if ($LASTEXITCODE -ne 0) { throw 'G3 source contract validation failed.' }

$Resolution = & $ResolveEngineScript -RequestedRoot $EngineRoot -ExpectedVersion $ExpectedVersion
if ($null -eq $Resolution -or [string]::IsNullOrWhiteSpace([string]$Resolution.path)) { throw 'Unable to resolve UE 5.8.2.' }
$ResolvedEngineRoot = [string]$Resolution.path
$EditorCmd = Join-Path $ResolvedEngineRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
if (-not (Test-Path $EditorCmd -PathType Leaf)) { throw "UnrealEditor-Cmd.exe missing under $ResolvedEngineRoot" }

$StaleFiles = @('g3-visual-fidelity.json', 'native-inventory.json', 'g3-native-map-inspection.json', 'p4-source-bundle.json')
foreach ($Name in $StaleFiles) {
    $Path = Join-Path $EvidencePath $Name
    if (Test-Path $Path -PathType Leaf) { Remove-Item $Path -Force }
}

try {
    & git -C $RepoRoot fetch origin main --quiet
    if ($LASTEXITCODE -ne 0) { throw 'Unable to fetch origin/main for G3 certification.' }

    $Commit = Get-GitValue @('rev-parse', 'HEAD')
    $Branch = Get-GitValue @('rev-parse', '--abbrev-ref', 'HEAD')
    $OriginMain = Get-GitValue @('rev-parse', 'origin/main')
    $WorkingTree = @(& git -C $RepoRoot status --porcelain --untracked-files=all)
    if ($Branch -ne 'main' -or $Commit -ne $OriginMain -or $WorkingTree.Count -ne 0) {
        throw 'G3 certification only runs on clean current main. Use the PR workflows for non-certifying development validation.'
    }

    $G2Args = @('-EngineRoot', $ResolvedEngineRoot, '-EvidenceDir', $G2EvidenceDir)
    if (-not [string]::IsNullOrWhiteSpace($G2RouteReviewFile)) {
        $G2Args += @('-RouteReviewFile', $G2RouteReviewFile)
    }
    if ($StopBlockingProcesses) { $G2Args += '-StopBlockingProcesses' }
    & $G2Script @G2Args
    if ($LASTEXITCODE -ne 0) { throw 'G2 prerequisite failed.' }

    if (-not (Test-Path $G2ResultPath -PathType Leaf)) { throw 'G2 result is missing after prerequisite execution.' }
    $G2 = Get-Content $G2ResultPath -Raw | ConvertFrom-Json
    if ($G2.status -ne 'CERTIFIED' -or -not [bool]$G2.certified -or $G2.repositoryCommit -ne $Commit) {
        throw 'G2 must be CERTIFIED on the same commit before G3 can certify.'
    }

    & $Python.Source $P4Generator --output $P4BundlePath --verify
    if ($LASTEXITCODE -ne 0 -or -not (Test-Path $P4BundlePath -PathType Leaf)) { throw 'Unable to generate verified P4 source bundle.' }

    $env:WM_P4_SOURCE_BUNDLE = $P4BundlePath
    $env:WM_P5_NATIVE_INVENTORY = $InventoryPath
    $env:WM_BUILD_COMMIT = $Commit
    & $EditorCmd $Project -unattended -nop4 -nullrhi "-ExecutePythonScript=$InventoryCollector" -log
    $InventoryExit = $LASTEXITCODE
    Remove-Item Env:WM_P4_SOURCE_BUNDLE -ErrorAction SilentlyContinue
    Remove-Item Env:WM_P5_NATIVE_INVENTORY -ErrorAction SilentlyContinue
    if ($InventoryExit -ne 0 -or -not (Test-Path $InventoryPath -PathType Leaf)) {
        throw 'G3 native authored inventory collection failed.'
    }

    $env:WM_G3_MAP_INSPECTION = $InspectionPath
    $env:WM_BUILD_COMMIT = $Commit
    & $EditorCmd $Project -unattended -nop4 -nullrhi "-ExecutePythonScript=$MapInspector" -log
    $InspectExit = $LASTEXITCODE
    Remove-Item Env:WM_G3_MAP_INSPECTION -ErrorAction SilentlyContinue
    Remove-Item Env:WM_BUILD_COMMIT -ErrorAction SilentlyContinue
    if ($InspectExit -ne 0 -or -not (Test-Path $InspectionPath -PathType Leaf)) {
        throw 'G3 native certification-map inspection failed.'
    }

    if ([string]::IsNullOrWhiteSpace($ReviewFile)) {
        $ReviewFile = Join-Path $EvidencePath 'g3-review.json'
    }
    if (-not (Test-Path $ReviewFile -PathType Leaf)) {
        throw "G3 human review is missing. Copy content/production/g3-visual-review-template.json to $ReviewFile and populate it from real UE 5.8.2 evidence."
    }
    $CanonicalReview = Join-Path $EvidencePath 'g3-review.json'
    if ((Resolve-Path $ReviewFile).Path -ne $CanonicalReview) {
        Copy-Item $ReviewFile $CanonicalReview -Force
    }

    & $Python.Source $Assessor --evidence-root $EvidencePath --commit $Commit --g2-result $G2ResultPath --output $ResultPath
    if ($LASTEXITCODE -ne 0) { throw "G3 visual fidelity certification is BLOCKED. Inspect $ResultPath." }

    $Result = Get-Content $ResultPath -Raw | ConvertFrom-Json
    if ($Result.status -ne 'CERTIFIED' -or -not [bool]$Result.certified) {
        throw 'G3 assessor did not certify the build.'
    }

    Write-Host ''
    Write-Host 'WORLD MAKERS G3 VISUAL FIDELITY: CERTIFIED' -ForegroundColor Green
    Write-Host "Evidence: $ResultPath"
    Write-Host 'Next gate: G4 representative-device performance certification.'
}
catch {
    if (-not (Test-Path $ResultPath -PathType Leaf)) { Write-BlockedResult $_.Exception.Message }
    throw "G3 visual fidelity certification is BLOCKED. Root cause: $($_.Exception.Message)"
}
