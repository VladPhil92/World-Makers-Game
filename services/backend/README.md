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

## Provider strategy

Implement provider integrations behind adapters. The game and parent portal should depend on stable domain/API contracts rather than PlayFab, Firebase, GameLift, EOS, or a custom data store directly.

## Core security rule

Identity/consent PII and gameplay state are different trust/data zones. The child-facing runtime should not receive parent contact details or consent evidence. Parent-facing APIs must enforce relationship authorization server-side.

See `privacy/README.md` and `contracts/parent-progress.openapi.yaml`.
