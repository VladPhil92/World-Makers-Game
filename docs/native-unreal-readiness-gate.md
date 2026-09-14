# World Makers — Native Unreal Readiness Gate

Status: pre-Unreal hardening contract  
Baseline: Unreal Engine 5.8.2 / Win64 / Development

## Purpose

This gate is the controlled handoff between repository/source hardening and authored Unreal Editor production. It exists so that opening the project in Unreal is not treated as a speculative debugging step and so that workstation failures are classified before anybody starts reinstalling software.

A workstation is **pre-editor native ready** only when the repository is current and clean, Git LFS is functioning, the locked engine version matches, the Windows C++ toolchain and SDK are verifiable, no Unreal/Live Coding process is blocking compilation, hosted-equivalent source preflight passes, and `WorldMakersEditor Win64 Development` compiles successfully.

A workstation is **authored-map certification ready** only when the same checks pass and `WM_PrototypeCertification.umap` exists at the canonical path. Full automation can then be executed as an explicit second gate.

These states do not imply representative-device certification.

## Diagnostic-first policy: do not reinstall by default

**Do not reinstall Unreal Engine, Visual Studio, Python, Git, or Git LFS merely because a native build failed.** The repository follows a diagnostic-first and manual-only policy. It never runs `winget`, Chocolatey, MSI installers, Epic installers, or Visual Studio installers automatically.

The workstation flow distinguishes five classes of failure:

1. **repository defect** — a required repository script or version lock is missing;
2. **dependency defect** — a specific executable/toolchain/SDK cannot be verified;
3. **runtime-state blocker** — Unreal Editor or Live Coding is already running and blocks compilation;
4. **classified native build defect** — UnrealBuildTool reached project code and the failure can be categorized as include/UHT/compiler/linker/plugin/file-lock/out-of-memory/LFS/etc.;
5. **unknown native build defect** — prerequisites are green but the log does not yet match a known failure signature.

A `blocking-unreal-process` result is **not an installation failure**. Save editor work, close Unreal Editor / `LiveCodingConsole`, and retry. The optional `-StopBlockingProcesses` switch may be used only when it is safe to terminate those known blocking processes.

## Easiest Windows entry points

You no longer need to type installation commands in PowerShell. From File Explorer or Command Prompt, use the repository-owned launchers.

### 1. Diagnose the workstation

```bat
WorldMakers-Doctor.cmd
```

This runs the read-only workstation doctor and writes:

`artifacts/unreal-readiness/workstation-doctor.json`

### 2. Run strict native readiness

```bat
WorldMakers-Readiness.cmd
```

This performs source freshness, Git LFS, workstation diagnosis, UE 5.8.2 validation and native compilation. It writes `readiness-result.json`, `build-result.json` and `build.log`.

### 3. Open Unreal Editor only after readiness passes

```bat
WorldMakers-OpenEditor.cmd
```

`WorldMakers-OpenEditor.cmd` delegates to `scripts/open-unreal-project.ps1`. It runs the strict readiness gate first and opens the exact resolved `UnrealEditor.exe` only if readiness succeeds. When readiness fails, the editor is not opened.

For local development on a deliberate non-main branch or with known uncommitted work, the same launcher supports the explicit diagnostic exceptions already defined by the readiness gate:

```bat
WorldMakers-OpenEditor.cmd -AllowNonMain -AllowDirtyWorktree
```

Those exceptions are useful for development but do **not** create certification evidence.

### 4. Reclassify the last native build failure

```bat
WorldMakers-DiagnoseLastFailure.cmd
```

This reads the existing `build.log` and regenerates:

`artifacts/unreal-readiness/native-failure-summary.json`

No data is uploaded. The summary is bounded and redacts repository/home paths before storing diagnostic excerpts.

If you prefer PowerShell, the equivalent entry points are:

```powershell
.\scripts\diagnose-unreal-workstation.ps1
.\scripts\run-unreal-readiness-gate.ps1 -CleanIntermediate
.\scripts\open-unreal-project.ps1
```

If Unreal Editor or Live Coding is open and you have already saved your work, an explicit opt-in exists:

```powershell
.\scripts\run-unreal-readiness-gate.ps1 -CleanIntermediate -StopBlockingProcesses
```

The default remains fail-closed and will never kill Unreal processes silently.

## Workstation doctor

