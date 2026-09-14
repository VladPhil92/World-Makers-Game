param(
    [string]$EngineRoot,
    [string]$EvidenceDir = 'artifacts\unreal-readiness',
    [switch]$StopBlockingProcesses,
    [switch]$AsJson
)

$ErrorActionPreference = 'Stop'
$RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$EvidencePath = Join-Path $RepoRoot $EvidenceDir
$ReportPath = Join-Path $EvidencePath 'workstation-doctor.json'
$ExpectedVersionFile = Join-Path $RepoRoot 'game\UNREAL_ENGINE_VERSION'
$EngineResolver = Join-Path $RepoRoot 'scripts\resolve-unreal-engine.ps1'

New-Item -ItemType Directory -Force -Path $EvidencePath | Out-Null

$Checks = [ordered]@{
    windows = $false
    powershell = $false
    git = $false
    gitLfs = $false
    python = $false
    engineResolver = $false
    engineResolved = $false
    engineVersionMatches = $false
    buildBat = $false
    editorExecutable = $false
    editorCommand = $false
    vsWhere = $false
    cppToolchain = $false
    windowsSdk = $false
    blockingProcessState = $false
}
$Blockers = New-Object System.Collections.Generic.List[object]
$Warnings = New-Object System.Collections.Generic.List[string]
$ExpectedVersion = $null
$ResolvedEngineRoot = $null
$ActualVersion = $null
$EngineResolutionSource = $null
$EngineResolutionMode = $null
$EngineCandidateCount = $null
$VisualStudioPath = $null
$VsWherePath = $null
$WindowsSdkRoot = $null
$WindowsSdkVersion = $null
$BlockingProcesses = @()

function Add-DoctorBlocker {
    param(
        [Parameter(Mandatory = $true)][string]$Code,
        [Parameter(Mandatory = $true)][string]$Category,
        [Parameter(Mandatory = $true)][string]$Message,
        [Parameter(Mandatory = $true)][string]$Remediation
    )

    $Blockers.Add([pscustomobject]@{
        code = $Code
        category = $Category
        message = $Message
        remediation = $Remediation
    })
}

function Get-BlockingUnrealProcesses {
    $Names = @('UnrealEditor', 'LiveCodingConsole')
    return @(Get-Process -ErrorAction SilentlyContinue | Where-Object { $Names -contains $_.ProcessName })
}

$IsWindowsHost = ([Environment]::OSVersion.Platform -eq [PlatformID]::Win32NT)
$Checks.windows = $IsWindowsHost
if (-not $Checks.windows) {
    Add-DoctorBlocker -Code 'unsupported-os' -Category 'environment' -Message 'Native World Makers certification requires a Windows workstation.' -Remediation 'Run this doctor on the Windows machine that contains Unreal Engine 5.8.2.'
}

$PowerShellVersion = $PSVersionTable.PSVersion.ToString()
$Checks.powershell = ($PSVersionTable.PSVersion.Major -gt 5 -or ($PSVersionTable.PSVersion.Major -eq 5 -and $PSVersionTable.PSVersion.Minor -ge 1))
if (-not $Checks.powershell) {
    Add-DoctorBlocker -Code 'powershell-too-old' -Category 'environment' -Message "PowerShell $PowerShellVersion is older than the supported 5.1 baseline." -Remediation 'Use Windows PowerShell 5.1 or a newer PowerShell release. No package installation is performed by this repository.'
}

