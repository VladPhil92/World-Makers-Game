param(
    [string]$EngineRoot,
    [string]$EvidenceDir = 'artifacts\unreal-readiness',
    [switch]$RequireAuthoredMap,
    [switch]$RunAutomation,
    [switch]$CleanIntermediate,
    [switch]$AllowDirtyWorktree,
    [switch]$AllowNonMain,
    [switch]$StopBlockingProcesses,
    [string]$TestFilter = 'WorldMakers.'
)

$ErrorActionPreference = 'Stop'
$RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$EvidencePath = Join-Path $RepoRoot $EvidenceDir
$ExpectedVersionFile = Join-Path $RepoRoot 'game\UNREAL_ENGINE_VERSION'
$WorkstationDoctor = Join-Path $RepoRoot 'scripts\diagnose-unreal-workstation.ps1'
$DoctorReportPath = Join-Path $EvidencePath 'workstation-doctor.json'
$PreflightScript = Join-Path $RepoRoot 'scripts\validate-unreal-source-preflight.py'
$BuildScript = Join-Path $RepoRoot 'scripts\build-unreal.ps1'
$TestScript = Join-Path $RepoRoot 'scripts\test-unreal.ps1'
$IntermediateBuild = Join-Path $RepoRoot 'game\Intermediate\Build'
$ReadinessResult = Join-Path $EvidencePath 'readiness-result.json'
$SourcePreflightLog = Join-Path $EvidencePath 'source-preflight.log'
$RequestedEngineRoot = $EngineRoot

# Contract ownership note: the isolated workstation doctor performs the
# resolve-unreal-engine.ps1 and UnrealEditor-Cmd.exe dependency checks.

New-Item -ItemType Directory -Force -Path $EvidencePath | Out-Null

