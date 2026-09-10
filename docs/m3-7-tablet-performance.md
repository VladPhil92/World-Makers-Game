# M3.7 — Tablet Performance Profiles & Capture

## Purpose

M3.7 defines the performance envelope for the Caribbean Rainforest vertical slice before M3.8 certification. It introduces explicit, versioned engineering targets and a local runtime capture path so World Makers can answer a concrete question on representative hardware: **does this device, running this profile, stay inside the agreed frame-time and world-complexity budget?**

The numbers in this phase are targets. They are not claims that a tablet has already achieved them.

## Profile architecture

Canonical source:

`content/performance/tablet-performance-profiles.json`

Packaged runtime copy:

`game/Content/WorldMakers/Performance/tablet-performance-profiles.json`

Schema:

`content/performance/tablet-performance-profile.schema.json`

Runtime ownership:

```text
performance profile JSON
        |
        v
FWMPerformanceProfileCatalog
        |
        v
UWMPerformanceProfileSubsystem
        |
        +--> fixed CVar allowlist
        |
        v
render/scalability settings
```

The JSON does not contain console-variable names or commands. It contains bounded values only. `FWMScalabilityProfile::BuildAllowlistedCVarAssignments()` owns the fixed mapping from those values to approved Unreal scalability variables.

## Engineering targets

| Profile | Target | Frame-time budget | Screen % | Texture pool | Max actors | Max placed pieces | Max interactables |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| Tablet Low | 30 FPS | 33.34 ms | 70 | 384 MB | 900 | 240 | 20 |
| Tablet Medium | 30 FPS | 33.34 ms | 85 | 512 MB | 1,300 | 400 | 32 |
| Tablet High | 60 FPS | 16.67 ms | 100 | 768 MB | 1,800 | 600 | 48 |
| Desktop Reference | 60 FPS | 16.67 ms | 100 | 1,024 MB | 2,600 | 1,000 | 64 |

Tablet High is intentionally demanding. A device that cannot hold its p95 frame time below 16.67 ms does not pass that tier and should use a lower profile. This is a measurable classification target, not a marketing promise.

## Scalability controls

Only the following variables may be written by the profile subsystem:

- `sg.ViewDistanceQuality`
- `sg.AntiAliasingQuality`
- `sg.ShadowQuality`
- `sg.PostProcessQuality`
- `sg.TextureQuality`
- `sg.EffectsQuality`
- `sg.FoliageQuality`
- `r.ScreenPercentage`
- `r.Streaming.PoolSize`
- `foliage.DensityScale`
- `grass.DensityScale`
- `r.Shadow.DistanceScale`

No CVar or console command is accepted from content data. This prevents a profile file from becoming an arbitrary command surface.

The current platform default is Tablet Medium on iOS/Android and Desktop Reference elsewhere. Production hardware classification may become more specific after representative-device evidence exists.

## Runtime capture

`UWMPerformanceCaptureSubsystem` is a `UTickableWorldSubsystem` used only while an explicit capture is active.

It records:

- per-frame frame time in milliseconds;
- maximum world actor count observed;
- maximum placed building-piece count observed;
- maximum active observation/care interactable count observed.

Structural counts are sampled every 0.5 seconds rather than every frame to keep measurement overhead bounded.

A capture summary contains:

- frame sample count;
- average frame time;
- p95 frame time;
- worst frame time;
- structural maxima;
- individual budget pass/fail flags;
- overall budget pass/fail.

The primary frame-time gate is p95, not average frame time, so intermittent slow frames cannot be hidden by faster frames.

## Local evidence file

Ending a capture with report writing enabled stores a deterministic local report under:

`Saved/WorldMakers/Performance/capture-<profile-id>.json`

The certification evidence collector copies any available capture reports into:

`artifacts/certification/performance/`

M3.7 does **not** make those reports mandatory for the existing M1 native gate. M3.8 is responsible for requiring representative-device performance evidence before the Caribbean Rainforest vertical slice can be called certified.

## Privacy boundary

Performance capture is deliberately not behavioral analytics. **No PII is collected or exported by this capture path.**

The report contains no:

- child name or profile ID;
- account/email/device advertising ID;
- physical location;
- mission answers or learning-response text;
- chat/free text;
- commerce, entitlement or purchase data;
- gameplay event history.

The report is a local engineering artifact composed only of performance profile ID, bounded frame-time measurements, structural counts and pass/fail results.

## Gameplay boundary

Performance systems are orthogonal to progression. They do not:

- grant rewards;
- complete missions;
- record learning evidence;
- mutate ecosystem state;
- change ecological interventions;
- change commerce or entitlements.

Applying a performance profile changes rendering/scalability only.

## Tests and source gate

Automation tests:

- `WorldMakers.Performance.Profiles.CatalogAndAllowlist`
- `WorldMakers.Performance.Capture.BudgetEvaluation`

Repository gate:

- `scripts/validate-m3-7-tablet-performance.py`

The gate validates canonical/package equivalence, profile monotonicity, target frame-time relationships, fixed CVar ownership, capture privacy boundaries, tests, evidence integration and CI registration.

## Certification boundary

M3.7 can be source-complete after source gates pass. Native performance certification remains blocked until:

1. Issue #9 provisions the exact Unreal Engine 5.8.2 Windows x64 certification environment and authored map;
2. representative tablets execute the complete M3 vertical-slice route under explicitly selected performance profiles;
3. capture JSON and device/build metadata are retained as evidence;
4. M3.8 defines and passes the final vertical-slice certification matrix.

A skipped Unreal-native job is not device certification.

## Next phase

**M3.8 — Caribbean Rainforest Vertical Slice Certification** should combine native Unreal compilation/tests, authored-map smoke evidence, representative tablet capture, complete play-loop verification and release-blocking certification criteria.