`scripts/diagnose-unreal-workstation.ps1` verifies the existing workstation without installing anything:

- Windows host and Windows PowerShell 5.1+ compatibility;
- Git and Git LFS;
- Python command availability;
- exact Unreal Engine 5.8.2 resolution;
- `Build.bat`, `UnrealEditor.exe`, and `UnrealEditor-Cmd.exe`;
- Visual Studio discovery through `vswhere.exe`;
- the x64 MSVC C++ toolchain component;
- a usable Windows 10/11 SDK with x64 libraries;
- running `UnrealEditor` / `LiveCodingConsole` processes.

Every failure is emitted with a stable blocker code plus a remediation message. `workstation-doctor.json` is therefore the first artifact to inspect after a failed local attempt.

### Visual Studio remediation

If the doctor reports `visual-studio-cpp-toolchain-missing` or `windows-sdk-missing`, use **Visual Studio Installer** → **Modify**. Enable the C++ game-development/MSVC x64 workload and a current Windows 10/11 SDK. Prefer the Installer UI; the repository deliberately does not attempt to install or mutate Visual Studio through PowerShell.

### Unreal remediation

If the doctor reports `unreal-engine-unresolved` or an Unreal binary is missing, first open **Epic Games Launcher → Unreal Engine → Library** and verify the existing UE 5.8.2 installation. Use Epic's **Verify** action before considering a second installation.

## Native failure classification

`scripts/classify-unreal-build-log.py` turns a failed `build.log` into `native-failure-summary.json`. `scripts/build-unreal.ps1` invokes it automatically after a non-zero UnrealBuildTool exit code and records the resulting category in `build-result.json` as `primaryFailureCategory`.

Current stable categories include:

- `live-coding-active`;
- `visual-studio-toolchain-missing`;
- `windows-sdk-missing`;
- `out-of-memory`;
- `include-file-missing`;
- `file-lock-or-access-denied`;
- `unreal-header-tool-error`;
- `compiler-error`;
- `linker-error`;
- `plugin-or-module-error`;
- `git-lfs-or-binary-pointer-error`;
- `generated-files-stale`;
- `unknown-native-build-failure`.

Specific causes outrank generic compiler/linker categories. For example, `C1060` is classified as `out-of-memory`, and a linker diagnostic that explicitly says a binary is being used by another process is classified as `file-lock-or-access-denied`.

The classifier emits only bounded diagnostic excerpts. Repository and home-directory paths are redacted, and the classifier never uploads data. The full local `build.log` remains available for deep debugging on the workstation.

## Engine auto-discovery

The workstation doctor delegates engine resolution to `scripts/resolve-unreal-engine.ps1`. Auto-discovery is deterministic and fail-closed: it accepts exactly one UE 5.8.2 installation discovered from Epic Launcher manifests or the normal Epic Games installation root. It does not silently select among multiple matching engines.

Resolution precedence is:

1. explicit `-EngineRoot`;
2. `UNREAL_ENGINE_ROOT` environment variable;
3. Epic Launcher manifest auto-discovery;
4. standard `C:\Program Files\Epic Games\UE_*` discovery.

An explicit path remains supported for a non-standard installation:

```powershell
.\scripts\run-unreal-readiness-gate.ps1 `
  -EngineRoot "D:\Epic\UE_5.8" `
  -CleanIntermediate
```

For a persistent workstation configuration:

```powershell
$env:UNREAL_ENGINE_ROOT = "D:\Epic\UE_5.8"
.\scripts\run-unreal-readiness-gate.ps1 -CleanIntermediate
```

## Strict readiness contract

The readiness gate requires:

- current branch `main`, unless a diagnostic exception is explicit;
- local `HEAD` equal to freshly fetched `origin/main`;
- a clean worktree;
- functioning Git and Git LFS plus successful `git lfs pull`;
- repository baseline UE 5.8.2;
- workstation doctor status `ready`;
- exact UE 5.8.2 engine resolution;
- verified `Build.bat` and `UnrealEditor-Cmd.exe`;
- verified Visual Studio x64 C++ toolchain and Windows SDK;
- no blocking Unreal/Live Coding process;
- `validate-unreal-source-preflight.py` success;
- `WorldMakersEditor Win64 Development` native compilation success.

Temporary diagnostic exceptions exist through `-AllowNonMain` and `-AllowDirtyWorktree`, but they must not be used as certification evidence.

