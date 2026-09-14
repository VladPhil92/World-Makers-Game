param(
    [string]$EngineRoot,
    [string]$EvidenceDir = 'artifacts\g1-native',
    [switch]$CleanIntermediate,
    [switch]$AllowNonMain,
    [switch]$AllowDirtyWorktree,
    [switch]$StopBlockingProcesses
)

$ErrorActionPreference = 'Stop'
$RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$EvidencePath = Join-Path $RepoRoot $EvidenceDir
$ReadinessScript = Join-Path $RepoRoot 'scripts\run-unreal-readiness-gate.ps1'
$ReadinessResult = Join-Path $EvidencePath 'readiness-result.json'
$G1Result = Join-Path $EvidencePath 'g1-native-readiness.json'

New-Item -ItemType Directory -Force -Path $EvidencePath | Out-Null

$StaleEvidence = @(
    'g1-native-readiness.json',
    'readiness-result.json',
    'workstation-doctor.json',
    'source-preflight.log',
    'build-result.json',
    'build.log',
    'native-failure-summary.json',
    'automation-result.json',
    'automation.log'
)
foreach ($Name in $StaleEvidence) {
    $Path = Join-Path $EvidencePath $Name
    if (Test-Path $Path -PathType Leaf) {
        Remove-Item $Path -Force
    }
}

function Read-JsonIfPresent([string]$Path) {
    if (Test-Path $Path -PathType Leaf) {
        return Get-Content $Path -Raw | ConvertFrom-Json
    }
    return $null
}

function Write-G1Result {
    param(
        [Parameter(Mandatory = $true)][string]$Status,
        [Parameter(Mandatory = $true)][bool]$Certified,
        [string[]]$Blockers = @()
    )

    $Readiness = Read-JsonIfPresent $ReadinessResult
    $Build = Read-JsonIfPresent (Join-Path $EvidencePath 'build-result.json')
    $Automation = Read-JsonIfPresent (Join-Path $EvidencePath 'automation-result.json')

    $EvidenceFiles = @(
        'workstation-doctor.json',
        'source-preflight.log',
        'readiness-result.json',
        'build-result.json',
        'build.log',
        'automation-result.json',
        'automation.log'
    )

    $EvidencePresence = [ordered]@{}
    foreach ($Name in $EvidenceFiles) {
        $EvidencePresence[$Name] = Test-Path (Join-Path $EvidencePath $Name) -PathType Leaf
    }

    [ordered]@{
        schema = 'worldmakers.g1-native-readiness-result.v1'
        generatedAtUtc = (Get-Date).ToUniversalTime().ToString('o')
        status = $Status
        certified = $Certified
        gate = 'G1'
        repositoryCommit = if ($Readiness) { [string]$Readiness.repositoryCommit } else { $null }
        branch = if ($Readiness) { [string]$Readiness.branch } else { $null }
        originMainCommit = if ($Readiness) { [string]$Readiness.originMainCommit } else { $null }
        expectedUnrealVersion = if ($Readiness) { [string]$Readiness.expectedUnrealVersion } else { '5.8.2' }
        actualUnrealVersion = if ($Readiness) { [string]$Readiness.actualUnrealVersion } else { $null }
        testFilter = 'WorldMakers.'
        checks = [ordered]@{
            readiness = ($Readiness -and $Readiness.status -eq 'ready')
            workstationDoctor = ($Readiness -and $Readiness.workstationDoctorStatus -eq 'ready')
            sourcePreflight = ($Readiness -and $Readiness.sourcePreflightStatus -eq 'passed')
            nativeBuild = ($Readiness -and $Readiness.nativeBuildStatus -eq 'passed' -and $Build -and $Build.status -eq 'passed')
            nativeAutomation = ($Readiness -and $Readiness.nativeAutomationStatus -eq 'passed' -and $Automation -and $Automation.status -eq 'passed')
            exactEngine = ($Readiness -and $Readiness.expectedUnrealVersion -eq '5.8.2' -and $Readiness.actualUnrealVersion -eq '5.8.2')
            completeAutomationNamespace = ($Readiness -and $Readiness.testFilter -eq 'WorldMakers.' -and $Automation -and $Automation.testFilter -eq 'WorldMakers.')
            authoredMapNotRequired = ($Readiness -and -not [bool]$Readiness.requireAuthoredMap)
            cleanCertificationMode = (-not $AllowNonMain -and -not $AllowDirtyWorktree)
            mainBranch = ($Readiness -and $Readiness.branch -eq 'main')
            originMainMatch = ($Readiness -and $Readiness.repositoryCommit -eq $Readiness.originMainCommit)
        }
        evidence = $EvidencePresence
        blockers = @($Blockers)
        boundary = [ordered]@{
            authoredMapRequired = $false
            authoredMapGate = 'G2'
            representativeDeviceEvidenceRequired = $false
            manualSmokeRequired = $false
        }
    } | ConvertTo-Json -Depth 8 | Set-Content -Path $G1Result -Encoding UTF8
}

