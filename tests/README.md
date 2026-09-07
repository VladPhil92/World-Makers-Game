# Test strategy

## Unreal

Use Unreal Automation Tests for deterministic systems and Functional Tests for player flows such as building placement, mission completion, save/load, and private-session authorization.

## Backend/contracts

Add contract tests for authorization, schema compatibility, pseudonymous identifiers, export/delete flows, and provider adapters.

## Parent portal

The scaffold currently runs dependency-free checks. Once the framework is selected, add unit tests, component/accessibility tests, API mocking, and end-to-end tests for authenticated parent workflows.

## Performance

Maintain a representative-device matrix for tablets. Track frame time, memory, texture pool, shader/material hotspots, load/streaming behavior, and thermal/session degradation.

## Security/privacy

Include explicit negative tests for cross-account child profile access, invite authorization bypass, profile enumeration, stale/revoked parent links, and analytics payload PII leakage.