$Checks = [ordered]@{
    workstationDoctor = $false
    gitAvailable = $false
    gitLfsAvailable = $false
    repositoryCommit = $false
    branchPolicy = $false
    originMainCurrent = $false
    worktreeClean = $false
    engineVersionFile = $false
    engineRootResolved = $false
    engineVersionMatches = $false
    engineBuildTool = $false
    editorCommand = $false
    cppToolchain = $false
    windowsSdk = $false
    blockingProcessState = $false
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
$DoctorStatus = 'not-run'
$DoctorBlockerCodes = @()
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
        schemaVersion = 2
        generatedAtUtc = (Get-Date).ToUniversalTime().ToString('o')
        status = $Status
        mode = if ($RequireAuthoredMap) { 'authored-map-certification-readiness' } else { 'pre-editor-native-readiness' }
        installPolicy = 'manual-only-no-auto-install'
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
        workstationDoctorStatus = $DoctorStatus
        workstationDoctorReport = 'workstation-doctor.json'
        workstationDoctorBlockerCodes = @($DoctorBlockerCodes)
        stopBlockingProcesses = [bool]$StopBlockingProcesses
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
    if (-not (Test-Path $ExpectedVersionFile -PathType Leaf)) {
        Add-Blocker 'Missing game/UNREAL_ENGINE_VERSION.'
    }
    else {
        $ExpectedVersion = (Get-Content $ExpectedVersionFile -Raw).Trim()
        $Checks.engineVersionFile = ($ExpectedVersion -eq '5.8.2')
        if (-not $Checks.engineVersionFile) {
            Add-Blocker "Repository engine baseline must be UE 5.8.2; found '$ExpectedVersion'."
        }
    }

    if (-not (Test-Path $WorkstationDoctor -PathType Leaf)) {
        Add-Blocker 'Missing scripts/diagnose-unreal-workstation.ps1.'
    }
    else {
        $PowerShellExe = Get-Command powershell.exe -ErrorAction SilentlyContinue
        if ($null -eq $PowerShellExe) {
            Add-Blocker 'Windows PowerShell executable is not available to run the isolated workstation doctor.'
        }
        else {
            if (Test-Path $DoctorReportPath) {
                Remove-Item $DoctorReportPath -Force
            }
            $DoctorArgs = @('-NoLogo', '-NoProfile', '-ExecutionPolicy', 'Bypass', '-File', $WorkstationDoctor, '-EvidenceDir', $EvidenceDir)
            if (-not [string]::IsNullOrWhiteSpace($RequestedEngineRoot)) {
                $DoctorArgs += @('-EngineRoot', $RequestedEngineRoot)
            }
            if ($StopBlockingProcesses) {
                $DoctorArgs += '-StopBlockingProcesses'
            }

            & $PowerShellExe.Source @DoctorArgs
            $DoctorExitCode = $LASTEXITCODE

            if (-not (Test-Path $DoctorReportPath -PathType Leaf)) {
                Add-Blocker "Workstation doctor did not produce $DoctorReportPath (exit code $DoctorExitCode)."
            }
            else {
                try {
                    $Doctor = Get-Content $DoctorReportPath -Raw | ConvertFrom-Json
                    $DoctorStatus = [string]$Doctor.status
                    $Checks.workstationDoctor = ($DoctorStatus -eq 'ready')
                    $ResolvedEngineRoot = [string]$Doctor.engineRoot
                    $ActualVersion = [string]$Doctor.actualUnrealVersion
                    $EngineResolutionSource = [string]$Doctor.engineResolutionSource
                    $EngineResolutionMode = [string]$Doctor.engineResolutionMode
                    $EngineCandidateCount = $Doctor.engineCandidateCount
                    $Checks.engineRootResolved = [bool]$Doctor.checks.engineResolved
                    $Checks.engineVersionMatches = [bool]$Doctor.checks.engineVersionMatches
                    $Checks.engineBuildTool = [bool]$Doctor.checks.buildBat
                    $Checks.editorCommand = [bool]$Doctor.checks.editorCommand
                    $Checks.cppToolchain = [bool]$Doctor.checks.cppToolchain
                    $Checks.windowsSdk = [bool]$Doctor.checks.windowsSdk
                    $Checks.blockingProcessState = [bool]$Doctor.checks.blockingProcessState

                    foreach ($DoctorBlocker in @($Doctor.blockers)) {
                        if ($null -ne $DoctorBlocker) {
                            $Code = [string]$DoctorBlocker.code
                            $DoctorBlockerCodes += $Code
                            Add-Blocker "Workstation doctor [$Code]: $($DoctorBlocker.message) Action: $($DoctorBlocker.remediation)"
                        }
                    }
                }
                catch {
                    Add-Blocker "Unable to parse workstation doctor evidence: $($_.Exception.Message)"
                }
            }
        }
    }

    $GitCommand = Get-Command git -ErrorAction SilentlyContinue
    if ($null -eq $GitCommand) {
        $Checks.gitAvailable = $false
        if (-not ($DoctorBlockerCodes -contains 'git-missing')) {
            Add-Blocker 'Git is not available on PATH.'
        }
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
        if (-not $Checks.gitLfsAvailable -and -not ($DoctorBlockerCodes -contains 'git-lfs-missing')) {
            Add-Blocker 'Git LFS is not installed or not available.'
        }
        elseif ($Checks.gitLfsAvailable) {
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
            & $BuildScript `
                -EngineRoot $ResolvedEngineRoot `
                -Configuration Development `
                -EvidenceDir $EvidenceDir `
                -StopBlockingProcesses:$StopBlockingProcesses
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
            & $TestScript `
                -EngineRoot $ResolvedEngineRoot `
                -TestFilter $TestFilter `
                -EvidenceDir $EvidenceDir `
                -StopBlockingProcesses:$StopBlockingProcesses
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
        throw "World Makers Unreal readiness gate blocked. See $ReadinessResult and $DoctorReportPath. Do not reinstall Unreal unless the doctor specifically reports an Unreal dependency defect."
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
