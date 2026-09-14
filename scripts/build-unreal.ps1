param(
    [string]$EngineRoot,
    [ValidateSet('Development', 'DebugGame', 'Shipping')]
    [string]$Configuration = 'Development',
    [string]$EvidenceDir = 'artifacts\certification',
    [switch]$StopBlockingProcesses
)

$ErrorActionPreference = 'Stop'
$RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$Project = Join-Path $RepoRoot 'game\WorldMakers.uproject'
$ExpectedVersionFile = Join-Path $RepoRoot 'game\UNREAL_ENGINE_VERSION'
$EngineResolver = Join-Path $RepoRoot 'scripts\resolve-unreal-engine.ps1'
$FailureClassifier = Join-Path $RepoRoot 'scripts\classify-unreal-build-log.py'
$EvidencePath = Join-Path $RepoRoot $EvidenceDir
New-Item -ItemType Directory -Force -Path $EvidencePath | Out-Null
$LogPath = Join-Path $EvidencePath 'build.log'
$ResultPath = Join-Path $EvidencePath 'build-result.json'
$FailureSummaryPath = Join-Path $EvidencePath 'native-failure-summary.json'

if (Test-Path $FailureSummaryPath -PathType Leaf) {
    Remove-Item $FailureSummaryPath -Force
}

function Get-BlockingUnrealProcesses {
    $Names = @('UnrealEditor', 'LiveCodingConsole')
    return @(Get-Process -ErrorAction SilentlyContinue | Where-Object { $Names -contains $_.ProcessName })
}

function Write-KnownFailureSummary {
    param(
        [Parameter(Mandatory = $true)][string]$Category,
        [Parameter(Mandatory = $true)][string]$Confidence,
        [Parameter(Mandatory = $true)][bool]$RepositoryActionable,
        [Parameter(Mandatory = $true)][string]$Remediation,
        [string[]]$Excerpts = @()
    )

    [ordered]@{
        schemaVersion = 1
        status = 'failed'
        primaryCategory = $Category
        confidence = $Confidence
        repositoryActionable = $RepositoryActionable
        remediation = $Remediation
        matchedCategories = @()
        primaryExcerpts = @($Excerpts)
        diagnostics = @()
        diagnosticCount = 0
        privacy = [ordered]@{
            boundedDiagnostics = $true
            absoluteRepositoryPathRedacted = $true
            homePathRedacted = $true
            uploadsData = $false
        }
    } | ConvertTo-Json -Depth 6 | Set-Content -Path $FailureSummaryPath -Encoding UTF8
}

function Invoke-FailureClassifier {
    if (-not (Test-Path $FailureClassifier -PathType Leaf)) {
        return 'classifier-script-missing'
    }

    $Python = Get-Command python -ErrorAction SilentlyContinue
    if ($null -eq $Python) {
        return 'classifier-python-unavailable'
    }

    try {
        # Capture classifier stdout/stderr locally. Otherwise PowerShell adds the
        # Python status lines to this function's success stream and callers can
        # receive System.Object[] instead of one failure category.
        $ClassifierOutput = @(& $Python.Source $FailureClassifier --log $LogPath --output $FailureSummaryPath --repo-root $RepoRoot 2>&1)
        $ClassifierExitCode = $LASTEXITCODE
        if ($ClassifierExitCode -ne 0 -or -not (Test-Path $FailureSummaryPath -PathType Leaf)) {
            return 'classifier-failed'
        }
        $Summary = Get-Content $FailureSummaryPath -Raw | ConvertFrom-Json
        if ([string]::IsNullOrWhiteSpace([string]$Summary.primaryCategory)) {
            return 'classifier-no-category'
        }
        return [string]$Summary.primaryCategory
    }
    catch {
        return 'classifier-failed'
    }
}

function Write-BuildResult {
    param(
        [string]$Status,
        [Nullable[int]]$ExitCode,
        [string]$Blocker,
        [object[]]$BlockingProcesses,
        [string]$UnrealVersion,
        [string]$ResolvedEngineRoot,
        [datetime]$Started,
        [datetime]$Finished,
        [string]$PrimaryFailureCategory
    )

    [ordered]@{
        schemaVersion = 3
        status = $Status
        exitCode = $ExitCode
        blocker = $Blocker
        primaryFailureCategory = $PrimaryFailureCategory
        failureSummary = if (Test-Path $FailureSummaryPath -PathType Leaf) { 'native-failure-summary.json' } else { $null }
        blockingProcesses = @($BlockingProcesses | ForEach-Object { [ordered]@{ name = $_.ProcessName; pid = $_.Id } })
        configuration = $Configuration
        target = 'WorldMakersEditor'
        platform = 'Win64'
        unrealVersion = $UnrealVersion
        engineRoot = $ResolvedEngineRoot
        startedAtUtc = if ($Started) { $Started.ToUniversalTime().ToString('o') } else { $null }
        finishedAtUtc = if ($Finished) { $Finished.ToUniversalTime().ToString('o') } else { $null }
        log = 'build.log'
    } | ConvertTo-Json -Depth 6 | Set-Content -Path $ResultPath -Encoding UTF8
}

