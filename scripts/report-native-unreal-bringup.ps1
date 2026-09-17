param(
    [string]$EvidenceDir = 'artifacts\native-materialization'
)

$ErrorActionPreference = 'Stop'
$RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$EvidencePath = Join-Path $RepoRoot $EvidenceDir
$EntryPath = Join-Path $EvidencePath 'native-materialization-entry.json'
$G1Path = Join-Path $EvidencePath 'g1\g1-native-readiness.json'
$FailurePath = Join-Path $EvidencePath 'g1\native-failure-summary.json'
$AuthorReportPath = Join-Path $EvidencePath 'g2\g2-author-report.json'
$SummaryPath = Join-Path $EvidencePath 'native-bringup-summary.json'
$MapPath = Join-Path $RepoRoot 'game\Content\WorldMakers\Maps\WM_PrototypeCertification.umap'

New-Item -ItemType Directory -Force -Path $EvidencePath | Out-Null

function Read-JsonIfPresent([string]$Path) {
    if (Test-Path $Path -PathType Leaf) {
        try {
            return Get-Content $Path -Raw | ConvertFrom-Json
        }
        catch {
            return $null
        }
    }
    return $null
}

function Relative-Path([string]$Path) {
    if (-not (Test-Path $Path -PathType Leaf)) { return $null }
    return ($Path.Substring($RepoRoot.Length + 1) -replace '\\', '/')
}

$Entry = Read-JsonIfPresent $EntryPath
$G1 = Read-JsonIfPresent $G1Path
$Failure = Read-JsonIfPresent $FailurePath
$AuthorReport = Read-JsonIfPresent $AuthorReportPath
$MapPresent = Test-Path $MapPath -PathType Leaf
$MapHash = if ($MapPresent) { (Get-FileHash -Algorithm SHA256 $MapPath).Hash.ToLowerInvariant() } else { $null }

$FailureCategory = if ($Failure -and -not [string]::IsNullOrWhiteSpace([string]$Failure.primaryCategory)) {
    [string]$Failure.primaryCategory
}
elseif ($G1 -and $G1.blockers -and @($G1.blockers).Count -gt 0) {
    'g1-blocked'
}
else {
    $null
}

$Remediation = if ($Failure -and -not [string]::IsNullOrWhiteSpace([string]$Failure.remediation)) {
    [string]$Failure.remediation
}
elseif ($Entry -and $Entry.blockers -and @($Entry.blockers).Count -gt 0) {
    [string]$Entry.blockers[0]
}
elseif ($G1 -and $G1.blockers -and @($G1.blockers).Count -gt 0) {
    [string]$G1.blockers[0]
}
else {
    $null
}

$Status = if ($Entry) {
    [string]$Entry.status
}
elseif ($G1 -and [string]$G1.status -eq 'CERTIFIED') {
    'G1_CERTIFIED_AUTHORING_NOT_RECORDED'
}
elseif ($G1 -and [string]$G1.status -eq 'NON_CERTIFYING_PASS') {
    'G1_NATIVE_PASS_AUTHORING_NOT_RECORDED'
}
elseif ($G1) {
    'G1_BLOCKED'
}
else {
    'NO_NATIVE_RESULT'
}

$Result = [ordered]@{
    schema = 'worldmakers.native-bringup-summary.v1'
    generatedAtUtc = (Get-Date).ToUniversalTime().ToString('o')
    status = $Status
    repositoryCommit = if ($Entry) { [string]$Entry.repositoryCommit } elseif ($G1) { [string]$G1.repositoryCommit } else { $null }
    g1 = [ordered]@{
        status = if ($G1) { [string]$G1.status } else { $null }
        certified = if ($G1) { [bool]$G1.certified } else { $false }
        evidence = Relative-Path $G1Path
    }
    failure = [ordered]@{
        category = $FailureCategory
        confidence = if ($Failure) { [string]$Failure.confidence } else { $null }
        repositoryActionable = if ($Failure -and $null -ne $Failure.repositoryActionable) { [bool]$Failure.repositoryActionable } else { $null }
        remediation = $Remediation
        evidence = Relative-Path $FailurePath
    }
    authoredMap = [ordered]@{
        present = $MapPresent
        diskPath = 'game/Content/WorldMakers/Maps/WM_PrototypeCertification.umap'
        sha256 = $MapHash
        authorStatus = if ($AuthorReport) { [string]$AuthorReport.status } else { $null }
        authorEvidence = Relative-Path $AuthorReportPath
    }
    entryEvidence = Relative-Path $EntryPath
    nextAction = if ($MapPresent) {
        'Open WM_PrototypeCertification in UE 5.8.2, review the native route/assets, then commit the real .umap/.uasset files through Git LFS before G2 certification.'
    }
    elseif ($FailureCategory) {
        'Fix the first classified native blocker, rerun Native Unreal Bring-Up, and do not proceed to authored-map review until G1 passes.'
    }
    else {
        'Inspect the uploaded native evidence. No authored map was produced and no classified build failure was available.'
    }
}

$Result | ConvertTo-Json -Depth 8 | Set-Content -Path $SummaryPath -Encoding UTF8

$Lines = @(
    '# World Makers — Native Unreal Bring-Up',
    '',
    '| Check | Result |',
    '| --- | --- |',
    "| Bring-up status | ``$Status`` |",
    "| G1 status | ``$(if ($G1) { [string]$G1.status } else { 'not-produced' })`` |",
    "| UE authored map | $(if ($MapPresent) { 'present' } else { 'not produced' }) |",
    "| Failure category | ``$(if ($FailureCategory) { $FailureCategory } else { 'none' })`` |",
    ''
)

if ($Remediation) {
    $Lines += '## First actionable blocker'
    $Lines += ''
    $Lines += $Remediation
    $Lines += ''
}

if ($Failure -and $Failure.primaryExcerpts) {
    $Excerpts = @($Failure.primaryExcerpts | Select-Object -First 5)
    if ($Excerpts.Count -gt 0) {
        $Lines += '### Bounded diagnostic excerpts'
        $Lines += ''
        $Lines += '```text'
        $Lines += $Excerpts
        $Lines += '```'
        $Lines += ''
    }
}

$Lines += '## Next action'
$Lines += ''
$Lines += [string]$Result.nextAction
$Lines += ''
$Lines += '> This report is diagnostic evidence. It does not upgrade a development bring-up into G1/G2 certification.'

$Markdown = $Lines -join [Environment]::NewLine
if (-not [string]::IsNullOrWhiteSpace($env:GITHUB_STEP_SUMMARY)) {
    Add-Content -Path $env:GITHUB_STEP_SUMMARY -Value $Markdown -Encoding UTF8
}
Write-Host $Markdown
Write-Host "Machine-readable summary: $SummaryPath"
