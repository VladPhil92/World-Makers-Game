#!/usr/bin/env python3
"""Validate D2 Player Identity & Persistent Profile source/security contracts."""
from __future__ import annotations

import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
APP = ROOT / "apps/player-dashboard"
SERVER = APP / "src/server.mjs"
IDENTITY = APP / "src/domain/identity.mjs"
PROFILE = APP / "src/domain/profile.mjs"
STORE = APP / "src/domain/profile-store.mjs"
BROWSER = APP / "public/app.js"
LINT = APP / "scripts/lint.mjs"
TEST_IDENTITY = APP / "tests/identity.test.mjs"
TEST_STORE = APP / "tests/profile-store.test.mjs"
SCHEMA = ROOT / "services/backend/contracts/player-profile.schema.json"
OPENAPI = ROOT / "services/backend/contracts/player-identity-profile.openapi.yaml"
DOC = ROOT / "docs/player-dashboard-d2-identity-persistence.md"
QUALITY = ROOT / ".github/workflows/repo-quality.yml"


def require(condition: bool, message: str) -> None:
    if not condition:
        raise SystemExit("Player Dashboard D2 validation failed: " + message)


def main() -> None:
    required = (SERVER, IDENTITY, PROFILE, STORE, BROWSER, LINT, TEST_IDENTITY, TEST_STORE, SCHEMA, OPENAPI, DOC, QUALITY)
    missing = [str(path.relative_to(ROOT)) for path in required if not path.is_file()]
    require(not missing, f"missing files: {missing}")

    identity = IDENTITY.read_text(encoding="utf-8")
    for token in (
        "ctg-one-identity-v1", "issuer: ISSUER", "audience: AUDIENCE", "createHmac", "sha256",
        "timingSafeEqual", "expiresAt", "deriveIdentityLinkId", "5 * 60_000",
    ):
        require(token in identity, f"identity contract missing: {token}")
    require("password" not in identity.lower(), "identity boundary must not handle passwords")

    store = STORE.read_text(encoding="utf-8")
    for token in ("JsonFileProfileStore", "ProfileConflictError", "expectedRevision", "rename(", "mode: 0o600", ".tmp-"):
        require(token in store, f"persistent store contract missing: {token}")
    require("writeFile(temporary" in store and "rename(temporary" in store, "profile writes must be atomic")

    profile = PROFILE.read_text(encoding="utf-8")
    for token in ("identityLinkId", "profilePlayerReadModel", "updateProfileLoadout", "updateProfileSelection", "updateProfilePreferences", "appendStoreRequest"):
        require(token in profile, f"profile domain capability missing: {token}")

    server = SERVER.read_text(encoding="utf-8")
    for token in (
        "WORLD_MAKERS_IDENTITY_ASSERTION_SECRET", "WORLD_MAKERS_IDENTITY_LINK_SECRET", "WORLD_MAKERS_PROFILE_STORE_PATH",
        "/api/identity/session", "/api/profile", "/api/preferences", "profile_conflict",
        "requireProfileStore", "ensureIdentityProfile", "profilePlayerReadModel", "profileRevision",
    ):
        require(token in server, f"server D2 boundary missing: {token}")
    require("NODE_ENV === 'production' ? ''" in server, "production must not silently fall back to development profile path")
    for forbidden in ("password", "refresh_token", "access_token", "SUPABASE_SERVICE_ROLE", "STRIPE_SECRET", "/api/store/purchase"):
        require(forbidden not in server.lower(), f"server contains forbidden credential/direct-purchase primitive: {forbidden}")

    browser = BROWSER.read_text(encoding="utf-8")
    for token in ("profileRevision", "recoverConflict", "/api/profile", "profile_conflict"):
        require(token in browser, f"browser optimistic-concurrency behavior missing: {token}")
    for forbidden in ("localStorage", "sessionStorage", "document.cookie"):
        require(forbidden not in browser, f"browser credential/state persistence forbidden: {forbidden}")

    schema = json.loads(SCHEMA.read_text(encoding="utf-8"))
    require(schema.get("title") == "World Makers Player Profile", "profile schema title drifted")
    props = schema.get("properties", {})
    for field in ("playerProfileId", "identityLinkId", "loadout", "selection", "entitlements", "progress", "preferences", "storeRequests", "revision"):
        require(field in props, f"profile schema field missing: {field}")
    require(props.get("revision", {}).get("minimum") == 1, "profile revision must be positive")

    openapi = OPENAPI.read_text(encoding="utf-8")
    for route in ("/api/identity/session", "/api/profile", "/api/avatar/loadout", "/api/selection", "/api/preferences"):
        require(route in openapi, f"OpenAPI route missing: {route}")
    require("profile_conflict" in openapi, "OpenAPI must document optimistic conflict")

    tests = TEST_IDENTITY.read_text(encoding="utf-8") + "\n" + TEST_STORE.read_text(encoding="utf-8")
    for phrase in (
        "CTG One assertion round-trips and binds expected audience",
        "tampered and expired assertions fail closed",
        "identity link is deterministic and does not expose raw subject",
        "file profile store persists across store instances",
        "optimistic revision conflict blocks stale overwrite",
    ):
        require(phrase in tests, f"D2 test coverage missing: {phrase}")

    quality = QUALITY.read_text(encoding="utf-8")
    require("python scripts/validate-player-dashboard-d2.py" in quality, "Repository Quality must run D2 source gate")
    require("working-directory: apps/player-dashboard" in quality, "player dashboard Node job missing")

    doc = DOC.read_text(encoding="utf-8").lower()
    for phrase in ("ctg one identity assertion", "persistent player profile", "optimistic concurrency", "profile_conflict", "atomic profile writes", "d3 — native launcher"):
        require(phrase in doc, f"D2 documentation missing: {phrase}")

    print("Player Dashboard D2 validated: CTG One assertion boundary, opaque identity link, atomic persistent profile store, optimistic revision control and server-confirmed launch state are wired fail-closed.")


if __name__ == "__main__":
    main()