$GitCommand = Get-Command git -ErrorAction SilentlyContinue
$Checks.git = ($null -ne $GitCommand)
if (-not $Checks.git) {
    Add-DoctorBlocker -Code 'git-missing' -Category 'dependency' -Message 'Git is not available on PATH.' -Remediation 'Install or repair Git for Windows using its normal installer, then reopen the terminal. Do not reinstall Unreal Engine for this error.'
}
else {
    $GitLfsOutput = (& git lfs version 2>&1 | Out-String).Trim()
    $Checks.gitLfs = ($LASTEXITCODE -eq 0 -and -not [string]::IsNullOrWhiteSpace($GitLfsOutput))
    if (-not $Checks.gitLfs) {
        Add-DoctorBlocker -Code 'git-lfs-missing' -Category 'dependency' -Message 'Git is available but Git LFS is not functioning.' -Remediation 'Repair Git LFS, then run `git lfs install`. Do not reinstall Unreal Engine for this error.'
    }
}

$PythonCommand = Get-Command python -ErrorAction SilentlyContinue
$Checks.python = ($null -ne $PythonCommand)
if (-not $Checks.python) {
    Add-DoctorBlocker -Code 'python-missing' -Category 'dependency' -Message 'The `python` command is not available on PATH.' -Remediation 'Install or repair Python 3 and ensure the `python` command is available. This is only required for repository validation, not for installing Unreal Engine.'
}

if (-not (Test-Path $ExpectedVersionFile -PathType Leaf)) {
    Add-DoctorBlocker -Code 'engine-lock-missing' -Category 'repository' -Message 'game/UNREAL_ENGINE_VERSION is missing.' -Remediation 'Restore the repository file from main before changing the workstation.'
}
else {
    $ExpectedVersion = (Get-Content $ExpectedVersionFile -Raw).Trim()
}

$Checks.engineResolver = Test-Path $EngineResolver -PathType Leaf
if (-not $Checks.engineResolver) {
    Add-DoctorBlocker -Code 'engine-resolver-missing' -Category 'repository' -Message 'scripts/resolve-unreal-engine.ps1 is missing.' -Remediation 'Restore the repository script from main; do not reinstall Unreal Engine.'
}
elseif ($ExpectedVersion) {
    try {
        $Resolution = & $EngineResolver -RequestedRoot $EngineRoot -ExpectedVersion $ExpectedVersion
        if ($null -eq $Resolution -or [string]::IsNullOrWhiteSpace([string]$Resolution.path)) {
            throw 'Engine resolver returned no path.'
        }
        $ResolvedEngineRoot = [string]$Resolution.path
        $EngineResolutionSource = [string]$Resolution.source
        $EngineResolutionMode = [string]$Resolution.resolutionMode
        $EngineCandidateCount = $Resolution.candidateCount
        $ActualVersion = [string]$Resolution.version
        $Checks.engineResolved = $true
        $Checks.engineVersionMatches = ($ActualVersion -eq $ExpectedVersion)
    }
    catch {
        Add-DoctorBlocker -Code 'unreal-engine-unresolved' -Category 'dependency' -Message $_.Exception.Message -Remediation "Use Epic Games Launcher to verify that exact Unreal Engine $ExpectedVersion is installed. The repository does not install or reinstall Unreal Engine automatically."
    }
}

if ($Checks.engineResolved) {
    $BuildBat = Join-Path $ResolvedEngineRoot 'Engine\Build\BatchFiles\Build.bat'
    $EditorExe = Join-Path $ResolvedEngineRoot 'Engine\Binaries\Win64\UnrealEditor.exe'
    $EditorCmd = Join-Path $ResolvedEngineRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'

    $Checks.buildBat = Test-Path $BuildBat -PathType Leaf
    $Checks.editorExecutable = Test-Path $EditorExe -PathType Leaf
    $Checks.editorCommand = Test-Path $EditorCmd -PathType Leaf

    if (-not $Checks.buildBat) {
        Add-DoctorBlocker -Code 'unreal-build-tool-missing' -Category 'dependency' -Message "Build.bat is missing under '$ResolvedEngineRoot'." -Remediation 'Use Epic Games Launcher > Library > Verify for the existing Unreal installation. Do not start a second installation first.'
    }
    if (-not $Checks.editorExecutable) {
        Add-DoctorBlocker -Code 'unreal-editor-missing' -Category 'dependency' -Message "UnrealEditor.exe is missing under '$ResolvedEngineRoot'." -Remediation 'Use Epic Games Launcher > Library > Verify for the existing Unreal installation.'
    }
    if (-not $Checks.editorCommand) {
        Add-DoctorBlocker -Code 'unreal-editor-cmd-missing' -Category 'dependency' -Message "UnrealEditor-Cmd.exe is missing under '$ResolvedEngineRoot'." -Remediation 'Use Epic Games Launcher > Library > Verify for the existing Unreal installation.'
    }
}

