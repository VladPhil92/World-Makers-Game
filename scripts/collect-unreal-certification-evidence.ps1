param(
    [Parameter(Mandatory = $true)]
    [string]$EngineRoot,
    [string]$EvidenceDir = 'artifacts\certification',
    [string]$ManualSmokeEvidenceFile = '',
    [string]$VerticalSliceEvidenceFile = '',
    [string]$DeviceEvidenceRoot = '',
    [switch]$RequireNativePass
)

$ErrorActionPreference = 'Stop'
$RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$EvidencePath = Join-Path $RepoRoot $EvidenceDir
New-Item -ItemType Directory -Force -Path $EvidencePath | Out-Null
$MapPath = Join-Path $RepoRoot 'game\Content\WorldMakers\Maps\WM_PrototypeCertification.umap'
$VersionFile = Join-Path $RepoRoot 'game\UNREAL_ENGINE_VERSION'
$BuildVersionFile = Join-Path $EngineRoot 'Engine\Build\Build.version'
$PerformanceSourcePath = Join-Path $RepoRoot 'game\Saved\WorldMakers\Performance'
$PerformanceEvidencePath = Join-Path $EvidencePath 'performance'
$DeviceEvidencePath = Join-Path $EvidencePath 'devices'

if ([string]::IsNullOrWhiteSpace($VerticalSliceEvidenceFile)) {
    $VerticalSliceEvidenceFile = Join-Path $RepoRoot 'game\Saved\WorldMakers\Certification\vertical-slice-route.json'
}
if ([string]::IsNullOrWhiteSpace($DeviceEvidenceRoot)) {
    $DeviceEvidenceRoot = Join-Path $RepoRoot 'game\Saved\WorldMakers\CertificationDevices'
}

function Read-JsonIfPresent([string]$Path) {
    if (Test-Path $Path) { return Get-Content $Path -Raw | ConvertFrom-Json }
    return $null
}

function Copy-JsonDirectory([string]$Source, [string]$Destination) {
    $Copied = @()
    if (Test-Path $Source) {
        New-Item -ItemType Directory -Force -Path $Destination | Out-Null
        foreach ($Item in Get-ChildItem -Path $Source -Filter '*.json' -File | Sort-Object Name) {
            Copy-Item -Force -Path $Item.FullName -Destination (Join-Path $Destination $Item.Name)
            $Copied += $Item.Name
        }
    }
    return $Copied
}

if (-not [string]::IsNullOrWhiteSpace($ManualSmokeEvidenceFile) -and (Test-Path $ManualSmokeEvidenceFile)) {
    Copy-Item -Force -Path $ManualSmokeEvidenceFile -Destination (Join-Path $EvidencePath 'manual-smoke.json')
}
if (-not [string]::IsNullOrWhiteSpace($VerticalSliceEvidenceFile) -and (Test-Path $VerticalSliceEvidenceFile)) {
    Copy-Item -Force -Path $VerticalSliceEvidenceFile -Destination (Join-Path $EvidencePath 'vertical-slice-route.json')
}

$PerformanceCaptureFiles = @()
$PerformanceCaptureFiles += Copy-JsonDirectory $PerformanceSourcePath $PerformanceEvidencePath

$DeviceEvidenceFiles = @()
if (Test-Path $DeviceEvidenceRoot) {
    $DeviceEvidenceFiles += Copy-JsonDirectory (Join-Path $DeviceEvidenceRoot 'devices') $DeviceEvidencePath
    $PerformanceCaptureFiles += Copy-JsonDirectory (Join-Path $DeviceEvidenceRoot 'performance') $PerformanceEvidencePath
}
$PerformanceCaptureFiles = @($PerformanceCaptureFiles | Sort-Object -Unique)
$DeviceEvidenceFiles = @($DeviceEvidenceFiles | Sort-Object -Unique)

$Preflight = Read-JsonIfPresent (Join-Path $EvidencePath 'runner-preflight.json')
$Build = Read-JsonIfPresent (Join-Path $EvidencePath 'build-result.json')
$Automation = Read-JsonIfPresent (Join-Path $EvidencePath 'automation-result.json')
$ManualSmoke = Read-JsonIfPresent (Join-Path $EvidencePath 'manual-smoke.json')
$VerticalSliceRoute = Read-JsonIfPresent (Join-Path $EvidencePath 'vertical-slice-route.json')

$ExpectedVersion = if (Test-Path $VersionFile) { (Get-Content $VersionFile -Raw).Trim() } else { $null }
$ActualVersion = $null
if (Test-Path $BuildVersionFile) {
    $BuildVersion = Get-Content $BuildVersionFile -Raw | ConvertFrom-Json
    $ActualVersion = "$($BuildVersion.MajorVersion).$($BuildVersion.MinorVersion).$($BuildVersion.PatchVersion)"
}
$CommitSha = (& git -C $RepoRoot rev-parse HEAD 2>$null | Out-String).Trim()

