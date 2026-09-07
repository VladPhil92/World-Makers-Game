param(
    [Parameter(Mandatory = $true)]
    [string]$EngineRoot,
    [string]$EvidenceDir = 'artifacts\certification',
    [switch]$RequireNativePass
)

$ErrorActionPreference = 'Stop'
$RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$EvidencePath = Join-Path $RepoRoot $EvidenceDir
New-Item -ItemType Directory -Force -Path $EvidencePath | Out-Null
$MapPath = Join-Path $RepoRoot 'game\Content\WorldMakers\Maps\WM_PrototypeCertification.umap'
$VersionFile = Join-Path $RepoRoot 'game\UNREAL_ENGINE_VERSION'
$BuildVersionFile = Join-Path $EngineRoot 'Engine\Build\Build.version'

function Read-JsonIfPresent([string]$Path) {
    if (Test-Path $Path) { return Get-Content $Path -Raw | ConvertFrom-Json }
    return $null
}

$Preflight = Read-JsonIfPresent (Join-Path $EvidencePath 'runner-preflight.json')
$Build = Read-JsonIfPresent (Join-Path $EvidencePath 'build-result.json')
$Automation = Read-JsonIfPresent (Join-Path $EvidencePath 'automation-result.json')
$ManualSmoke = Read-JsonIfPresent (Join-Path $EvidencePath 'manual-smoke.json')

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
    'manual-smoke.json'
)
foreach ($Name in $HashCandidates) {
    $Path = Join-Path $EvidencePath $Name
    if (Test-Path $Path) {
        $Hash = Get-FileHash -Algorithm SHA256 -Path $Path
        $HashEntries += [ordered]@{ file = $Name; sha256 = $Hash.Hash.ToLowerInvariant() }
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
