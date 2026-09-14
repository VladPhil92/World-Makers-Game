param(
    [string]$RequestedRoot,
    [string]$ExpectedVersion = '5.8.2',
    [switch]$AsJson
)

$ErrorActionPreference = 'Stop'

function Get-WMUnrealCandidate {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Path,
        [Parameter(Mandatory = $true)]
        [string]$Source
    )

    if ([string]::IsNullOrWhiteSpace($Path)) {
        return $null
    }

    try {
        $NormalizedPath = [System.IO.Path]::GetFullPath([Environment]::ExpandEnvironmentVariables($Path.Trim()))
    }
    catch {
        return [pscustomobject]@{
            path = $Path
            source = $Source
            version = $null
            valid = $false
            reason = 'invalid-path'
        }
    }

    $BuildVersionFile = Join-Path $NormalizedPath 'Engine\Build\Build.version'
    if (-not (Test-Path $BuildVersionFile -PathType Leaf)) {
        return [pscustomobject]@{
            path = $NormalizedPath
            source = $Source
            version = $null
            valid = $false
            reason = 'missing-build-version'
        }
    }

    try {
        $BuildVersion = Get-Content $BuildVersionFile -Raw | ConvertFrom-Json
        $Version = "$($BuildVersion.MajorVersion).$($BuildVersion.MinorVersion).$($BuildVersion.PatchVersion)"
        return [pscustomobject]@{
            path = $NormalizedPath
            source = $Source
            version = $Version
            valid = $true
            reason = $null
        }
    }
    catch {
        return [pscustomobject]@{
            path = $NormalizedPath
            source = $Source
            version = $null
            valid = $false
            reason = 'unreadable-build-version'
        }
    }
}

$Candidates = New-Object System.Collections.Generic.List[object]
$Seen = New-Object 'System.Collections.Generic.HashSet[string]' ([System.StringComparer]::OrdinalIgnoreCase)

function Add-WMUnrealCandidate {
    param(
        [string]$Path,
        [string]$Source
    )

    if ([string]::IsNullOrWhiteSpace($Path)) {
        return
    }

    try {
        $Key = [System.IO.Path]::GetFullPath([Environment]::ExpandEnvironmentVariables($Path.Trim())).TrimEnd('\')
    }
    catch {
        $Key = $Path.Trim()
    }

    if ($Seen.Add($Key)) {
        $Candidate = Get-WMUnrealCandidate -Path $Path -Source $Source
        if ($null -ne $Candidate) {
            $Candidates.Add($Candidate)
        }
    }
}

$ResolutionMode = 'autodiscovery'
if (-not [string]::IsNullOrWhiteSpace($RequestedRoot)) {
    $ResolutionMode = 'explicit'
    Add-WMUnrealCandidate -Path $RequestedRoot -Source 'parameter'
}
elif (-not [string]::IsNullOrWhiteSpace($env:UNREAL_ENGINE_ROOT)) {
    $ResolutionMode = 'environment'
    Add-WMUnrealCandidate -Path $env:UNREAL_ENGINE_ROOT -Source 'UNREAL_ENGINE_ROOT'
}
else {
    $ManifestRoot = if ($env:ProgramData) {
        Join-Path $env:ProgramData 'Epic\EpicGamesLauncher\Data\Manifests'
    }
    else {
        'C:\ProgramData\Epic\EpicGamesLauncher\Data\Manifests'
    }

    if (Test-Path $ManifestRoot -PathType Container) {
        foreach ($ManifestFile in Get-ChildItem -Path $ManifestRoot -Filter '*.item' -File -ErrorAction SilentlyContinue) {
            try {
                $Manifest = Get-Content $ManifestFile.FullName -Raw | ConvertFrom-Json
                if ($Manifest.InstallLocation) {
                    Add-WMUnrealCandidate -Path ([string]$Manifest.InstallLocation) -Source "epic-manifest:$($ManifestFile.Name)"
                }
            }
            catch {
                Write-Verbose "Ignoring unreadable Epic manifest: $($ManifestFile.FullName)"
            }
        }
    }

    $EpicRoots = New-Object System.Collections.Generic.List[string]
    if ($env:ProgramFiles) {
        $EpicRoots.Add((Join-Path $env:ProgramFiles 'Epic Games'))
    }
    $EpicRoots.Add('C:\Program Files\Epic Games')

    $SeenEpicRoots = New-Object 'System.Collections.Generic.HashSet[string]' ([System.StringComparer]::OrdinalIgnoreCase)
    foreach ($EpicRoot in $EpicRoots) {
        if (-not $SeenEpicRoots.Add($EpicRoot)) {
            continue
        }
        if (-not (Test-Path $EpicRoot -PathType Container)) {
            continue
        }
        foreach ($Directory in Get-ChildItem -Path $EpicRoot -Directory -Filter 'UE_*' -ErrorAction SilentlyContinue) {
            Add-WMUnrealCandidate -Path $Directory.FullName -Source 'epic-default-root'
        }
    }
}

$ExactMatches = @($Candidates | Where-Object { $_.valid -and $_.version -eq $ExpectedVersion })

if ($ExactMatches.Count -eq 1) {
    $Selected = $ExactMatches[0]
    $Result = [ordered]@{
        schemaVersion = 1
        status = 'resolved'
        expectedVersion = $ExpectedVersion
        resolutionMode = $ResolutionMode
        path = $Selected.path
        version = $Selected.version
        source = $Selected.source
        candidateCount = $Candidates.Count
        candidates = @($Candidates)
    }

    if ($AsJson) {
        $Result | ConvertTo-Json -Depth 6
    }
    else {
        [pscustomobject]$Result
    }
    exit 0
}

$CandidateSummary = if ($Candidates.Count -eq 0) {
    'no Unreal Engine installations were discovered'
}
else {
    (@($Candidates | ForEach-Object {
        $VersionText = if ($_.version) { $_.version } else { $_.reason }
        "$($_.path) [$VersionText; source=$($_.source)]"
    }) -join '; ')
}

if ($ExactMatches.Count -gt 1) {
    $MatchSummary = (@($ExactMatches | ForEach-Object { "$($_.path) (source=$($_.source))" }) -join '; ')
    throw "Multiple Unreal Engine $ExpectedVersion installations match the readiness baseline: $MatchSummary. Pass -RequestedRoot explicitly or set UNREAL_ENGINE_ROOT."
}

if ($ResolutionMode -eq 'explicit') {
    throw "Requested Unreal Engine root does not resolve to exact UE $ExpectedVersion. Candidate: $CandidateSummary"
}
if ($ResolutionMode -eq 'environment') {
    throw "UNREAL_ENGINE_ROOT does not resolve to exact UE $ExpectedVersion. Candidate: $CandidateSummary"
}

throw "Unable to auto-discover exactly one Unreal Engine $ExpectedVersion installation; $CandidateSummary. Pass -EngineRoot to the readiness gate or set UNREAL_ENGINE_ROOT."
