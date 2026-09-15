param(
    [Parameter(Mandatory = $true)]
    [ValidateSet('windows-reference','android-alpha','ipados-alpha')]
    [string]$TargetId,
    [string]$EngineRoot,
    [string]$ArchiveRoot = 'artifacts\production-alpha\package',
    [ValidateSet('pending','verified','internal-not-required')]
    [string]$SigningStatus = 'pending',
    [string]$SigningIdentity = '',
    [switch]$AllowNonMain,
    [switch]$AllowDirtyWorktree
)

$ErrorActionPreference = 'Stop'
$RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$ContractPath = Join-Path $RepoRoot 'content\production\production-alpha-readiness-v1.json'
$ResolveEngineScript = Join-Path $RepoRoot 'scripts\resolve-unreal-engine.ps1'
$ProjectPath = Join-Path $RepoRoot 'game\WorldMakers.uproject'
$VersionFile = Join-Path $RepoRoot 'game\UNREAL_ENGINE_VERSION'

function Get-GitValue([string[]]$Args) {
    $Value = (& git -C $RepoRoot @Args 2>$null | Out-String).Trim()
    if ($LASTEXITCODE -ne 0) { return $null }
    return $Value
}

function Get-Sha256([string]$Path) {
    return (Get-FileHash -Algorithm SHA256 $Path).Hash.ToLowerInvariant()
}

if (-not (Test-Path $ContractPath -PathType Leaf)) { throw 'Production Alpha contract is missing.' }
if (-not (Test-Path $ProjectPath -PathType Leaf)) { throw 'WorldMakers.uproject is missing.' }
if (-not (Test-Path $VersionFile -PathType Leaf)) { throw 'UNREAL_ENGINE_VERSION is missing.' }

$Contract = Get-Content $ContractPath -Raw | ConvertFrom-Json
$ExpectedVersion = (Get-Content $VersionFile -Raw).Trim()
if ($ExpectedVersion -ne [string]$Contract.engine.expectedVersion) {
    throw "Engine lock mismatch. Contract expects $($Contract.engine.expectedVersion); repository locks $ExpectedVersion."
}
$Target = @($Contract.requiredBuildTargets | Where-Object { $_.id -eq $TargetId })
if ($Target.Count -ne 1) { throw "Unknown or duplicate target id: $TargetId" }
$Target = $Target[0]

& git -C $RepoRoot fetch origin main --quiet
if ($LASTEXITCODE -ne 0) { throw 'Unable to fetch origin/main.' }
$Commit = Get-GitValue @('rev-parse','HEAD')
$Branch = Get-GitValue @('rev-parse','--abbrev-ref','HEAD')
$OriginMain = Get-GitValue @('rev-parse','origin/main')
$Dirty = @(& git -C $RepoRoot status --porcelain --untracked-files=all)
if (-not $AllowNonMain -and $Branch -ne 'main') { throw 'Certified alpha packaging must run from main. Use -AllowNonMain only for non-certifying development packages.' }
if (-not $AllowNonMain -and $Commit -ne $OriginMain) { throw 'Certified alpha packaging requires HEAD == origin/main.' }
if (-not $AllowDirtyWorktree -and $Dirty.Count -gt 0) { throw 'Certified alpha packaging requires a clean worktree.' }

if (-not (Test-Path $ResolveEngineScript -PathType Leaf)) { throw 'Unreal engine resolver is missing.' }
$Resolution = & $ResolveEngineScript -RequestedRoot $EngineRoot -ExpectedVersion $ExpectedVersion
if ($null -eq $Resolution -or [string]::IsNullOrWhiteSpace([string]$Resolution.path)) { throw 'Unable to resolve locked Unreal Engine.' }
$ResolvedEngineRoot = [string]$Resolution.path

$HostIsWindows = $false
$HostIsMacOS = $false
if (Get-Variable IsWindows -ErrorAction SilentlyContinue) { $HostIsWindows = [bool]$IsWindows }
if (Get-Variable IsMacOS -ErrorAction SilentlyContinue) { $HostIsMacOS = [bool]$IsMacOS }
if ($env:OS -eq 'Windows_NT') { $HostIsWindows = $true }

$Platform = [string]$Target.unrealPlatform
if (($Platform -eq 'Win64' -or $Platform -eq 'Android') -and -not $HostIsWindows) {
    throw "$Platform alpha packaging is assigned to a Windows Unreal build host."
}
if ($Platform -eq 'IOS' -and -not $HostIsMacOS) {
    throw 'iPadOS alpha packaging requires the macOS/Xcode Unreal build lane.'
}

