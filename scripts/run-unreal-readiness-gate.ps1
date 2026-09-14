param(
    [string]$EngineRoot,
    [string]$EvidenceDir = 'artifacts\unreal-readiness',
    [switch]$RequireAuthoredMap,
    [switch]$RunAutomation,
    [switch]$CleanIntermediate,
    [switch]$AllowDirtyWorktree,
    [switch]$AllowNonMain,
    [string]$TestFilter = 'WorldMakers.'
)

$ErrorActionPreference = 'Stop'
$RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$EvidencePath = Join-Path $RepoRoot $EvidenceDir
$ExpectedVersionFile = Join-Path $RepoRoot 'game\UNREAL_ENGINE_VERSION'
$EngineResolver = Join-Path $RepoRoot 'scripts\resolve-unreal-engine.ps1'
$PreflightScript = Join-Path $RepoRoot 'scripts\validate-unreal-source-preflight.py'
$BuildScript = Join-Path $RepoRoot 'scripts\build-unreal.ps1'
$TestScript = Join-Path $RepoRoot 'scripts\test-unreal.ps1'
$IntermediateBuild = Join-Path $RepoRoot 'game\Intermediate\Build'
$ReadinessResult = Join-Path $EvidencePath 'readiness-result.json'
$SourcePreflightLog = Join-Path $EvidencePath 'source-preflight.log'
$RequestedEngineRoot = $EngineRoot

New-Item -ItemType Directory -Force -Path $EvidencePath | Out-Null

$Checks = [ordered]@{
    gitAvailable = $false
    gitLfsAvailable = $false
    repositoryCommit = $false
    branchPolicy = $false
    originMainCurrent = $false
    worktreeClean = $false
    engineVersionFile = $false
    engineResolver = $false
    engineRootResolved = $false
    engineVersionMatches = $false
    engineBuildTool = $false
    editorCommand = $false
    sourcePreflight = $false
    nativeBuild = $false
    nativeAutomation = if ($RunAutomation) { $false } else { $null }
}
$Blockers = New-Object System.Collections.Generic.List[string]
$CommitSha = $null
$BranchName = $null
$OriginMainSha = $null
$ExpectedVersion = $null
$ActualVersion = $null
$ResolvedEngineRoot = $null
$EngineResolutionSource = $null
$EngineResolutionMode = $null
$EngineCandidateCount = $null
$SourcePreflightStatus = 'not-run'
$NativeBuildStatus = 'not-run'
$NativeAutomationStatus = if ($RunAutomation) { 'not-run' } else { 'not-requested' }

function Add-Blocker([string]$Message) {
    $Blockers.Add($Message)
    Write-Host "BLOCKED: $Message" -ForegroundColor Red
}

function Write-ReadinessReport {
    param([string]$Status)

    [ordered]@{
        schemaVersion = 1
        generatedAtUtc = (Get-Date).ToUniversalTime().ToString('o')
        status = $Status
        mode = if ($RequireAuthoredMap) { 'authored-map-certification-readiness' } else { 'pre-editor-native-readiness' }
        repositoryCommit = $CommitSha
        branch = $BranchName
        originMainCommit = $OriginMainSha
        requestedEngineRoot = $RequestedEngineRoot
        engineRoot = $ResolvedEngineRoot
        engineResolutionSource = $EngineResolutionSource
        engineResolutionMode = $EngineResolutionMode
        engineCandidateCount = $EngineCandidateCount
        expectedUnrealVersion = $ExpectedVersion
        actualUnrealVersion = $ActualVersion
        requireAuthoredMap = [bool]$RequireAuthoredMap
        runAutomation = [bool]$RunAutomation
        testFilter = $TestFilter
        sourcePreflightStatus = $SourcePreflightStatus
        nativeBuildStatus = $NativeBuildStatus
        nativeAutomationStatus = $NativeAutomationStatus
        checks = $Checks
        blockers = @($Blockers)
    } | ConvertTo-Json -Depth 8 | Set-Content -Path $ReadinessResult -Encoding UTF8
}

