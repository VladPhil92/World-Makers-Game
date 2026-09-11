#!/usr/bin/env python3
"""Validate D1 World Makers Player Dashboard source/product/security contracts."""
from __future__ import annotations

import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
APP = ROOT / "apps/player-dashboard"
PACKAGE = APP / "package.json"
LOCK = APP / "package-lock.json"
SERVER = APP / "src/server.mjs"
CATALOG = APP / "src/domain/catalog.mjs"
LAUNCH = APP / "src/domain/launch.mjs"
HTML = APP / "public/index.html"
JS = APP / "public/app.js"
CSS = APP / "public/app.css"
LINT = APP / "scripts/lint.mjs"
TEST_CATALOG = APP / "tests/catalog.test.mjs"
TEST_LAUNCH = APP / "tests/launch.test.mjs"
DOC = ROOT / "docs/player-dashboard-d1.md"
QUALITY = ROOT / ".github/workflows/repo-quality.yml"


def require(condition: bool, message: str) -> None:
    if not condition:
        raise SystemExit("Player Dashboard D1 validation failed: " + message)


def main() -> None:
    required = (PACKAGE, LOCK, SERVER, CATALOG, LAUNCH, HTML, JS, CSS, LINT, TEST_CATALOG, TEST_LAUNCH, DOC, QUALITY)
    missing = [str(path.relative_to(ROOT)) for path in required if not path.is_file()]
    require(not missing, f"missing files: {missing}")

    pkg = json.loads(PACKAGE.read_text(encoding="utf-8"))
    require(pkg.get("name") == "@world-makers/player-dashboard", "package identity drifted")
    require(pkg.get("private") is True, "player dashboard must remain private")
    require(not pkg.get("dependencies"), "D1 must remain dependency-minimized")
    require(pkg.get("engines", {}).get("node") == ">=22", "D1 requires locked Node 22+")

    server = SERVER.read_text(encoding="utf-8")
    for token in (
        "WORLD_MAKERS_ALLOW_DEMO_AUTH",
        "WORLD_MAKERS_LAUNCH_SIGNING_SECRET",
        "HttpOnly",
        "SameSite=Strict",
        "Content-Security-Policy",
        "/api/avatar/loadout",
        "/api/selection",
        "/api/store/requests",
        "/api/launch-context",
        "launch_signing_not_configured",
    ):
        require(token in server, f"server contract missing: {token}")
    for forbidden in ("/api/store/purchase", "STRIPE_SECRET", "SUPABASE_SERVICE_ROLE", "PRIVATE_KEY"):
        require(forbidden not in server, f"D1 server contains forbidden privileged/direct-purchase path: {forbidden}")

    catalog = CATALOG.read_text(encoding="utf-8")
    for mode in ("free-explore", "missions", "laboratory", "cooperative"):
        require(f"id: '{mode}'" in catalog, f"game mode missing: {mode}")
    require("playable: false" in catalog, "cooperative mode must remain visibly locked")
    for slot in ("hair", "top", "bottom", "footwear", "head-accessory", "back-accessory", "hand-prop"):
        require(slot in catalog, f"avatar cosmetic slot missing: {slot}")
    for world in ("world.rainforest", "world.research-island", "world.impossible-city", "world.moonforge"):
        require(world in catalog, f"world descriptor missing: {world}")
    require("parent-approval-required" in catalog, "store must require parental approval")

    launch = LAUNCH.read_text(encoding="utf-8")
    for token in ("createHmac", "sha256", "timingSafeEqual", "expiresAt", "avatarLoadout", "inventoryEntitlements"):
        require(token in launch, f"launch contract missing: {token}")
    require("ttlSeconds = defaultTtlSeconds" in launch, "launch TTL contract missing")

    html = HTML.read_text(encoding="utf-8")
    for marker in ("PLAYER HUB", "AVATAR STUDIO", "GAME MODES", "WORLD SELECT", "STORE", "JUGAR", "Saltar al contenido principal"):
        require(marker in html, f"player UI marker missing: {marker}")

    browser = JS.read_text(encoding="utf-8")
    for forbidden in ("innerHTML", "outerHTML", "localStorage", "sessionStorage", "document.cookie", "eval("):
        require(forbidden not in browser, f"unsafe browser primitive detected: {forbidden}")
    for required_token in ("selectMode", "selectWorld", "selectMission", "saveCosmetic", "requestStoreItem", "launchGame"):
        require(required_token in browser, f"interactive dashboard capability missing: {required_token}")

    tests = TEST_CATALOG.read_text(encoding="utf-8") + "\n" + TEST_LAUNCH.read_text(encoding="utf-8")
    for phrase in (
        "three playable modes plus locked cooperative",
        "avatar loadout covers stable P3 cosmetic slots",
        "world selection is constrained by mode compatibility",
        "store is cosmetic-only and parent approval gated",
        "launch context is short-lived, signed and carries player configuration",
        "tampered launch context is rejected",
    ):
        require(phrase in tests, f"D1 test coverage missing: {phrase}")

    quality = QUALITY.read_text(encoding="utf-8")
    require("python scripts/validate-player-dashboard-d1.py" in quality, "Repository Quality must run D1 source gate")
    require("working-directory: apps/player-dashboard" in quality, "Repository Quality must execute player-dashboard Node checks")
    require("apps/player-dashboard/package-lock.json" in quality, "player dashboard npm cache boundary missing")

    doc = DOC.read_text(encoding="utf-8").lower()
    for phrase in ("authentication → player dashboard", "exploración libre", "aventuras & misiones", "laboratorio", "cooperativo", "parent-approval-required", "worldmakers-launch-v1"):
        require(phrase in doc, f"D1 documentation missing: {phrase}")

    print("Player Dashboard D1 validated: avatar studio, three playable modes, world/mission compatibility, parent-gated cosmetic store and signed short-lived Unreal launch context are wired fail-closed.")


if __name__ == "__main__":
    main()
