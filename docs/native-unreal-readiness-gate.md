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

The workstation doctor distinguishes four classes of failure:

1. **repository defect** — a required repository script or version lock is missing;
2. **dependency defect** — a specific executable/toolchain/SDK cannot be verified;
3. **runtime-state blocker** — Unreal Editor or Live Coding is already running and blocks compilation;
4. **native source/build failure** — prerequisites are present and Unreal Build Tool reached the project code.

A `blocking-unreal-process` result is **not an installation failure**. Save editor work, close Unreal Editor / `LiveCodingConsole`, and retry. The optional `-StopBlockingProcesses` switch may be used only when it is safe to terminate those known blocking processes.

## Easiest Windows entry points

You no longer need to type installation commands in PowerShell.

From File Explorer or Command Prompt, the diagnostic entry point is:

```bat
WorldMakers-Doctor.cmd
```

It runs the read-only workstation doctor and writes:

`artifacts/unreal-readiness/workstation-doctor.json`

When the doctor is green, the native readiness entry point is:

```bat
WorldMakers-Readiness.cmd
```

The launcher delegates to the repository-owned PowerShell scripts; it does not install dependencies.

If you prefer PowerShell, the equivalent commands are:

```powershell
.\scripts\diagnose-unreal-workstation.ps1
.\scripts\run-unreal-readiness-gate.ps1 -CleanIntermediate
```

If Unreal Editor or Live Coding is open and you have already saved your work, an explicit opt-in exists:

```powershell
.\scripts\run-unreal-readiness-gate.ps1 -CleanIntermediate -StopBlockingProcesses
```

The default remains fail-closed and will never kill Unreal processes silently.

## What the workstation doctor verifies

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

Every failure is emitted with a stable blocker code plus a remediation message. This makes `workstation-doctor.json` the first artifact to inspect after a failed local attempt.

### Visual Studio remediation

If the doctor reports `visual-studio-cpp-toolchain-missing` or `windows-sdk-missing`, use **Visual Studio Installer** → **Modify**. Enable the C++ game-development/MSVC x64 workload and a current Windows 10/11 SDK. Prefer the Installer UI; the repository deliberately does not attempt to install or mutate Visual Studio through PowerShell.

### Unreal remediation

If the doctor reports `unreal-engine-unresolved` or an Unreal binary is missing, first open **Epic Games Launcher → Unreal Engine → Library** and verify the existing UE 5.8.2 installation. Use Epic's **Verify** action before considering a second installation.

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
- `build-result.json` and `build.log`: native build result;
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
- `resolve-unreal-engine.ps1`: deterministic workstation engine resolution; no build/test claim.
- `diagnose-unreal-workstation.ps1`: read-only dependency/process-state doctor; no installation actions.
- `run-unreal-readiness-gate.ps1`: workstation doctor + source freshness + native compile + optional automation.
- `Unreal CI / unreal-build-and-test`: continuous native evidence once the self-hosted UE 5.8.2 runner is provisioned.
- M3/M5 certification assessors: higher-level route/device evidence, not substitutes for compilation.

See also `docs/unreal-readiness-audit.md`, Issue #9, Issue #106 and Issue #108.
