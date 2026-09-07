# Backend service boundary

This directory defines backend responsibilities without selecting a concrete provider prematurely.

## Domains

- Parent/account identity and verified authorization.
- Child profile pseudonymous mapping.
- Parent-approved social/invitation relationships.
- World/save persistence.
- Learning/progress read models.
- Privacy export/delete/retention orchestration.
- Minimized telemetry ingestion.
- Parent commerce catalog and localized pricing.
- Parent purchase authorization and provider adapters.
- Server-side transaction/receipt verification.
- Durable family entitlements and subscription state.
- Child interest requests with rate limiting and family controls.
- Gameplay reward/progression ledger isolated from commerce value.

## Provider strategy

Implement provider integrations behind adapters. The game and parent portal should depend on stable domain/API contracts rather than PlayFab, Firebase, GameLift, EOS, Apple/Google billing APIs, payment processors, or a custom data store directly.

## Core trust rules

Identity/consent PII, gameplay state, and commerce/payment data are different trust/data zones.

The child-facing runtime:

- must not receive parent contact details or consent evidence;
- must not contain payment-provider SDKs or receipt-validation logic;
- may read authorized entitlement IDs;
- may optionally save a rate-limited offer interest for family review;
- cannot create paid entitlements or execute checkout.

The parent/backend trust zone owns price disclosure, checkout initiation, provider verification, refunds/restores, subscription state and entitlement grants/revocations.

Gameplay rewards are deterministic and have no monetary convertibility.

See:

- `privacy/README.md`
- `contracts/parent-progress.openapi.yaml`
- `contracts/commerce.openapi.yaml`
- `../../docs/trust-economy.md`