if (-not (Test-Path $ReadinessScript -PathType Leaf)) {
    Write-G1Result -Status 'BLOCKED' -Certified $false -Blockers @('Missing scripts/run-unreal-readiness-gate.ps1')
    throw 'G1 certification cannot run because the readiness orchestrator is missing.'
}

$ReadinessArgs = @(
    '-EvidenceDir', $EvidenceDir,
    '-RunAutomation',
    '-TestFilter', 'WorldMakers.'
)
if (-not [string]::IsNullOrWhiteSpace($EngineRoot)) { $ReadinessArgs += @('-EngineRoot', $EngineRoot) }
if ($CleanIntermediate) { $ReadinessArgs += '-CleanIntermediate' }
if ($AllowNonMain) { $ReadinessArgs += '-AllowNonMain' }
if ($AllowDirtyWorktree) { $ReadinessArgs += '-AllowDirtyWorktree' }
if ($StopBlockingProcesses) { $ReadinessArgs += '-StopBlockingProcesses' }

try {
    & $ReadinessScript @ReadinessArgs
}
catch {
    $Readiness = Read-JsonIfPresent $ReadinessResult
    $Blockers = if ($Readiness) { @($Readiness.blockers) } else { @($_.Exception.Message) }
    Write-G1Result -Status 'BLOCKED' -Certified $false -Blockers $Blockers
    throw "G1 native Unreal certification is BLOCKED. Inspect $G1Result and the evidence directory before changing dependencies."
}

$Readiness = Read-JsonIfPresent $ReadinessResult
$Build = Read-JsonIfPresent (Join-Path $EvidencePath 'build-result.json')
$Automation = Read-JsonIfPresent (Join-Path $EvidencePath 'automation-result.json')

$NativePass = (
    $Readiness -and $Readiness.status -eq 'ready' -and
    $Readiness.workstationDoctorStatus -eq 'ready' -and
    $Readiness.sourcePreflightStatus -eq 'passed' -and
    $Readiness.nativeBuildStatus -eq 'passed' -and
    $Readiness.nativeAutomationStatus -eq 'passed' -and
    $Readiness.expectedUnrealVersion -eq '5.8.2' -and
    $Readiness.actualUnrealVersion -eq '5.8.2' -and
    -not [bool]$Readiness.requireAuthoredMap -and
    [bool]$Readiness.runAutomation -and
    $Readiness.testFilter -eq 'WorldMakers.' -and
    $Build -and $Build.status -eq 'passed' -and
    $Build.target -eq 'WorldMakersEditor' -and
    $Build.platform -eq 'Win64' -and
    $Build.configuration -eq 'Development' -and
    $Automation -and $Automation.status -eq 'passed' -and
    $Automation.testFilter -eq 'WorldMakers.'
)

$CertifyingContext = (
    -not $AllowNonMain -and
    -not $AllowDirtyWorktree -and
    $Readiness -and
    $Readiness.branch -eq 'main' -and
    $Readiness.repositoryCommit -eq $Readiness.originMainCommit
)

if (-not $NativePass) {
    Write-G1Result -Status 'BLOCKED' -Certified $false -Blockers @('Native readiness evidence did not satisfy the complete G1 contract.')
    throw "G1 native Unreal certification is BLOCKED. Inspect $G1Result."
}

if ($CertifyingContext) {
    Write-G1Result -Status 'CERTIFIED' -Certified $true
    Write-Host ''
    Write-Host 'WORLD MAKERS G1 NATIVE UNREAL READINESS: CERTIFIED' -ForegroundColor Green
    Write-Host "Evidence: $G1Result"
    Write-Host 'G2 is now the first gate that requires WM_PrototypeCertification.umap and authored Unreal assets.'
}
else {
    Write-G1Result -Status 'NON_CERTIFYING_PASS' -Certified $false -Blockers @('Native checks passed under development exceptions; only clean current main may certify G1.')
    Write-Host ''
    Write-Host 'WORLD MAKERS G1 NATIVE UNREAL READINESS: NON-CERTIFYING PASS' -ForegroundColor Yellow
    Write-Host "Evidence: $G1Result"
}