try {
    $GitCommand = Get-Command git -ErrorAction SilentlyContinue
    if ($null -eq $GitCommand) {
        Add-Blocker 'Git is not available on PATH.'
    }
    else {
        $Checks.gitAvailable = $true

        $CommitSha = (& git -C $RepoRoot rev-parse HEAD 2>$null | Out-String).Trim()
        $Checks.repositoryCommit = ($LASTEXITCODE -eq 0 -and $CommitSha -match '^[0-9a-f]{40}$')
        if (-not $Checks.repositoryCommit) {
            Add-Blocker 'Unable to resolve the repository HEAD commit.'
        }

        $BranchName = (& git -C $RepoRoot rev-parse --abbrev-ref HEAD 2>$null | Out-String).Trim()
        if ($AllowNonMain -or $BranchName -eq 'main') {
            $Checks.branchPolicy = $true
        }
        else {
            Add-Blocker "Readiness must run from main unless -AllowNonMain is explicit; current branch is '$BranchName'."
        }

        $GitLfsVersion = (& git lfs version 2>&1 | Out-String).Trim()
        $Checks.gitLfsAvailable = ($LASTEXITCODE -eq 0 -and -not [string]::IsNullOrWhiteSpace($GitLfsVersion))
        if (-not $Checks.gitLfsAvailable) {
            Add-Blocker 'Git LFS is not installed or not available.'
        }
        else {
            & git -C $RepoRoot lfs pull
            if ($LASTEXITCODE -ne 0) {
                Add-Blocker 'git lfs pull failed; authored/binary assets cannot be trusted.'
            }
        }

        & git -C $RepoRoot fetch origin main --quiet
        if ($LASTEXITCODE -eq 0) {
            $OriginMainSha = (& git -C $RepoRoot rev-parse origin/main 2>$null | Out-String).Trim()
            $Checks.originMainCurrent = ($AllowNonMain -or ($CommitSha -eq $OriginMainSha))
            if (-not $Checks.originMainCurrent) {
                Add-Blocker "Local HEAD is not origin/main. Local=$CommitSha origin/main=$OriginMainSha"
            }
        }
        else {
            Add-Blocker 'Unable to fetch origin/main; source freshness cannot be established.'
        }

        $WorkingTree = @(& git -C $RepoRoot status --porcelain --untracked-files=all)
        $Checks.worktreeClean = ($AllowDirtyWorktree -or $WorkingTree.Count -eq 0)
        if (-not $Checks.worktreeClean) {
            Add-Blocker 'Working tree contains uncommitted or untracked changes. Commit/stash them or use -AllowDirtyWorktree explicitly.'
        }
    }

    if (-not (Test-Path $ExpectedVersionFile)) {
        Add-Blocker 'Missing game/UNREAL_ENGINE_VERSION.'
    }
    else {
        $ExpectedVersion = (Get-Content $ExpectedVersionFile -Raw).Trim()
        $Checks.engineVersionFile = ($ExpectedVersion -eq '5.8.2')
        if (-not $Checks.engineVersionFile) {
            Add-Blocker "Repository engine baseline must be UE 5.8.2; found '$ExpectedVersion'."
        }
    }

    $Checks.engineResolver = Test-Path $EngineResolver -PathType Leaf
    if (-not $Checks.engineResolver) {
        Add-Blocker 'Missing scripts/resolve-unreal-engine.ps1.'
    }
    elseif ($ExpectedVersion) {
        try {
            $Resolution = & $EngineResolver -RequestedRoot $RequestedEngineRoot -ExpectedVersion $ExpectedVersion
            if ($null -eq $Resolution -or [string]::IsNullOrWhiteSpace([string]$Resolution.path)) {
                throw 'Engine resolver returned no path.'
            }
            $ResolvedEngineRoot = [string]$Resolution.path
            $EngineResolutionSource = [string]$Resolution.source
            $EngineResolutionMode = [string]$Resolution.resolutionMode
            $EngineCandidateCount = $Resolution.candidateCount
            $Checks.engineRootResolved = $true
            Write-Host "Resolved Unreal Engine $ExpectedVersion: $ResolvedEngineRoot ($EngineResolutionSource)"
        }
        catch {
            Add-Blocker "Unable to resolve exact Unreal Engine $ExpectedVersion: $($_.Exception.Message)"
        }
    }

    if ($Checks.engineRootResolved) {
        $BuildVersionFile = Join-Path $ResolvedEngineRoot 'Engine\Build\Build.version'
        $BuildBat = Join-Path $ResolvedEngineRoot 'Engine\Build\BatchFiles\Build.bat'
        $EditorCmd = Join-Path $ResolvedEngineRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'

        if (-not (Test-Path $BuildVersionFile -PathType Leaf)) {
            Add-Blocker "Engine Build.version not found under '$ResolvedEngineRoot'."
        }
        else {
            try {
                $BuildVersion = Get-Content $BuildVersionFile -Raw | ConvertFrom-Json
                $ActualVersion = "$($BuildVersion.MajorVersion).$($BuildVersion.MinorVersion).$($BuildVersion.PatchVersion)"
                $Checks.engineVersionMatches = ($ExpectedVersion -and $ActualVersion -eq $ExpectedVersion)
                if (-not $Checks.engineVersionMatches) {
                    Add-Blocker "Unreal version mismatch. Expected $ExpectedVersion but workstation has $ActualVersion."
                }
            }
            catch {
                Add-Blocker "Unable to parse Unreal Build.version: $($_.Exception.Message)"
            }
        }

        $Checks.engineBuildTool = Test-Path $BuildBat -PathType Leaf
        if (-not $Checks.engineBuildTool) {
            Add-Blocker "Unreal Build.bat is missing under '$ResolvedEngineRoot'."
        }

        $Checks.editorCommand = Test-Path $EditorCmd -PathType Leaf
        if (-not $Checks.editorCommand) {
            Add-Blocker "UnrealEditor-Cmd.exe is missing under '$ResolvedEngineRoot'."
        }
    }

    if ($Blockers.Count -eq 0) {
        $Python = Get-Command python -ErrorAction SilentlyContinue
        if ($null -eq $Python) {
            Add-Blocker 'Python is not available on PATH for source preflight.'
        }
        else {
            $PreflightArgs = @($PreflightScript)
            if ($RequireAuthoredMap) { $PreflightArgs += '--require-authored-map' }
            $PreflightOutput = & $Python.Source @PreflightArgs 2>&1
            $PreflightExit = $LASTEXITCODE
            $PreflightOutput | Set-Content -Path $SourcePreflightLog -Encoding UTF8
            $PreflightOutput | Write-Host
            $Checks.sourcePreflight = ($PreflightExit -eq 0)
            $SourcePreflightStatus = if ($Checks.sourcePreflight) { 'passed' } else { 'failed' }
            if (-not $Checks.sourcePreflight) {
                Add-Blocker "Source preflight failed with exit code $PreflightExit."
            }
        }
    }

    if ($Blockers.Count -eq 0 -and $CleanIntermediate -and (Test-Path $IntermediateBuild)) {
        Write-Host 'Removing game/Intermediate/Build for a clean native compile...'
        Remove-Item $IntermediateBuild -Recurse -Force
    }

    if ($Blockers.Count -eq 0) {
        try {
            & $BuildScript -EngineRoot $ResolvedEngineRoot -Configuration Development -EvidenceDir $EvidenceDir
            $Checks.nativeBuild = $true
            $NativeBuildStatus = 'passed'
        }
        catch {
            $NativeBuildStatus = 'failed'
            Add-Blocker "Native WorldMakersEditor build failed: $($_.Exception.Message)"
        }
    }

    if ($Blockers.Count -eq 0 -and $RunAutomation) {
        try {
            & $TestScript -EngineRoot $ResolvedEngineRoot -TestFilter $TestFilter -EvidenceDir $EvidenceDir
            $Checks.nativeAutomation = $true
            $NativeAutomationStatus = 'passed'
        }
        catch {
            $NativeAutomationStatus = 'failed'
            Add-Blocker "Native Unreal automation failed: $($_.Exception.Message)"
        }
    }

    if ($Blockers.Count -gt 0) {
        Write-ReadinessReport -Status 'blocked'
        throw "World Makers Unreal readiness gate blocked. See $ReadinessResult"
    }

    Write-ReadinessReport -Status 'ready'
    Write-Host ''
    Write-Host 'WORLD MAKERS UNREAL READINESS: READY' -ForegroundColor Green
    Write-Host "Evidence: $ReadinessResult"
    if (-not $RequireAuthoredMap) {
        Write-Host 'Next editor-only task: author game/Content/WorldMakers/Maps/WM_PrototypeCertification.umap in UE 5.8.2.'
    }
}
catch {
    if (-not (Test-Path $ReadinessResult)) {
        Write-ReadinessReport -Status 'blocked'
    }
    throw
}