$RunUAT = if ($HostIsWindows) {
    Join-Path $ResolvedEngineRoot 'Engine\Build\BatchFiles\RunUAT.bat'
} else {
    Join-Path $ResolvedEngineRoot 'Engine/Build/BatchFiles/RunUAT.sh'
}
if (-not (Test-Path $RunUAT -PathType Leaf)) { throw "RunUAT not found: $RunUAT" }

$ArchiveBase = Join-Path $RepoRoot $ArchiveRoot
$ArchivePath = Join-Path $ArchiveBase $TargetId
if (Test-Path $ArchivePath) { Remove-Item $ArchivePath -Recurse -Force }
New-Item -ItemType Directory -Force -Path $ArchivePath | Out-Null

$Args = @(
    'BuildCookRun',
    "-project=$ProjectPath",
    '-noP4',
    '-utf8output',
    '-unattended',
    '-build',
    '-cook',
    '-stage',
    '-pak',
    '-package',
    '-archive',
    "-archivedirectory=$ArchivePath",
    "-target=$($Contract.engine.gameTarget)",
    "-platform=$Platform",
    "-clientconfig=$($Target.configuration)"
)
if ($TargetId -eq 'android-alpha') { $Args += '-cookflavor=ASTC' }

Write-Host "Packaging World Makers Production Alpha target '$TargetId' ($Platform/$($Target.configuration))..."
& $RunUAT @Args
if ($LASTEXITCODE -ne 0) { throw "RunUAT BuildCookRun failed for $TargetId with exit code $LASTEXITCODE." }

$ManifestDir = Join-Path $RepoRoot 'artifacts\production-alpha\builds'
New-Item -ItemType Directory -Force -Path $ManifestDir | Out-Null
$ManifestPath = Join-Path $ManifestDir "$TargetId.json"
$Files = @(Get-ChildItem $ArchivePath -File -Recurse | Sort-Object FullName)
if ($Files.Count -eq 0) { throw "Packaging succeeded but produced no files under $ArchivePath." }

$Rows = @()
$AggregateLines = @()
$TotalBytes = [int64]0
foreach ($File in $Files) {
    $Relative = [IO.Path]::GetRelativePath($ArchivePath, $File.FullName).Replace('\','/')
    $Hash = Get-Sha256 $File.FullName
    $Size = [int64]$File.Length
    $TotalBytes += $Size
    $Rows += [ordered]@{ path = $Relative; bytes = $Size; sha256 = $Hash }
    $AggregateLines += "$Relative|$Size|$Hash"
}
$AggregateText = ($AggregateLines -join "`n") + "`n"
$Sha = [System.Security.Cryptography.SHA256]::Create()
try {
    $AggregateHash = ([BitConverter]::ToString($Sha.ComputeHash([Text.Encoding]::UTF8.GetBytes($AggregateText)))).Replace('-','').ToLowerInvariant()
} finally { $Sha.Dispose() }

if ($SigningStatus -eq 'verified' -and [string]::IsNullOrWhiteSpace($SigningIdentity)) {
    throw 'SigningIdentity is required when SigningStatus is verified.'
}
$CertifyingContext = (-not $AllowNonMain -and -not $AllowDirtyWorktree -and $Branch -eq 'main' -and $Commit -eq $OriginMain -and $Dirty.Count -eq 0)
$BuildId = "wm-alpha-$($Commit.Substring(0,12))-$TargetId"
$Manifest = [ordered]@{
    schema = 'worldmakers.alpha-build-manifest.v1'
    status = 'built'
    buildId = $BuildId
    releaseChannel = 'internal-alpha'
    repositoryCommit = $Commit
    engineVersion = $ExpectedVersion
    targetId = $TargetId
    unrealPlatform = $Platform
    configuration = [string]$Target.configuration
    buildHost = [ordered]@{
        os = [System.Runtime.InteropServices.RuntimeInformation]::OSDescription
        runner = $env:RUNNER_NAME
        machineClass = [System.Runtime.InteropServices.RuntimeInformation]::OSArchitecture.ToString()
    }
    builtAtUtc = (Get-Date).ToUniversalTime().ToString('o')
    archiveRoot = $ArchivePath
    artifactFiles = $Rows
    aggregateArtifactSha256 = $AggregateHash
    aggregateArtifactBytes = $TotalBytes
    signing = [ordered]@{
        requiredForExternalDistribution = ($TargetId -ne 'windows-reference')
        status = $SigningStatus
        identity = $SigningIdentity
    }
    source = [ordered]@{
        branch = $Branch
        originMainCommit = $OriginMain
        cleanWorktree = ($Dirty.Count -eq 0)
        certifyingContext = $CertifyingContext
    }
}
$Manifest | ConvertTo-Json -Depth 8 | Set-Content -Path $ManifestPath -Encoding UTF8
Write-Host "Production Alpha package complete: $ArchivePath"
Write-Host "Build manifest: $ManifestPath"
