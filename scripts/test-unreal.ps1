param(
    [Parameter(Mandatory = $true)]
    [string]$EngineRoot
)

$ErrorActionPreference = 'Stop'
$RepoRoot = Resolve-Path (Join-Path $PSScriptRoot '..')
$Project = Join-Path $RepoRoot 'game\WorldMakers.uproject'
$EditorCmd = Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'

if (-not (Test-Path $EditorCmd)) {
    throw "UnrealEditor-Cmd.exe not found under EngineRoot: $EngineRoot"
}

& $EditorCmd $Project '-ExecCmds=Automation RunTests WorldMakers.Building; Quit' -unattended -nop4 -nosplash -nullrhi '-TestExit=Automation Test Queue Empty'
if ($LASTEXITCODE -ne 0) {
    throw "Unreal automation tests failed with exit code $LASTEXITCODE"
}
