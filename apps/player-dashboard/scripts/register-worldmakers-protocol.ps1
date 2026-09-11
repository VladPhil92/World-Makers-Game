[CmdletBinding()]
param(
  [Parameter(Mandatory = $false)]
  [string]$ExecutablePath,

  [Parameter(Mandatory = $false)]
  [switch]$Unregister
)

$ErrorActionPreference = 'Stop'
$protocolRoot = 'HKCU:\Software\Classes\worldmakers'

if ($Unregister) {
  if (Test-Path $protocolRoot) {
    Remove-Item -Path $protocolRoot -Recurse -Force
    Write-Host 'World Makers protocol handler removed.'
  } else {
    Write-Host 'World Makers protocol handler was not registered.'
  }
  exit 0
}

if ([string]::IsNullOrWhiteSpace($ExecutablePath)) {
  throw 'ExecutablePath is required unless -Unregister is used.'
}

$resolvedExecutable = (Resolve-Path -LiteralPath $ExecutablePath).Path
if (-not (Test-Path -LiteralPath $resolvedExecutable -PathType Leaf)) {
  throw "World Makers executable was not found: $resolvedExecutable"
}

New-Item -Path $protocolRoot -Force | Out-Null
Set-Item -Path $protocolRoot -Value 'URL:World Makers Native Launch Protocol'
New-ItemProperty -Path $protocolRoot -Name 'URL Protocol' -Value '' -PropertyType String -Force | Out-Null

$iconKey = Join-Path $protocolRoot 'DefaultIcon'
New-Item -Path $iconKey -Force | Out-Null
Set-Item -Path $iconKey -Value ('"{0}",0' -f $resolvedExecutable)

$commandKey = Join-Path $protocolRoot 'shell\open\command'
New-Item -Path $commandKey -Force | Out-Null
Set-Item -Path $commandKey -Value ('"{0}" "%1"' -f $resolvedExecutable)

Write-Host "Registered worldmakers:// -> $resolvedExecutable"
Write-Host 'The Player Hub can now request the native client through the INICIAR JUEGO button.'