if ($IsWindowsHost) {
    $VsWhereCandidates = New-Object System.Collections.Generic.List[string]
    if (-not [string]::IsNullOrWhiteSpace(${env:ProgramFiles(x86)})) {
        $VsWhereCandidates.Add((Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'))
    }
    $VsWhereCandidates.Add('C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe')

    foreach ($Candidate in $VsWhereCandidates) {
        if (Test-Path $Candidate -PathType Leaf) {
            $VsWherePath = $Candidate
            break
        }
    }
    if (-not $VsWherePath) {
        $VsWhereCommand = Get-Command vswhere.exe -ErrorAction SilentlyContinue
        if ($null -ne $VsWhereCommand) {
            $VsWherePath = $VsWhereCommand.Source
        }
    }

    $Checks.vsWhere = -not [string]::IsNullOrWhiteSpace($VsWherePath)
    if ($Checks.vsWhere) {
        try {
            $VisualStudioPath = ((& $VsWherePath -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath 2>$null | Select-Object -First 1) | Out-String).Trim()
            if (-not [string]::IsNullOrWhiteSpace($VisualStudioPath)) {
                $VcVars64 = Join-Path $VisualStudioPath 'VC\Auxiliary\Build\vcvars64.bat'
                $Checks.cppToolchain = Test-Path $VcVars64 -PathType Leaf
            }
        }
        catch {
            $Warnings.Add("Visual Studio discovery failed: $($_.Exception.Message)")
        }
    }

    if (-not $Checks.cppToolchain) {
        Add-DoctorBlocker -Code 'visual-studio-cpp-toolchain-missing' -Category 'dependency' -Message 'A Visual Studio installation with the x64 C++ toolchain could not be verified.' -Remediation 'Open Visual Studio Installer, choose Modify, and enable Game development with C++ / MSVC x64 tools. Use the Installer UI instead of a PowerShell installation command.'
    }

    try {
        $SdkProperty = Get-ItemProperty 'HKLM:\SOFTWARE\Microsoft\Windows Kits\Installed Roots' -Name KitsRoot10 -ErrorAction Stop
        $WindowsSdkRoot = [string]$SdkProperty.KitsRoot10
    }
    catch {
        if (-not [string]::IsNullOrWhiteSpace(${env:ProgramFiles(x86)})) {
            $FallbackSdkRoot = Join-Path ${env:ProgramFiles(x86)} 'Windows Kits\10'
            if (Test-Path $FallbackSdkRoot -PathType Container) {
                $WindowsSdkRoot = $FallbackSdkRoot
            }
        }
    }

    if ($WindowsSdkRoot) {
        $IncludeRoot = Join-Path $WindowsSdkRoot 'Include'
        if (Test-Path $IncludeRoot -PathType Container) {
            $SdkVersions = @(Get-ChildItem $IncludeRoot -Directory -ErrorAction SilentlyContinue | Where-Object { $_.Name -match '^10\.\d+\.\d+\.\d+$' } | Sort-Object Name -Descending)
            if ($SdkVersions.Count -gt 0) {
                $WindowsSdkVersion = $SdkVersions[0].Name
                $Kernel32Lib = Join-Path $WindowsSdkRoot ("Lib\{0}\um\x64\kernel32.lib" -f $WindowsSdkVersion)
                $Checks.windowsSdk = Test-Path $Kernel32Lib -PathType Leaf
            }
        }
    }

    if (-not $Checks.windowsSdk) {
        Add-DoctorBlocker -Code 'windows-sdk-missing' -Category 'dependency' -Message 'A usable Windows 10/11 SDK with x64 libraries could not be verified.' -Remediation 'Open Visual Studio Installer > Modify and add a current Windows 10/11 SDK. Do not reinstall Unreal Engine for this error.'
    }

    $BlockingProcesses = Get-BlockingUnrealProcesses
    if ($BlockingProcesses.Count -gt 0 -and $StopBlockingProcesses) {
        foreach ($Process in $BlockingProcesses) {
            try {
                Write-Host "Stopping blocking process $($Process.ProcessName) (PID $($Process.Id))..." -ForegroundColor Yellow
                Stop-Process -Id $Process.Id -Force -ErrorAction Stop
            }
            catch {
                $Warnings.Add("Unable to stop $($Process.ProcessName) PID $($Process.Id): $($_.Exception.Message)")
            }
        }
        Start-Sleep -Seconds 2
        $BlockingProcesses = Get-BlockingUnrealProcesses
    }

    $Checks.blockingProcessState = ($BlockingProcesses.Count -eq 0)
    if (-not $Checks.blockingProcessState) {
        $ProcessSummary = (@($BlockingProcesses | ForEach-Object { "$($_.ProcessName) (PID $($_.Id))" }) -join ', ')
        Add-DoctorBlocker -Code 'blocking-unreal-process' -Category 'runtime-state' -Message "Unreal/Live Coding is currently running: $ProcessSummary." -Remediation 'Save your work and close Unreal Editor / Live Coding before compiling, or rerun the readiness gate with -StopBlockingProcesses. This is not an installation failure.'
    }
}

$Status = if ($Blockers.Count -eq 0) { 'ready' } else { 'blocked' }
$Report = [ordered]@{
    schemaVersion = 1
    generatedAtUtc = (Get-Date).ToUniversalTime().ToString('o')
    status = $Status
    installPolicy = 'manual-only-no-auto-install'
    expectedUnrealVersion = $ExpectedVersion
    actualUnrealVersion = $ActualVersion
    requestedEngineRoot = $EngineRoot
    engineRoot = $ResolvedEngineRoot
    engineResolutionSource = $EngineResolutionSource
    engineResolutionMode = $EngineResolutionMode
    engineCandidateCount = $EngineCandidateCount
    powershellVersion = $PowerShellVersion
    visualStudioPath = $VisualStudioPath
    vsWherePath = $VsWherePath
    windowsSdkRoot = $WindowsSdkRoot
    windowsSdkVersion = $WindowsSdkVersion
    blockingProcesses = @($BlockingProcesses | ForEach-Object { [ordered]@{ name = $_.ProcessName; pid = $_.Id } })
    checks = $Checks
    blockers = @($Blockers)
    warnings = @($Warnings)
}

$Report | ConvertTo-Json -Depth 8 | Set-Content -Path $ReportPath -Encoding UTF8

if ($AsJson) {
    $Report | ConvertTo-Json -Depth 8
}
else {
    [pscustomobject]$Report
    Write-Host ''
    if ($Status -eq 'ready') {
        Write-Host 'WORLD MAKERS WORKSTATION DOCTOR: READY' -ForegroundColor Green
    }
    else {
        Write-Host 'WORLD MAKERS WORKSTATION DOCTOR: BLOCKED' -ForegroundColor Red
        foreach ($Blocker in $Blockers) {
            Write-Host "[$($Blocker.code)] $($Blocker.message)" -ForegroundColor Red
            Write-Host "  Action: $($Blocker.remediation)" -ForegroundColor Yellow
        }
    }
    Write-Host "Evidence: $ReportPath"
}

if ($Status -ne 'ready') {
    exit 2
}
