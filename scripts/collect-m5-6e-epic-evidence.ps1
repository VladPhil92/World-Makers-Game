param(
    [Parameter(Mandatory = $true)]
    [string]$EngineRoot,
    [string]$EvidenceDir = 'artifacts\m5-6e-epic',
    [switch]$RequireNativePass
)

$ErrorActionPreference = 'Stop'
$RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$EvidencePath = Join-Path $RepoRoot $EvidenceDir
New-Item -ItemType Directory -Force -Path $EvidencePath | Out-Null

$VersionFile = Join-Path $RepoRoot 'game\UNREAL_ENGINE_VERSION'
$BuildVersionFile = Join-Path $EngineRoot 'Engine\Build\Build.version'
$AuthoredMapPath = Join-Path $RepoRoot 'game\Content\WorldMakers\Maps\WM_PrototypeCertification.umap'
$PreflightPath = Join-Path $EvidencePath 'runner-preflight.json'
$BuildPath = Join-Path $EvidencePath 'build-result.json'
$AutomationPath = Join-Path $EvidencePath 'automation-result.json'
$AutomationLogPath = Join-Path $EvidencePath 'automation.log'
$ManifestPath = Join-Path $EvidencePath 'm5-6e-epic-certification-manifest.json'

$ExpectedFilter = 'WorldMakers.Epic.Persistence.'
$ExpectedTests = @(
    'WorldMakers.Epic.Persistence.ChapterCheckpointDropsPartialEvidence',
    'WorldMakers.Epic.Persistence.ResumeIndexFailClosed',
    'WorldMakers.Epic.Persistence.NativeSaveGameRoundTrip'
)

function Read-JsonIfPresent([string]$Path) {
    if (Test-Path $Path) { return Get-Content $Path -Raw | ConvertFrom-Json }
    return $null
}

function File-HashEntry([string]$Path, [string]$RelativeName) {
    if (-not (Test-Path $Path)) { return $null }
    $Hash = Get-FileHash -Algorithm SHA256 -Path $Path
    return [ordered]@{ file = $RelativeName; sha256 = $Hash.Hash.ToLowerInvariant() }
}

$Preflight = Read-JsonIfPresent $PreflightPath
$Build = Read-JsonIfPresent $BuildPath
$Automation = Read-JsonIfPresent $AutomationPath
$AutomationLog = if (Test-Path $AutomationLogPath) { Get-Content $AutomationLogPath -Raw } else { '' }

$ExpectedVersion = if (Test-Path $VersionFile) { (Get-Content $VersionFile -Raw).Trim() } else { $null }
$ActualVersion = $null
if (Test-Path $BuildVersionFile) {
    try {
        $BuildVersion = Get-Content $BuildVersionFile -Raw | ConvertFrom-Json
        $ActualVersion = "$($BuildVersion.MajorVersion).$($BuildVersion.MinorVersion).$($BuildVersion.PatchVersion)"
    }
    catch {
        $ActualVersion = $null
    }
}

$CommitSha = (& git -C $RepoRoot rev-parse HEAD 2>$null | Out-String).Trim()
$ObservedTests = @($ExpectedTests | Where-Object { $AutomationLog.Contains($_) })
$MissingTests = @($ExpectedTests | Where-Object { -not $AutomationLog.Contains($_) })
$ExactFilter = ($Automation -and $Automation.testFilter -eq $ExpectedFilter)
$AuthoredMapPresent = Test-Path $AuthoredMapPath
$VersionMatches = ($ExpectedVersion -and $ActualVersion -and $ExpectedVersion -eq $ActualVersion)
$PreflightPassed = ($Preflight -and $Preflight.status -eq 'ready' -and $Preflight.repositoryCommit -eq $CommitSha)
$BuildPassed = ($Build -and $Build.status -eq 'passed')
$AutomationPassed = ($Automation -and $Automation.status -eq 'passed')
$NativeHarnessPassed = (
    $PreflightPassed -and
    $BuildPassed -and
    $AutomationPassed -and
    $ExactFilter -and
    $MissingTests.Count -eq 0 -and
    $AuthoredMapPresent -and
    $VersionMatches
)

$Hashes = @()
foreach ($Pair in @(
    @($PreflightPath, 'runner-preflight.json'),
    @($BuildPath, 'build-result.json'),
    @(Join-Path $EvidencePath 'build.log', 'build.log'),
    @($AutomationPath, 'automation-result.json'),
    @($AutomationLogPath, 'automation.log')
)) {
    $Entry = File-HashEntry $Pair[0] $Pair[1]
    if ($Entry) { $Hashes += $Entry }
}

$MapHash = $null
if ($AuthoredMapPresent) {
    $MapHash = (Get-FileHash -Algorithm SHA256 -Path $AuthoredMapPath).Hash.ToLowerInvariant()
}

$Manifest = [ordered]@{
    schemaVersion = 1
    generatedAtUtc = (Get-Date).ToUniversalTime().ToString('o')
    repositoryCommit = $CommitSha
    expectedUnrealVersion = $ExpectedVersion
    actualUnrealVersion = $ActualVersion
    exactTestFilter = $ExpectedFilter
    authoredCertificationMap = [ordered]@{
        path = 'game/Content/WorldMakers/Maps/WM_PrototypeCertification.umap'
        present = $AuthoredMapPresent
        sha256 = $MapHash
    }
    nativeChecks = [ordered]@{
        runnerPreflight = if ($PreflightPassed) { 'passed' } else { 'blocked-or-failed' }
        editorBuild = if ($BuildPassed) { 'passed' } else { 'blocked-or-failed' }
        epicAutomation = if ($AutomationPassed -and $ExactFilter) { 'passed' } else { 'blocked-or-failed' }
        saveGameRoundTripObserved = $ObservedTests -contains 'WorldMakers.Epic.Persistence.NativeSaveGameRoundTrip'
        exactUnrealVersion = $VersionMatches
    }
    expectedTests = $ExpectedTests
    observedTests = $ObservedTests
    missingTests = $MissingTests
    privacyBoundary = [ordered]@{
        persistsChapterCheckpointOnly = $true
        persistsPartialEvidence = $false
        persistsFreeText = $false
        persistsMoralChoice = $false
        persistsPersonalityOrIdeology = $false
    }
    nativeEpicContinuity = [ordered]@{
        passed = $NativeHarnessPassed
        status = if ($NativeHarnessPassed) { 'passed' } else { 'blocked-or-failed' }
        scope = 'UE editor build plus WorldMakers.Epic.Persistence native automation only; packaged-device and production telemetry certification remain separate.'
    }
    evidenceHashes = $Hashes
}

$Manifest | ConvertTo-Json -Depth 8 | Set-Content -Path $ManifestPath -Encoding UTF8
$Manifest | ConvertTo-Json -Depth 8 | Write-Host

if ($RequireNativePass -and -not $NativeHarnessPassed) {
    throw 'M5.6E native epic continuity evidence is incomplete or failed. See m5-6e-epic-certification-manifest.json.'
}
