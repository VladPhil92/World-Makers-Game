# World Makers — Technical Design Document

## 1. Technical goals

- Unreal Engine 5.8 native project with C++ foundations and Blueprint-friendly extension points.
- Certification baseline locked to Unreal Engine 5.8.2 until deliberately upgraded.
- Scalable quality tiers for desktop and mid-range tablets.
- Provider-neutral online/backend interfaces until vendor selection.
- Privacy-first account, progress, telemetry, parental authorization, commerce, and entitlement boundaries.
- Content pipelines that allow pedagogues/designers to review source data without opening binary Unreal assets.
- Child-safe commerce architecture where payment execution is impossible from the child runtime.

## 2. Runtime architecture

Initial native module: `WorldMakers`.

Split into feature modules only when ownership/dependencies justify it. Candidate domains: Core, Building, Missions, Exploration, Learning, Online, Customization, UI, Progression, Entitlements.

### Key subsystems

- Building subsystem — place/edit/serialize construction objects.
- Mission subsystem — evaluate mission state from data-driven definitions.
- Learning evidence subsystem — emit minimized objective evidence events.
- Progression/reward subsystem — deterministic non-purchasable unlocks and mastery recognition.
- Entitlement read subsystem — consume a server-authorized read model of content owned by the family; no checkout capability.
- Save subsystem — local/prototype persistence evolving to authenticated cloud persistence.
- Online subsystem adapter — sessions/invitations behind Unreal abstractions.
- Scalability/device profile layer — feature tiers for lighting, geometry, materials, VFX, textures.

### Prohibited runtime dependencies

The child-facing game runtime must never directly integrate payment-provider SDKs, app-store billing SDKs, receipt-validation libraries, advertising SDKs, or third-party behavioral-advertising SDKs. StoreKit/Google Billing/platform commerce integrations belong behind parent/server adapters only.

## 3. Data model

Canonical pedagogical mission source lives under `content/missions`. Canonical gameplay reward definitions live under `content/economy/rewards` and must validate against the Trust Economy schema. Build/import tooling can convert approved source into Unreal Data Assets/Data Tables. Runtime should reference stable IDs rather than human-readable strings.

Commerce offers are not gameplay reward definitions. Parent-facing commerce contracts live under `services/backend/contracts` and resolve platform SKU/price data through server/provider adapters.

## 4. Networking

- Server-authoritative permissions for private sessions and invites.
- Never trust client claims about parent/child relationships.
- Validate gameplay RPC ownership, rate limits, and input bounds.
- Avoid exposing account PII to game-session participants.
- Entitlement grants are server-authoritative; the client may cache but cannot mint paid ownership.

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
- minimized telemetry ingestion;
- **Commerce Catalog** — parent-visible sellable products and platform SKU mapping;
- **Parent Purchase Authorization** — authenticated parent-only checkout initiation;
- **Payment Provider Adapters** — Apple/Google/desktop provider integrations behind server/provider boundaries;
- **Receipt/Transaction Verification** — server-side validation and normalization;
- **Entitlements** — durable family-owned content rights, restore/revoke/refund state;
- **Subscriptions** — family membership state without deleting child creations when access changes;
- **Child Interest Requests** — optional, rate-limited, non-commercial requests that save an offer for parent review without checkout or nagging;
- **Reward Ledger / Progression** — deterministic gameplay-earned unlock state completely separate from payment state.

### Trust boundary

`Child Runtime -> Entitlement Read / Child Interest Request` is allowed.

`Child Runtime -> Payment / Receipt Validation / Provider Checkout` is forbidden.

`Parent Portal -> Parent Auth -> Commerce -> Provider Adapter -> Entitlement Service` is the approved paid-content path.

Reward state cannot be converted into paid currency, fiat, crypto, or transferable value.

## 6. Parent portal

Separate deployable app within the monorepo. It consumes only authenticated backend APIs; it must not query game databases directly.

Parent commerce surfaces own:

- localized prices and product disclosure;
- purchase requests/interests;
- purchase confirmation and platform handoff;
- transaction history;
- subscriptions;
- restore/refund/help status;
- commercial-surface enable/disable controls per family/child profile.

## 7. Security and trust

Threat-model at minimum:

- account takeover;
- parental authorization bypass;
- child profile enumeration;
- invite/session abuse;
- insecure direct object references;
- telemetry PII leakage;
- entitlement forgery;
- replayed/forged receipts;
- child-to-parent purchase-pressure abuse;
- accidental payment-SDK inclusion in child runtime;
- malicious user-generated text/content if introduced later;
- backend secret leakage;
- asset/mod supply-chain risks.

