# G4 — Representative Device Performance & Stability

Status: **SOURCE READY / PHYSICAL-DEVICE EVIDENCE REQUIRED**

G4 is the last production gate before **Production Alpha Readiness**. It does not certify visual quality again; G3 owns that. G4 proves that the same certified build sustains its quality and gameplay budgets on representative physical Android and iPadOS devices.

## Gate dependency

G4 requires a G3 result with `status=CERTIFIED`, `certified=true`, and the exact same 40-character repository commit used by every device evidence package.

## Required matrix

Every certification run requires exactly six platform/profile packages:

| Platform | Low | Medium | High |
| --- | --- | --- | --- |
| Android | required | required | required |
| iPadOS | required | required | required |

The profile IDs are:

- `performance.tablet.low`
- `performance.tablet.medium`
- `performance.tablet.high`

V8 remains the authoritative frame-time/GPU/render-budget assessor. G4 consumes its result instead of reimplementing its metrics.

## Required scenarios

Every V8 package must contain the four existing production scenarios:

1. `visual.rainforest.explore`
2. `visual.build.dense`
3. `visual.science.vfx-burst`
4. `visual.adventure.reveal`

Each scenario requires at least 1,800 measured frame samples and its capture/screenshot integrity hashes.

## Long-session stability evidence

Each of the six platform/profile pairs also requires one physical-device stability package. The run must last at least **15 minutes** and contain real telemetry with a SHA-256 hash.

A package is rejected when any of the following occurs:

- thermal throttling or a critical thermal state;
- more than 15% sustained frame-time drift;
- more than 15% sustained performance loss;
- process memory exceeding 80% of physical memory;
- resident-memory growth above the profile ceiling (96 MB low, 128 MB medium, 192 MB high);
- any crash, fatal error, or out-of-memory event;
- fewer than three foreground/background cycles;
- failed suspend/resume recovery;
- failed input recovery after resume;
- emulator/simulator evidence presented as a physical device;
- missing or invalid telemetry hashes.

Platform-native thermal state names are preserved in the evidence (`thermalStateStart` and `thermalStateEnd`) for auditability; G4 does not pretend Android and iPadOS expose identical thermal APIs.

## Evidence layout

```text
artifacts/g4-device/
├── g4-device-performance.json
├── v8/
│   ├── android-low.json
│   ├── android-medium.json
│   ├── android-high.json
│   ├── ipados-low.json
│   ├── ipados-medium.json
│   ├── ipados-high.json
│   ├── captures/...
│   └── screenshots/...
└── stability/
    ├── android-low.json
    ├── android-medium.json
    ├── android-high.json
    ├── ipados-low.json
    ├── ipados-medium.json
    ├── ipados-high.json
    └── telemetry/...
```

G3 evidence normally remains at:

```text
artifacts/g3-visual/g3-visual-fidelity.json
```

`artifacts/` is ignored by Git, so evidence does not make an otherwise clean certification checkout dirty.

## One-command assessment

After real Android/iPadOS evidence has been copied into the structure above and G3 is certified on the same commit:

```bat
WorldMakers-G4-Certify.cmd
```

Optional development assessment:

```bat
WorldMakers-G4-Certify.cmd -AllowNonMain
```

A development run may return `NON_CERTIFYING_PASS`; only clean current `main` synchronized with `origin/main` may produce the official `CERTIFIED` state.

## What CI can and cannot prove

Hosted CI validates the contract, compiles the G4 assessor, and runs deterministic fail-closed self-tests. It **cannot** manufacture physical-device telemetry, thermal behavior, screenshots, or suspend/resume results.

Therefore a green `G4 Device Performance` workflow means **G4 source infrastructure ready**, not that World Makers is device-certified.

## Exit condition

G4 exits only when:

- G3 is certified on the same build commit;
- V8 certifies all six Android/iPadOS platform-profile packages;
- all six long-session stability packages pass;
- capture, screenshot, and telemetry hashes validate;
- the run occurs on clean current `main`.

A real G4 `CERTIFIED` result advances the project to **Production Alpha Readiness**: packaging, broader external playtests, store/distribution readiness, crash telemetry, and operational release discipline.
