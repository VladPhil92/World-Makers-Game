# Security Policy

## Scope

World Makers is designed for children ages 4–10 and therefore treats identity, parental consent, multiplayer permissions, telemetry, and progress reporting as high-sensitivity systems.

## Reporting vulnerabilities

Do not disclose exploitable vulnerabilities, child-data exposure, authentication bypasses, invitation abuse, or moderation/safety weaknesses in a public GitHub issue. Contact the project owner through a private channel and include reproduction steps, affected component, impact, and mitigations if known.

## Child-data architecture rules

- Minimize data collection by default.
- Separate parent/account PII from gameplay state and pedagogical progress.
- Use pseudonymous internal child/profile IDs outside the identity service.
- Do not store raw child PII in analytics events.
- Require server authorization for parent-child relationships and multiplayer invitations.
- Use allowlisted social interactions; no public stranger discovery or open stranger chat.
- Encrypt sensitive data in transit and at rest in production infrastructure.
- Define retention/deletion workflows before collecting production child data.
- Never commit real user exports, access tokens, secrets, consent evidence, or production databases.

## Legal review gates

Before production collection of child data, obtain qualified legal review covering at minimum COPPA where applicable, GDPR child provisions where applicable, Colombia Law 1581 of 2012 and related data-protection requirements, parental-consent design, privacy notices, international transfers, retention/deletion, and app-store child/family policies.

This file is an engineering security baseline, not legal advice.
