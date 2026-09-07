# Parent Portal

This directory is the independently deployable parent-facing application boundary.

The initial scaffold intentionally avoids locking the team into a production framework before authentication, hosting, accessibility, localization, and product requirements are confirmed. **Next.js + TypeScript** is the current default recommendation for implementation because it is widely supported, server-capable, and appropriate for a small team; that decision should be recorded in an ADR before production development begins.

## Responsibilities

- Parent/guardian authentication.
- Authorized child-profile summaries.
- Play-time summaries.
- Recent build/creation summaries.
- Learning-objective progress summaries.
- Social/invitation controls.
- Privacy/export/delete controls.

## Non-responsibilities

- Direct database access.
- Storage of backend credentials in client bundles.
- Raw analytics or unrestricted child telemetry.
- Public child profiles.

## Local checks

```bash
npm ci
npm run lint
npm test
```

The current scripts validate the scaffold and are replaced by framework-specific lint/test commands when the web stack is initialized.
