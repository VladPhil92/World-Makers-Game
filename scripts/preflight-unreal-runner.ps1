param(
    [Parameter(Mandatory = $true)]
    [string]$EngineRoot,
    [string]$EvidenceDir = 'artifacts\certification',
    [switch]$RequireAuthoredMap
)

$ErrorActionPreference = 'Stop'
$RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$EvidencePath = Join-Path $RepoRoot $EvidenceDir
New-Item -ItemType Directory -Force -Path $EvidencePath | Out-Null

$ExpectedVersionFile = Join-Path $RepoRoot 'game\UNREAL_ENGINE_VERSION'
$BuildVersionFile = Join-Path $EngineRoot 'Engine\Build\Build.version'
$BuildBat = Join-Path $EngineRoot 'Engine\Build\BatchFiles\Build.bat'
$EditorCmd = Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
$CertificationMap = Join-Path $RepoRoot 'game\Content\WorldMakers\Maps\WM_PrototypeCertification.umap'

$checks = [ordered]@{}
$checks.expectedVersionFile = Test-Path $ExpectedVersionFile
$checks.buildVersionFile = Test-Path $BuildVersionFile
$checks.buildBat = Test-Path $BuildBat
$checks.editorCmd = Test-Path $EditorCmd
$checks.authoredMap = Test-Path $CertificationMap

$ExpectedVersion = if ($checks.expectedVersionFile) { (Get-Content $ExpectedVersionFile -Raw).Trim() } else { $null }
$ActualVersion = $null
if ($checks.buildVersionFile) {
    try {
        $BuildVersion = Get-Content $BuildVersionFile -Raw | ConvertFrom-Json
        $ActualVersion = "$($BuildVersion.MajorVersion).$($BuildVersion.MinorVersion).$($BuildVersion.PatchVersion)"
    }
    catch {
        $ActualVersion = $null
    }
}
$checks.versionMatches = ($ExpectedVersion -and $ActualVersion -and $ExpectedVersion -eq $ActualVersion)

$GitCommand = Get-Command git -ErrorAction SilentlyContinue
$checks.git = ($null -ne $GitCommand)
$GitLfsOutput = $null
$CommitSha = $null
if ($checks.git) {
    $GitLfsOutput = (& git lfs version 2>&1 | Out-String).Trim()
    $checks.gitLfs = ($LASTEXITCODE -eq 0 -and -not [string]::IsNullOrWhiteSpace($GitLfsOutput))
    $CommitSha = (& git -C $RepoRoot rev-parse HEAD 2>$null | Out-String).Trim()
    $checks.gitCommit = ($LASTEXITCODE -eq 0 -and $CommitSha -match '^[0-9a-f]{40}$')
}
else {
    $checks.gitLfs = $false
    $checks.gitCommit = $false
}

$RequiredChecks = @('expectedVersionFile', 'buildVersionFile', 'buildBat', 'editorCmd', 'versionMatches', 'git', 'gitLfs', 'gitCommit')
if ($RequireAuthoredMap) {
    $RequiredChecks += 'authoredMap'
}
$Failed = @($RequiredChecks | Where-Object { -not $checks[$_] })

$Report = [ordered]@{
    schemaVersion = 1
    generatedAtUtc = (Get-Date).ToUniversalTime().ToString('o')
    status = if ($Failed.Count -eq 0) { 'ready' } else { 'blocked' }
    repositoryCommit = $CommitSha
    engineRoot = $EngineRoot
    expectedUnrealVersion = $ExpectedVersion
    actualUnrealVersion = $ActualVersion
    certificationMap = 'game/Content/WorldMakers/Maps/WM_PrototypeCertification.umap'
    gitLfs = $GitLfsOutput
    checks = $checks
    failedChecks = $Failed
}

$ReportPath = Join-Path $EvidencePath 'runner-preflight.json'
$Report | ConvertTo-Json -Depth 6 | Set-Content -Path $ReportPath -Encoding UTF8
$Report | ConvertTo-Json -Depth 6 | Write-Host

if ($Failed.Count -gt 0) {
    throw "Unreal runner preflight blocked: $($Failed -join ', ')"
}
