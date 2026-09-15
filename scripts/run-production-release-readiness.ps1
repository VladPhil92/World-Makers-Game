param(
  [string]$EvidenceDir = "artifacts\production-release",
  [string]$RcResult = "artifacts\release-candidate\release-candidate-readiness.json",
  [string]$RcFreeze = "artifacts\release-candidate\freeze-manifest.json",
  [switch]$AllowNonMain,
  [switch]$AllowDirtyWorktree
)
$ErrorActionPreference = "Stop"
$repo = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
Set-Location $repo
python scripts/validate-production-release-readiness.py
if ($LASTEXITCODE -ne 0) { throw "Production Release source validation failed." }
git fetch origin main --quiet
$head = (git rev-parse HEAD).Trim(); $branch = (git branch --show-current).Trim(); $originMain = (git rev-parse origin/main).Trim(); $dirty = -not [string]::IsNullOrWhiteSpace((git status --porcelain | Out-String).Trim())
$outDir = Join-Path $repo $EvidenceDir; New-Item -ItemType Directory -Force -Path $outDir | Out-Null
$temp = Join-Path $outDir "production-release-assessment.raw.json"; $final = Join-Path $outDir "production-release-readiness.json"
python scripts/assess-production-release-readiness.py --root $EvidenceDir --rc-result $RcResult --rc-freeze $RcFreeze --commit $head --output $temp
$assessment = Get-Content $temp -Raw | ConvertFrom-Json
$contextReasons = @()
if (-not $AllowNonMain) {
  if ($branch -ne "main") { $contextReasons += "Certification requires branch main." }
  if ($head -ne $originMain) { $contextReasons += "Certification requires HEAD == origin/main." }
}
if (-not $AllowDirtyWorktree -and $dirty) { $contextReasons += "Certification requires a clean worktree." }
if ($assessment.certified -eq $true -and $contextReasons.Count -gt 0) {
  $assessment.certified = $false; $assessment.status = "NON_CERTIFYING_PASS"; $assessment.nextPhase = $null
  foreach ($reason in $contextReasons) { $assessment.reasons += $reason }
}
$assessment | Add-Member -NotePropertyName repositoryContext -NotePropertyValue ([ordered]@{branch=$branch; head=$head; originMain=$originMain; cleanWorktree=(-not $dirty); allowNonMain=[bool]$AllowNonMain; allowDirtyWorktree=[bool]$AllowDirtyWorktree}) -Force
$assessment | ConvertTo-Json -Depth 20 | Set-Content -Encoding utf8 $final
Remove-Item $temp -Force -ErrorAction SilentlyContinue
Write-Host "Production Release readiness: $($assessment.status)"
Write-Host "Evidence: $final"
if ($assessment.status -eq "CERTIFIED") { Write-Host "Exact RC payload authorized for staged production rollout. This does not publish the release."; exit 0 }
if ($assessment.status -eq "NON_CERTIFYING_PASS") { Write-Host "Evidence passes, but repository context is non-certifying."; exit 0 }
$assessment.reasons | ForEach-Object { Write-Host "BLOCKER: $_" }
exit 1
