param(
    [string]$EngineRoot,
    [string]$EvidenceDir = 'artifacts\native-materialization',
    [switch]$AllowPreUnrealBlockedForDevelopment,
    [switch]$AllowNonMain,
    [switch]$AllowDirtyWorktree,
    [switch]$StopBlockingProcesses
)

$ErrorActionPreference = 'Stop'
$RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$EvidencePath = Join-Path $RepoRoot $EvidenceDir
$ResultPath = Join-Path $EvidencePath 'native-materialization-entry.json'
$PreUnrealResultPath = Join-Path $RepoRoot 'artifacts\pre-unreal-handoff\final-pre-unreal-handoff.json'
$G1Script = Join-Path $RepoRoot 'scripts\run-g1-native-certification.ps1'
$G2Script = Join-Path $RepoRoot 'scripts\run-g2-authored-certification.ps1'
$G1EvidenceDir = Join-Path $EvidenceDir 'g1'
$G2EvidenceDir = Join-Path $EvidenceDir 'g2'
$G1ResultPath = Join-Path $RepoRoot (Join-Path $G1EvidenceDir 'g1-native-readiness.json')
$G2AuthorReportPath = Join-Path $RepoRoot (Join-Path $G2EvidenceDir 'g2-author-report.json')
$MapGitPath = 'game/Content/WorldMakers/Maps/WM_PrototypeCertification.umap'
$MapDiskPath = Join-Path $RepoRoot ($MapGitPath -replace '/', '\')

New-Item -ItemType Directory -Force -Path $EvidencePath | Out-Null
if (Test-Path $ResultPath -PathType Leaf) { Remove-Item $ResultPath -Force }

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

function Invoke-ChildPowerShell([string]$ScriptPath, [string[]]$Arguments, [string]$Label) {
    & powershell.exe -NoLogo -NoProfile -ExecutionPolicy Bypass -File $ScriptPath @Arguments
    if ($LASTEXITCODE -ne 0) {
        throw "$Label failed with exit code $LASTEXITCODE."
    }
}

function Test-PreUnrealAuthorization([object]$PreUnreal, [string]$Commit) {
    return [bool](
        $PreUnreal -and
        $PreUnreal.schema -eq 'worldmakers.final-pre-unreal-handoff-result.v1' -and
        $PreUnreal.status -eq 'PRE_UNREAL_READY' -and
        [bool]$PreUnreal.preUnrealReady -and
        $PreUnreal.repositoryCommit -eq $Commit
    )
}

function Write-EntryResult {
    param(
        [Parameter(Mandatory = $true)][string]$Status,
        [string[]]$Blockers = @(),
        [object]$PreUnreal = $null,
        [object]$G1 = $null,
        [bool]$PreUnrealAuthorized = $false,
        [bool]$DevelopmentOverrideUsed = $false
    )

    $Commit = Get-GitValue @('rev-parse', 'HEAD')
    $Branch = Get-GitValue @('rev-parse', '--abbrev-ref', 'HEAD')
    $OriginMain = Get-GitValue @('rev-parse', 'origin/main')
    $MapPresent = Test-Path $MapDiskPath -PathType Leaf
    $MapHash = if ($MapPresent) { (Get-FileHash -Algorithm SHA256 $MapDiskPath).Hash.ToLowerInvariant() } else { $null }
    $LfsOutput = if ($MapPresent) { (& git -C $RepoRoot check-attr filter -- $MapGitPath 2>$null | Out-String).Trim() } else { '' }
    $LfsTracked = $MapPresent -and $LfsOutput -match 'filter:\s*lfs'

    [ordered]@{
        schema = 'worldmakers.native-materialization-entry-result.v1'
        generatedAtUtc = (Get-Date).ToUniversalTime().ToString('o')
        status = $Status
        repositoryCommit = $Commit
        branch = $Branch
        originMainCommit = $OriginMain
        preUnreal = [ordered]@{
            authorized = $PreUnrealAuthorized
            developmentOverrideUsed = $DevelopmentOverrideUsed
            status = if ($PreUnreal) { [string]$PreUnreal.status } else { $null }
            repositoryCommit = if ($PreUnreal) { [string]$PreUnreal.repositoryCommit } else { $null }
            evidencePath = if (Test-Path $PreUnrealResultPath -PathType Leaf) { 'artifacts/pre-unreal-handoff/final-pre-unreal-handoff.json' } else { $null }
        }
        g1 = [ordered]@{
            status = if ($G1) { [string]$G1.status } else { $null }
            certified = if ($G1) { [bool]$G1.certified } else { $false }
            evidencePath = if (Test-Path $G1ResultPath -PathType Leaf) { ($G1ResultPath.Substring($RepoRoot.Length + 1) -replace '\\','/') } else { $null }
        }
        authoredMap = [ordered]@{
            packagePath = '/Game/WorldMakers/Maps/WM_PrototypeCertification'
            diskPath = $MapGitPath
            present = $MapPresent
            gitLfsTracked = $LfsTracked
            sha256 = $MapHash
            authorReport = if (Test-Path $G2AuthorReportPath -PathType Leaf) { ($G2AuthorReportPath.Substring($RepoRoot.Length + 1) -replace '\\','/') } else { $null }
        }
        blockers = @($Blockers)
        nextActions = if ($Status -eq 'AUTHORING_COMPLETE_COMMIT_REQUIRED') { @(
            'Open WM_PrototypeCertification in UE 5.8.2 and perform native visual/gameplay review.',
            'Import and review the nine P2 Caribbean Rainforest authored assets before setting authoredPresent=true.',
            'Verify git lfs status and commit the real .umap/.uasset binaries through Git LFS.',
            'Create passed G2 manual route evidence bound to the committed HEAD.',
            'Run WorldMakers-G2-Certify.cmd from clean current main.'
        ) } else { @() }
        truthBoundary = 'AUTHORING_COMPLETE_COMMIT_REQUIRED means Unreal created/reconciled the real map after an acceptable native G1 pass; it is not G2 certification, visual certification, device certification, or a production build.'
    } | ConvertTo-Json -Depth 8 | Set-Content -Path $ResultPath -Encoding UTF8
}

$PreUnreal = $null
$G1 = $null
$PreUnrealAuthorized = $false
$DevelopmentOverrideUsed = $false

try {
    & git -C $RepoRoot fetch origin main --quiet
    if ($LASTEXITCODE -ne 0) { throw 'Unable to fetch origin/main before native materialization entry.' }

    $Commit = Get-GitValue @('rev-parse', 'HEAD')
    $Branch = Get-GitValue @('rev-parse', '--abbrev-ref', 'HEAD')
    $OriginMain = Get-GitValue @('rev-parse', 'origin/main')
    $WorkingTree = @(& git -C $RepoRoot status --porcelain --untracked-files=all)
    $PreUnreal = Read-JsonIfPresent $PreUnrealResultPath
    $PreUnrealAuthorized = Test-PreUnrealAuthorization $PreUnreal $Commit
    $DevelopmentOverrideUsed = -not $PreUnrealAuthorized -and [bool]$AllowPreUnrealBlockedForDevelopment

    if (-not $PreUnrealAuthorized -and -not $DevelopmentOverrideUsed) {
        $Observed = if ($PreUnreal) { [string]$PreUnreal.status } else { 'missing' }
        throw "Final Pre-Unreal handoff has not authorized native materialization for this commit (observed: $Observed). Finish public HTTPS + authenticated CTG One E2E closure and run WorldMakers-FinalPreUnreal-Certify.cmd, or use -AllowPreUnrealBlockedForDevelopment only for non-certifying local development."
    }

    if ($PreUnrealAuthorized) {
        if ($Branch -ne 'main' -or $Commit -ne $OriginMain -or $WorkingTree.Count -ne 0) {
            throw 'Official native materialization entry requires clean current main matching origin/main.'
        }
        Write-Host 'Final Pre-Unreal handoff: PRE_UNREAL_READY for current HEAD.' -ForegroundColor Green
    }
    else {
        Write-Warning 'Pre-Unreal closure is not officially ready. Development override is active; this run cannot be used as official handoff or G2 certification evidence.'
    }

    if (-not (Test-Path $G1Script -PathType Leaf)) { throw 'Missing G1 native certification runner.' }
    if (-not (Test-Path $G2Script -PathType Leaf)) { throw 'Missing G2 authored certification runner.' }

    $G1Args = @('-EvidenceDir', $G1EvidenceDir)
    if (-not [string]::IsNullOrWhiteSpace($EngineRoot)) { $G1Args += @('-EngineRoot', $EngineRoot) }
    if ($AllowNonMain) { $G1Args += '-AllowNonMain' }
    if ($AllowDirtyWorktree) { $G1Args += '-AllowDirtyWorktree' }
    if ($StopBlockingProcesses) { $G1Args += '-StopBlockingProcesses' }
    Invoke-ChildPowerShell $G1Script $G1Args 'G1 native certification runner'

    $G1 = Read-JsonIfPresent $G1ResultPath
    if ($null -eq $G1) { throw 'G1 result evidence is missing after the native run.' }

    if ($PreUnrealAuthorized) {
        if ($G1.status -ne 'CERTIFIED' -or -not [bool]$G1.certified) {
            throw "Official materialization requires G1 CERTIFIED; observed '$($G1.status)'."
        }
    }
    elseif ($G1.status -notin @('CERTIFIED', 'NON_CERTIFYING_PASS')) {
        throw "Development materialization requires an acceptable native G1 pass; observed '$($G1.status)'."
    }

    $G2Args = @('-EvidenceDir', $G2EvidenceDir, '-AuthorMap')
    if (-not [string]::IsNullOrWhiteSpace($EngineRoot)) { $G2Args += @('-EngineRoot', $EngineRoot) }
    if ($AllowNonMain) { $G2Args += '-AllowNonMain' }
    if ($AllowDirtyWorktree) { $G2Args += '-AllowDirtyWorktree' }
    if ($StopBlockingProcesses) { $G2Args += '-StopBlockingProcesses' }
    Invoke-ChildPowerShell $G2Script $G2Args 'G2 Unreal map authoring runner'

    if (-not (Test-Path $MapDiskPath -PathType Leaf)) {
        throw 'Unreal returned from G2 authoring without creating WM_PrototypeCertification.umap.'
    }

    $LfsOutput = (& git -C $RepoRoot check-attr filter -- $MapGitPath 2>$null | Out-String).Trim()
    if ($LASTEXITCODE -ne 0 -or $LfsOutput -notmatch 'filter:\s*lfs') {
        throw 'WM_PrototypeCertification.umap is not covered by Git LFS filter=lfs.'
    }

    Write-EntryResult -Status 'AUTHORING_COMPLETE_COMMIT_REQUIRED' -PreUnreal $PreUnreal -G1 $G1 -PreUnrealAuthorized $PreUnrealAuthorized -DevelopmentOverrideUsed $DevelopmentOverrideUsed

    Write-Host ''
    Write-Host 'WORLD MAKERS NATIVE MATERIALIZATION: AUTHORING COMPLETE' -ForegroundColor Green
    Write-Host 'The real WM_PrototypeCertification.umap now exists in the working tree and is covered by Git LFS.'
    Write-Host 'Do not call G2 certified yet: review the map/assets in UE, commit the binaries through LFS, complete the manual route review, then run WorldMakers-G2-Certify.cmd.'
    Write-Host "Evidence: $ResultPath"
}
catch {
    Write-EntryResult -Status 'BLOCKED' -Blockers @($_.Exception.Message) -PreUnreal $PreUnreal -G1 $G1 -PreUnrealAuthorized $PreUnrealAuthorized -DevelopmentOverrideUsed $DevelopmentOverrideUsed
    throw "Native Unreal materialization entry is BLOCKED. Inspect $ResultPath. Root cause: $($_.Exception.Message)"
}
