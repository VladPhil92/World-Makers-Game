# World Makers External Alpha Playtest

## Purpose

External Alpha is the controlled user-testing gate after Production Alpha Readiness and before Release Candidate Readiness. It does not certify source code, an Unreal build or a device profile by itself. It answers a different question: **did a traceable Production Alpha build survive controlled use by real external testers with acceptable reliability, task completion, safety and issue quality?**

The source contract is `content/production/external-alpha-playtest-v1.json`. Hosted CI validates only that this gate is coherent and fail closed. It cannot manufacture real tester sessions, guardian authorization, telemetry, issues or a human go/no-go decision.

## Entry condition

The tested build must have `Production Alpha` status `CERTIFIED` on the exact same repository commit used by all External Alpha evidence. A bug fix that changes the commit creates a new candidate build and must re-enter the required upstream certification chain.

## Cohort floor

Promotion toward Release Candidate requires at least:

- 10 unique pseudonymous testers;
- 20 recorded sessions;
- 18 completed sessions;
- 4 Win64 sessions;
- 6 Android sessions;
- 6 iPadOS sessions;
- 10 minutes minimum for a session to count as completed.

Raw tester identities are not part of the evidence model. `testerIdHash` is a lowercase SHA-256 pseudonym. If a participant is a minor, the session uses `participantClass=minor-authorized` and requires a hashed guardian/authorized-adult authorization reference. Do not store names, phone numbers, email addresses, school information or other child PII in playtest evidence.

## Experience and reliability SLO

The candidate build must reach all thresholds:

| Metric | Minimum |
| --- | ---: |
| Session completion | 90% |
| Crash-free sessions | 98% |
| Fatal-error-free sessions | 99% |
| Core journey completion | 90% |
| Save/reload success | 95% |
| Mobile background/resume success | 95% |

The core journey remains the same product proof established by G2/Production Alpha: launch, load the certification map, first-person control, observe/scan, science interaction, build/place and persistence.

## Evidence layout

```text
artifacts/external-alpha/
├── cohort.json
├── release-decision.json
├── sessions/
│   ├── session-001.json
│   ├── session-002.json
│   └── telemetry/
│       └── ...
├── issues/
│   └── ...
└── external-alpha-readiness.json
```

Each session telemetry payload is SHA-256 bound to its session JSON. Tampering or path traversal blocks certification.

Templates live under `content/production/`:

- `external-alpha-cohort-template.json`
- `external-alpha-session-template.json`
- `external-alpha-issue-template.json`
- `external-alpha-release-decision-template.json`

Templates always begin in `pending`. Copy them into the evidence area and fill them only with observed/reviewed facts.

## Issue severity and release discipline

External Alpha uses four severities:

- **P0** — catastrophic integrity/safety/security/data-loss failure;
- **P1** — major blocker or widespread crash/gameplay failure;
- **P2** — substantial defect with a viable workaround or limited blast radius;
- **P3** — minor defect/polish issue.

Release Candidate promotion permits zero open P0, zero open P1, at most three open P2 and zero open safety issues. Any observed save corruption or confirmed child-safety incident forces rollback/new build rather than accepting risk on the same commit.

## Child-safety boundary

External Alpha must keep all of these true:

- open chat disabled;
- real-money or token earning disabled;
- no child PII collection;
- no free-text child-data collection;
- privacy review approved;
- child-safety review approved;
- authorized-adult/guardian flow verified whenever minors are included.

These are release gates, not recommendations.

## Promotion and rollback

Promotion is from `external-alpha` to `release-candidate` only. Metrics alone do not promote a build. `release-decision.json` must independently confirm QA, child-safety and product approval, telemetry review and a verified rollback plan.

Rollback/new-build triggers include:

- any open P0/P1;
- crash-free or fatal-error-free SLO regression;
- save corruption;
- confirmed child-safety incident;
- telemetry integrity failure.

A rolled-back or fixed build receives a new commit/build identifier and must produce fresh evidence.

## Certification command

From a clean current `main` synchronized with `origin/main`:

```bat
WorldMakers-ExternalAlpha-Certify.cmd
```

The canonical result is:

```text
artifacts/external-alpha/external-alpha-readiness.json
```

Possible states are `CERTIFIED`, `BLOCKED` and `NON_CERTIFYING_PASS`. Development exceptions may prove that evidence is structurally acceptable, but they cannot promote a non-main or dirty-worktree build.

## Exit condition

A real `CERTIFIED` External Alpha result means the tested Production Alpha build has met the controlled cohort, experience, reliability, issue, privacy and child-safety thresholds and has an explicit human go/no-go approval. The next phase is **Release Candidate Readiness**.
