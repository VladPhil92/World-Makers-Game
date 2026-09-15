param(
    [string]$EvidenceRoot = "artifacts\pre-unreal",
    [switch]$AllowNonMain,
    [switch]$AllowDirtyWorktree
)

$ErrorActionPreference = "Stop"
$RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
Set-Location $RepoRoot

$Validator = Join-Path $RepoRoot "scripts\validate-pre-unreal-content-platform.py"
if (-not (Test-Path $Validator)) {
    throw "Missing Pre-Unreal validator: $Validator"
}

python $Validator
if ($LASTEXITCODE -ne 0) {
    throw "Pre-Unreal source validation failed."
}

$Head = (git rev-parse HEAD).Trim()
$Branch = (git rev-parse --abbrev-ref HEAD).Trim()
$Dirty = -not [string]::IsNullOrWhiteSpace((git status --porcelain | Out-String).Trim())

try {
    git fetch origin main --quiet
} catch {
    throw "Unable to fetch origin/main for Pre-Unreal handoff verification."
}

$OriginMain = (git rev-parse origin/main).Trim()
$IsCurrentMain = $Branch -eq "main" -and $Head -eq $OriginMain
$CertifyingContext = $IsCurrentMain -and (-not $Dirty) -and (-not $AllowNonMain) -and (-not $AllowDirtyWorktree)

if (-not $IsCurrentMain -and -not $AllowNonMain) {
    throw "Official Pre-Unreal readiness requires current main (HEAD == origin/main)."
}
if ($Dirty -and -not $AllowDirtyWorktree) {
    throw "Official Pre-Unreal readiness requires a clean worktree."
}

$OutputRoot = Join-Path $RepoRoot $EvidenceRoot
New-Item -ItemType Directory -Path $OutputRoot -Force | Out-Null
$OutputPath = Join-Path $OutputRoot "pre-unreal-readiness.json"

$Status = if ($CertifyingContext) { "READY_FOR_NATIVE_MATERIALIZATION" } else { "NON_CERTIFYING_PASS" }
$Result = [ordered]@{
    schema = "worldmakers.pre-unreal-readiness-result.v1"
    status = $Status
    sourceReady = $true
    repositoryCommit = $Head
    originMain = $OriginMain
    branch = $Branch
    cleanWorktree = (-not $Dirty)
    certifyingContext = $CertifyingContext
    engineVersion = "5.8.2"
    domains = @(
        "gameplay-runtime",
        "mission-science-content",
        "visual-animation-vfx",
        "audio",
        "input",
        "ui",
        "accessibility-localization",
        "platforms",
        "cloud-runtime",
        "packaging-certification"
    )
    truthBoundary = [ordered]@{
        nativeCompilationCertified = $false
        nativeAssetsCertified = $false
        nativeMapCertified = $false
        devicePerformanceCertified = $false
    }
    nextPhase = "native-unreal-materialization"
    generatedAtUtc = [DateTime]::UtcNow.ToString("o")
}

$Result | ConvertTo-Json -Depth 8 | Set-Content -Path $OutputPath -Encoding UTF8
Write-Host "Pre-Unreal readiness result: $Status"
Write-Host "Evidence: $OutputPath"

if ($Status -eq "READY_FOR_NATIVE_MATERIALIZATION") {
    exit 0
}
exit 0