## Guarded editor launch

`scripts/open-unreal-project.ps1` is intentionally strict. It runs `run-unreal-readiness-gate.ps1` in an isolated Windows PowerShell process and refuses to launch Unreal Editor after a failed readiness result. If a native build fails, it displays the primary category/remediation from `native-failure-summary.json` when available.

On success it launches the exact engine binary resolved by the workstation doctor and writes:

`artifacts/unreal-readiness/editor-launch-result.json`

That record contains the repository commit, UE version, process ID and references to the readiness/workstation/native-build evidence. It does not claim runtime or device certification.

## Post-map certification mode

After the first Unreal-authored certification map has been created and committed through Git LFS:

```powershell
.\scripts\run-unreal-readiness-gate.ps1 `
  -RequireAuthoredMap `
  -RunAutomation `
  -CleanIntermediate
```

This requires the exact file:

`game/Content/WorldMakers/Maps/WM_PrototypeCertification.umap`

and then executes the complete `WorldMakers.` automation namespace through the native test script.

## Evidence

The gate writes engineering evidence under:

`artifacts/unreal-readiness/`

Primary records:

- `workstation-doctor.json`: machine prerequisite classification and remediation codes;
- `readiness-result.json`: repository/native readiness orchestration;
- `build-result.json` and `build.log`: native build result and full local build log;
- `native-failure-summary.json`: bounded, privacy-minimized failure classification when a build fails or is process-blocked;
- `editor-launch-result.json`: guarded editor launch record after readiness passes;
- `automation-result.json` and `automation.log`: optional native automation result.

`readiness-result.json` records repository commit/branch, fetched `origin/main`, requested/resolved engine roots, engine resolution source/mode, expected/actual Unreal versions, workstation doctor status and blocker codes, source-preflight state, native build state, optional automation state, and blocking reasons.

Evidence must not contain player/guardian identifiers, child-authored text, learning evidence, account data, precise location, commerce state, or behavioral telemetry.

## Fail-closed behavior

The gate blocks rather than guessing when any requested fact cannot be established. A `ready` result means only that every requested engineering gate passed; it does **not** mean the game is production-ready or device-certified.

Crucially, dependency remediation and source remediation are separated. Once the doctor confirms the engine/toolchain/SDK and process state, a subsequent compiler error should be treated as a code/build issue first—not as a reason to reinstall the workstation.

## First Unreal Editor task

Once pre-editor native readiness is green, the first editor-only deliverable is the deterministic certification level:

`/Game/WorldMakers/Maps/WM_PrototypeCertification`

Minimum content:

1. deterministic `PlayerStart`;
2. minimal authored collision/ground geometry;
3. project `WMGameMode` active;
4. enough authored geometry to test build placement without relying exclusively on runtime-generated ground;
5. stable save path and repeatable smoke route.

The map must be committed as `.umap` through Git LFS before it can satisfy certification mode.

## Native smoke sequence after the map exists

The first manual/native route should cover movement/camera, valid and invalid building placement, rotate/move/remove, undo/redo, save/load, rainforest observation/interaction, child journey UI, and one representative mission/epic route without spoofed progression.

Only after this route and `WorldMakers.*` automation are clean should broad authored character, environment, animation, Niagara/VFX and cinematic production become the primary development track.

## Relationship to repository gates

- `Unreal Source Preflight`: hosted static regression gate; no engine execution.
- `validate-windows-workstation-contract.py`: hosted regression guard for the diagnostic/manual-only Windows contract.
- `test-unreal-build-classifier.py`: deterministic hosted tests for native failure categorization and path redaction.
- `resolve-unreal-engine.ps1`: deterministic workstation engine resolution; no build/test claim.
- `diagnose-unreal-workstation.ps1`: read-only dependency/process-state doctor; no installation actions.
- `build-unreal.ps1`: native compile plus structured failure evidence.
- `run-unreal-readiness-gate.ps1`: workstation doctor + source freshness + native compile + optional automation.
- `open-unreal-project.ps1`: guarded editor launch after readiness.
- `Unreal CI / unreal-build-and-test`: continuous native evidence once the self-hosted UE 5.8.2 runner is provisioned.
- M3/M5 certification assessors: higher-level route/device evidence, not substitutes for compilation.

See also `docs/unreal-readiness-audit.md`, Issue #9, Issue #106 and Issue #114.
