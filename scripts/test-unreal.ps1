param(
    [Parameter(Mandatory = $true)]
    [string]$EngineRoot,
    [string]$TestFilter = 'WorldMakers.',
    [string]$EvidenceDir = 'artifacts\certification'
)

$ErrorActionPreference = 'Stop'
$RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$Project = Join-Path $RepoRoot 'game\WorldMakers.uproject'
$EditorCmd = Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
$EvidencePath = Join-Path $RepoRoot $EvidenceDir
New-Item -ItemType Directory -Force -Path $EvidencePath | Out-Null
$LogPath = Join-Path $EvidencePath 'automation.log'
$ResultPath = Join-Path $EvidencePath 'automation-result.json'

if (-not (Test-Path $EditorCmd)) { throw "UnrealEditor-Cmd.exe not found under EngineRoot: $EngineRoot" }

$Started = (Get-Date).ToUniversalTime()
$ExecCmds = "Automation RunTests $TestFilter; Quit"
& $EditorCmd $Project "-ExecCmds=$ExecCmds" -unattended -nop4 -nosplash -nullrhi '-TestExit=Automation Test Queue Empty' 2>&1 | Tee-Object -FilePath $LogPath
$ExitCode = $LASTEXITCODE
$Finished = (Get-Date).ToUniversalTime()

[ordered]@{
    schemaVersion = 1
    status = if ($ExitCode -eq 0) { 'passed' } else { 'failed' }
    exitCode = $ExitCode
    testFilter = $TestFilter
    startedAtUtc = $Started.ToString('o')
    finishedAtUtc = $Finished.ToString('o')
    log = 'automation.log'
} | ConvertTo-Json -Depth 4 | Set-Content -Path $ResultPath -Encoding UTF8

if ($ExitCode -ne 0) { throw "Unreal automation tests failed with exit code $ExitCode" }