Commerce audit records belong to the parent/family trust zone and must not be exposed to child peers or public gameplay.

## 8. Rendering and performance

Nanite/Lumen are optional high-tier features, not universal assumptions. Performance is governed by versioned World Makers profiles rather than one universal rendering assumption.

M3.7 source targets:

| Tier | Target | p95 frame time | Screen percentage | Texture pool |
| --- | ---: | ---: | ---: | ---: |
| Tablet Low | 30 FPS | 33.34 ms | 70% | 384 MB |
| Tablet Medium | 30 FPS | 33.34 ms | 85% | 512 MB |
| Tablet High | 60 FPS | 16.67 ms | 100% | 768 MB |
| Desktop Reference | 60 FPS | 16.67 ms | 100% | 1024 MB |

The profile catalog also budgets maximum world actors, placed building pieces and active interactables. Rendering tiers control view distance, anti-aliasing, shadows, post-processing, textures, effects, foliage quality/density, grass density and shadow distance through a fixed CVar allowlist owned by code.

The primary runtime fluidity gate is **p95 frame time**. Average frame time is retained for diagnostics but cannot hide intermittent slow frames. Performance capture is local engineering evidence only and contains no child identity, free text, learning responses, commerce or behavioral analytics.

These values are engineering targets, not representative-device pass claims. M3.8 must execute the vertical slice on representative tablets and retain the resulting capture evidence before any tablet tier is called certified.

## 9. Save/version compatibility

Persistent save data must carry a schema version and support migration. Do not serialize raw UObject implementation details as the long-term backend contract. Prototype saves enforce bounded piece counts and reject invalid transforms before replacing the active world.

Paid entitlement state is not authoritative inside local SaveGame. A local save may reference entitlement-backed content IDs, but ownership must be re-established from the authorized entitlement service.

## 10. Testing

- Unreal Automation Tests for deterministic systems.
- Functional tests for building/mission flows.
- Contract tests for backend APIs.
- Trust Economy schema/policy tests.
- Negative tests proving child runtime cannot invoke payment flows.
- Entitlement restore/revoke/refund tests.
- Web unit/e2e tests once parent portal framework is selected.
- Device performance smoke suite on representative tablets.
- Security/privacy test cases for authorization boundaries.

## 11. Build and CI

GitHub-hosted runners validate source/config/content and phase gates. Native certification uses a Windows x64 self-hosted runner with Unreal Engine 5.8.2. The runner must compile `WorldMakersEditor` and execute the complete `WorldMakers.*` automation namespace through `UnrealEditor-Cmd.exe`.

Repository Quality also executes the Trust Economy policy validator. It fails if required commerce/reward contracts regress or if known payment/ad/premium-currency primitives appear in child runtime source/config/content.

A lightweight GitHub check is never sufficient evidence of Unreal runtime certification. See `docs/prototype-certification.md`.

### M3.8 certification matrix

M3.8 adds a separate **fail-closed** release-certification path. Ordinary PR/push CI validates source contracts but cannot set a certified result. The release workflow requires native Unreal evidence, passed manual smoke, the complete integrated Caribbean Rainforest route, evidence from at least one representative iPadOS tablet and one representative Android tablet, passing M3.7 performance captures, commit/version coherence and SHA-256 evidence integrity.

The canonical machine-readable result is `artifacts/certification/m3-vertical-slice-assessment.json`. It may contain only `CERTIFIED` or `BLOCKED`; the `--require-certified` release mode exits non-zero for every incomplete or inconsistent evidence set.

Representative-device evidence records non-unique engineering metadata only. Hardware serials, child/account identifiers, advertising IDs, free text, learning-answer payloads, commerce fields and behavioral history are prohibited.

## 12. Current locked decisions

- Engine family: Unreal Engine 5.8.
- Certification patch baseline: Unreal Engine 5.8.2.
- Repository architecture: modular monorepo.
- Development model: short-lived branches into `main` through PRs.
- Third-party advertising in child gameplay: prohibited.
- Purchasable premium currency: prohibited.
- Paid randomness/loot boxes/gacha: prohibited.
- Child runtime direct payments: prohibited.
- Paid content ownership: family entitlement model.
- Gameplay rewards: deterministic, non-purchasable, non-transferable, non-convertible.
- M3 release certification: fail-closed evidence matrix; source success is not runtime/device certification.

## 13. Open decisions

- Parent portal production framework.
- Backend/online provider.
- Exact payment-provider/server adapter implementation per platform.
- Pricing/packaging by market and platform.
- Listen vs dedicated multiplayer hosting.
- Cloud save topology.
- Analytics provider or in-house pipeline.
- Production encryption/KMS/secrets platform.