$NativePassed = (
    $Preflight -and $Preflight.status -eq 'ready' -and
    $Build -and $Build.status -eq 'passed' -and
    $Automation -and $Automation.status -eq 'passed' -and
    (Test-Path $MapPath) -and
    $ExpectedVersion -and $ActualVersion -eq $ExpectedVersion
)
$ManualPassed = ($ManualSmoke -and $ManualSmoke.status -eq 'passed')
$RuntimeCertified = ($NativePassed -and $ManualPassed)

$HashEntries = @()
$HashCandidates = @(
    'runner-preflight.json',
    'build-result.json',
    'build.log',
    'automation-result.json',
    'automation.log',
    'manual-smoke.json',
    'vertical-slice-route.json'
)
foreach ($Name in $HashCandidates) {
    $Path = Join-Path $EvidencePath $Name
    if (Test-Path $Path) {
        $Hash = Get-FileHash -Algorithm SHA256 -Path $Path
        $HashEntries += [ordered]@{ file = $Name; sha256 = $Hash.Hash.ToLowerInvariant() }
    }
}
foreach ($Name in $PerformanceCaptureFiles) {
    $Path = Join-Path $PerformanceEvidencePath $Name
    if (Test-Path $Path) {
        $Hash = Get-FileHash -Algorithm SHA256 -Path $Path
        $HashEntries += [ordered]@{ file = "performance/$Name"; sha256 = $Hash.Hash.ToLowerInvariant() }
    }
}
foreach ($Name in $DeviceEvidenceFiles) {
    $Path = Join-Path $DeviceEvidencePath $Name
    if (Test-Path $Path) {
        $Hash = Get-FileHash -Algorithm SHA256 -Path $Path
        $HashEntries += [ordered]@{ file = "devices/$Name"; sha256 = $Hash.Hash.ToLowerInvariant() }
    }
}

$MapHash = $null
if (Test-Path $MapPath) {
    $MapHash = (Get-FileHash -Algorithm SHA256 -Path $MapPath).Hash.ToLowerInvariant()
}

$Manifest = [ordered]@{
    schemaVersion = 1
    generatedAtUtc = (Get-Date).ToUniversalTime().ToString('o')
    repositoryCommit = $CommitSha
    expectedUnrealVersion = $ExpectedVersion
    actualUnrealVersion = $ActualVersion
    authoredMap = [ordered]@{
        path = 'game/Content/WorldMakers/Maps/WM_PrototypeCertification.umap'
        present = (Test-Path $MapPath)
        sha256 = $MapHash
    }
    nativeAutomation = [ordered]@{
        status = if ($NativePassed) { 'passed' } else { 'blocked-or-failed' }
        testFilter = if ($Automation) { $Automation.testFilter } else { 'WorldMakers.' }
    }
    manualSmoke = [ordered]@{
        status = if ($ManualPassed) { 'passed' } else { 'pending' }
    }
    performanceCapture = [ordered]@{
        present = ($PerformanceCaptureFiles.Count -gt 0)
        requiredForM3VerticalSlice = $true
        enforcedByCurrentM1NativeGate = $false
        source = 'game/Saved/WorldMakers/Performance/*.json and M3 device evidence bundle'
        files = $PerformanceCaptureFiles
    }
    m3VerticalSliceEvidence = [ordered]@{
        assessmentRequired = $true
        routePresent = ($null -ne $VerticalSliceRoute)
        routeStatus = if ($VerticalSliceRoute) { $VerticalSliceRoute.status } else { 'pending' }
        deviceEvidencePresent = ($DeviceEvidenceFiles.Count -gt 0)
        deviceEvidenceFiles = $DeviceEvidenceFiles
        assessor = 'scripts/assess-m3-8-certification.py'
    }
    runtimeCertification = [ordered]@{
        certified = $RuntimeCertified
        status = if ($RuntimeCertified) { 'certified' } elseif ($NativePassed) { 'native-pass-manual-smoke-pending' } else { 'blocked' }
    }
    evidenceHashes = $HashEntries
}

$ManifestPath = Join-Path $EvidencePath 'certification-manifest.json'
$Manifest | ConvertTo-Json -Depth 8 | Set-Content -Path $ManifestPath -Encoding UTF8
$Manifest | ConvertTo-Json -Depth 8 | Write-Host

if ($RequireNativePass -and -not $NativePassed) {
    throw 'Native Unreal certification evidence is incomplete or failed. See certification-manifest.json.'
}
