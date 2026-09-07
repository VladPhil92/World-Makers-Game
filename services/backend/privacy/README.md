# Privacy and child-data boundary

This directory documents the highest-sensitivity architecture in World Makers.

## Data classes

### Restricted identity/consent data

Examples: parent/guardian email, verified account identity, child-parent relationship, consent evidence, support/account-recovery records, legally required age/consent attributes.

Store only when necessary, behind a dedicated access boundary with narrow authorization and retention controls.

### Pseudonymous gameplay data

Examples: internal profile ID, world/build state, cosmetic selections, mission state. It should not need the child's real name or parent email.

### Learning evidence

Examples: objective ID, mission ID, evidence event, timestamp/sequence, derived progress. Use pseudonymous profile IDs and collect only what is necessary for the parent-facing learning purpose.

### Analytics

Minimize aggressively. Never treat third-party advertising analytics as a default. No raw parent/child PII in gameplay events.

## Required production controls

- Verified parent/guardian authorization model.
- Least-privilege service and support access.
- Encryption in transit and at rest.
- Audit trail for sensitive administrative access.
- Export/delete and retention lifecycle.
- Incident-response procedure for child-data exposure.
- Data-processing/vendor inventory.
- Region/transfer assessment.
- Legal/privacy review before production collection.

## Engineering review gates

Any PR changing authentication, profile linking, invitations, telemetry, parental reporting, export/delete, or data retention requires explicit security/privacy review.

## Legal review markers

Before launch, qualified counsel should review applicability and implementation of COPPA, GDPR child provisions/GDPR-K expectations where relevant, Colombia Law 1581 of 2012 and related rules, app-store family/child requirements, privacy notices, consent evidence, international transfers, and deletion/retention obligations.