if (-not (Test-Path $ExpectedVersionFile -PathType Leaf)) { throw 'Missing game/UNREAL_ENGINE_VERSION certification lock.' }
if (-not (Test-Path $EngineResolver -PathType Leaf)) { throw 'Missing scripts/resolve-unreal-engine.ps1.' }

$ExpectedVersion = (Get-Content $ExpectedVersionFile -Raw).Trim()
$Resolution = & $EngineResolver -RequestedRoot $EngineRoot -ExpectedVersion $ExpectedVersion
if ($null -eq $Resolution -or [string]::IsNullOrWhiteSpace([string]$Resolution.path)) {
    throw 'Unable to resolve the Unreal Engine root.'
}
$ResolvedEngineRoot = [string]$Resolution.path
$BuildVersionFile = Join-Path $ResolvedEngineRoot 'Engine\Build\Build.version'
$BuildBat = Join-Path $ResolvedEngineRoot 'Engine\Build\BatchFiles\Build.bat'

if (-not (Test-Path $BuildVersionFile -PathType Leaf)) { throw "Unreal Build.version not found under EngineRoot: $ResolvedEngineRoot" }
if (-not (Test-Path $BuildBat -PathType Leaf)) { throw "Unreal Build.bat not found under EngineRoot: $ResolvedEngineRoot" }

$BuildVersion = Get-Content $BuildVersionFile -Raw | ConvertFrom-Json
$ActualVersion = "$($BuildVersion.MajorVersion).$($BuildVersion.MinorVersion).$($BuildVersion.PatchVersion)"
if ($ActualVersion -ne $ExpectedVersion) { throw "Unreal version mismatch. Expected $ExpectedVersion but runner has $ActualVersion." }

$BlockingProcesses = Get-BlockingUnrealProcesses
if ($BlockingProcesses.Count -gt 0 -and $StopBlockingProcesses) {
    foreach ($Process in $BlockingProcesses) {
        Write-Host "Stopping blocking process $($Process.ProcessName) (PID $($Process.Id))..." -ForegroundColor Yellow
        Stop-Process -Id $Process.Id -Force -ErrorAction Stop
    }
    Start-Sleep -Seconds 2
    $BlockingProcesses = Get-BlockingUnrealProcesses
}

if ($BlockingProcesses.Count -gt 0) {
    $Summary = (@($BlockingProcesses | ForEach-Object { "$($_.ProcessName) (PID $($_.Id))" }) -join ', ')
    Write-KnownFailureSummary `
        -Category 'live-coding-active' `
        -Confidence 'high' `
        -RepositoryActionable $false `
        -Remediation 'Save editor work, close Unreal Editor / Live Coding, then rerun the build. This is a runtime-state blocker, not an installation failure.' `
        -Excerpts @($Summary)
    Write-BuildResult -Status 'blocked' -ExitCode $null -Blocker 'blocking-unreal-process' -BlockingProcesses $BlockingProcesses -UnrealVersion $ActualVersion -ResolvedEngineRoot $ResolvedEngineRoot -Started $null -Finished $null -PrimaryFailureCategory 'live-coding-active'
    throw "Native build blocked because Unreal Editor or Live Coding is running: $Summary. Save your work and close those processes, or explicitly use -StopBlockingProcesses. This is not an installation failure."
}

$Started = (Get-Date).ToUniversalTime()
Write-Host "Certified Unreal Engine version: $ActualVersion"
Write-Host "Resolved engine root: $ResolvedEngineRoot"
& $BuildBat WorldMakersEditor Win64 $Configuration $Project -WaitMutex -NoHotReloadFromIDE 2>&1 | Tee-Object -FilePath $LogPath
$ExitCode = $LASTEXITCODE
$Finished = (Get-Date).ToUniversalTime()

$PrimaryFailureCategory = $null
if ($ExitCode -ne 0) {
    $PrimaryFailureCategory = Invoke-FailureClassifier
    Write-Host "Native failure category: $PrimaryFailureCategory" -ForegroundColor Yellow
    if (Test-Path $FailureSummaryPath -PathType Leaf) {
        Write-Host "Failure summary: $FailureSummaryPath" -ForegroundColor Yellow
    }
}

Write-BuildResult -Status $(if ($ExitCode -eq 0) { 'passed' } else { 'failed' }) -ExitCode $ExitCode -Blocker $null -BlockingProcesses @() -UnrealVersion $ActualVersion -ResolvedEngineRoot $ResolvedEngineRoot -Started $Started -Finished $Finished -PrimaryFailureCategory $PrimaryFailureCategory

if ($ExitCode -ne 0) {
    throw "Unreal build failed with exit code $ExitCode (category: $PrimaryFailureCategory). Inspect $FailureSummaryPath and $LogPath; reinstalling Unreal is not the default corrective action."
}
