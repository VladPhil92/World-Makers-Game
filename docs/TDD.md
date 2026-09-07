# World Makers — Technical Design Document

## 1. Technical goals

- Unreal Engine 5.8 native project with C++ foundations and Blueprint-friendly extension points.
- Certification baseline locked to Unreal Engine 5.8.2 until deliberately upgraded.
- Scalable quality tiers for desktop and mid-range tablets.
- Provider-neutral online/backend interfaces until vendor selection.
- Privacy-first account, progress, telemetry, and parental authorization boundaries.
- Content pipelines that allow pedagogues/designers to review source data without opening binary Unreal assets.

## 2. Runtime architecture

Initial native module: `WorldMakers`.

Split into feature modules only when ownership/dependencies justify it. Candidate domains: Core, Building, Missions, Exploration, Learning, Online, Customization, UI.

### Key subsystems

- Building subsystem — place/edit/serialize construction objects.
- Mission subsystem — evaluate mission state from data-driven definitions.
- Learning evidence subsystem — emit minimized objective evidence events.
- Save subsystem — local/prototype persistence evolving to authenticated cloud persistence.
- Online subsystem adapter — sessions/invitations behind Unreal abstractions.
- Scalability/device profile layer — feature tiers for lighting, geometry, materials, VFX, textures.

## 3. Data model

Canonical pedagogical mission source lives under `content/missions`. Build/import tooling can convert approved source into Unreal Data Assets/Data Tables. Runtime should reference stable IDs rather than human-readable strings.

## 4. Networking

- Server-authoritative permissions for private sessions and invites.
- Never trust client claims about parent/child relationships.
- Validate gameplay RPC ownership, rate limits, and input bounds.
- Avoid exposing account PII to game-session participants.

[PLACEHOLDER: listen server vs dedicated server decision, session size, replication graph needs, persistence topology]

## 5. Backend interfaces

Provider-neutral contracts begin under `services/backend/contracts`.

Required domains:

- parent/account identity and consent;
- child profile pseudonymous mapping;
- private friend/invite approvals;
- build/save persistence;
- progress/learning read models;
- export/delete/retention operations;
- minimized telemetry ingestion.

## 6. Parent portal

Separate deployable app within the monorepo. It consumes only authenticated backend APIs; it must not query game databases directly.

## 7. Security

Threat-model at minimum:

- account takeover;
- parental authorization bypass;
- child profile enumeration;
- invite/session abuse;
- insecure direct object references;
- telemetry PII leakage;
- malicious user-generated text/content if introduced later;
- backend secret leakage;
- asset/mod supply-chain risks.

## 8. Rendering and performance

Nanite/Lumen are optional high-tier features, not universal assumptions. Establish representative-device budgets for:

- frame time;
- memory;
- texture pool;
- draw calls/material slots;
- shader complexity;
- dynamic lights/shadows;
- skeletal meshes/animation;
- effects/particles;
- world streaming.

[PLACEHOLDER: target FPS per device class and representative test devices]

## 9. Save/version compatibility

Persistent save data must carry a schema version and support migration. Do not serialize raw UObject implementation details as the long-term backend contract. Prototype saves enforce bounded piece counts and reject invalid transforms before replacing the active world.

## 10. Testing

- Unreal Automation Tests for deterministic systems.
- Functional tests for building/mission flows.
- Contract tests for backend APIs.
- Web unit/e2e tests once parent portal framework is selected.
- Device performance smoke suite on representative tablets.
- Security/privacy test cases for authorization boundaries.

## 11. Build and CI

GitHub-hosted runners validate source/config/content and M1.5 hardening gates. Native certification uses a Windows x64 self-hosted runner with Unreal Engine 5.8.2. The runner must compile `WorldMakersEditor` and execute `WorldMakers.Building.*` through `UnrealEditor-Cmd.exe`.

A lightweight GitHub check is never sufficient evidence of Unreal runtime certification. See `docs/prototype-certification.md`.

## 12. Current locked decisions

- Engine family: Unreal Engine 5.8.
- Certification patch baseline: Unreal Engine 5.8.2.
- Repository architecture: modular monorepo.
- Development model: short-lived branches into `main` through PRs.

## 13. Open decisions

- Parent portal production framework.
- Backend/online provider.
- Listen vs dedicated multiplayer hosting.
- Cloud save topology.
- Analytics provider or in-house pipeline.
- Production encryption/KMS/secrets platform.
