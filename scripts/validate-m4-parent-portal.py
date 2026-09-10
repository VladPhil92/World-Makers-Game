#!/usr/bin/env python3
"""Validate M4.1 Parent Portal MVP source/security contracts."""

from __future__ import annotations

import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
PORTAL = ROOT / "apps/parent-portal"

REQUIRED_FILES = (
    "apps/parent-portal/src/domain/authorization.mjs",
    "apps/parent-portal/src/domain/dashboard.mjs",
    "apps/parent-portal/src/domain/privacy.mjs",
    "apps/parent-portal/src/data/demo-family.mjs",
    "apps/parent-portal/src/server.mjs",
    "apps/parent-portal/public/index.html",
    "apps/parent-portal/public/app.css",
    "apps/parent-portal/public/app.js",
    "apps/parent-portal/tests/parent-portal.test.mjs",
    "apps/parent-portal/scripts/lint.mjs",
    "services/backend/contracts/parent-portal.openapi.yaml",
    "services/backend/contracts/parent-family-dashboard.schema.json",
    "docs/adr/ADR-0001-parent-portal-stack.md",
    "docs/m4-parent-portal-mvp.md",
)


def require(condition: bool, message: str) -> None:
    if not condition:
        raise SystemExit(message)


def read(path: str) -> str:
    return (ROOT / path).read_text(encoding="utf-8")


def main() -> None:
    missing = [path for path in REQUIRED_FILES if not (ROOT / path).is_file()]
    require(not missing, f"Missing M4.1 files: {missing}")

    package = json.loads(read("apps/parent-portal/package.json"))
    require(package.get("private") is True, "Parent portal package must remain private")
    require(package.get("engines", {}).get("node") == ">=22", "M4.1 runtime baseline must remain Node >=22")
    require(not package.get("dependencies"), "M4.1 must remain dependency-minimized until M4.2 provider selection")
    scripts = package.get("scripts", {})
    for name in ("start", "lint", "test"):
        require(name in scripts, f"Parent portal package missing script: {name}")

    env = read("apps/parent-portal/.env.example")
    require("WORLD_MAKERS_ALLOW_DEMO_AUTH=false" in env, "Demo auth must be opt-in and disabled in env example")
    for forbidden in ("PRIVATE_KEY=", "SERVICE_ROLE=", "STRIPE_SECRET=", "DATABASE_PASSWORD="):
        require(forbidden not in env, f"Browser-facing env example must not contain privileged secret slot: {forbidden}")

    auth = read("apps/parent-portal/src/domain/authorization.mjs")
    for token in ("listAuthorizedChildren", "requireAuthorizedChild", "AuthorizationError", "not_authorized", "avoid enumeration"):
        require(token in auth, f"Authorization contract missing: {token}")
    require("authorizedParentIds" in auth, "Child relationship authorization must be explicit")

    dashboard = read("apps/parent-portal/src/domain/dashboard.mjs")
    for token in ("last7DaysMinutes", "sessionsLast7Days", "recentBuilds", "adventures", "rightToStop", "pressureLoopsDetected"):
        require(token in dashboard, f"Parent dashboard projection missing: {token}")
    for forbidden in ("chatMessage", "learningAnswer", "advertisingId", "preciseLocation", "paymentToken", "purchaseHistory"):
        require(forbidden.lower() in dashboard.lower(), f"Dashboard minimization guard must name forbidden field: {forbidden}")

    privacy = read("apps/parent-portal/src/domain/privacy.mjs")
    for operation in ("export-child-data", "delete-child-data", "unlink-child-profile"):
        require(operation in privacy, f"Privacy operation missing: {operation}")
    require("acknowledged !== true" in privacy, "Destructive/privacy requests must require explicit acknowledgement")
    require("^[A-Z0-9]{8}$" in privacy, "Link code must be bounded to eight alphanumeric characters")

    server = read("apps/parent-portal/src/server.mjs")
    for token in (
        "Content-Security-Policy",
        "HttpOnly",
        "SameSite=Strict",
        "Permissions-Policy",
        "payment=()",
        "Cache-Control",
        "no-store",
        "WORLD_MAKERS_ALLOW_DEMO_AUTH",
        "assertSameOrigin",
        "16_384",
        "pending-backend-verification",
        "requireSession",
        "requireAuthorizedChild",
    ):
        require(token in server, f"Parent portal server boundary missing: {token}")
    require("Access-Control-Allow-Origin', '*'" not in server and 'Access-Control-Allow-Origin", "*"' not in server, "Parent APIs must not enable wildcard CORS")
    for forbidden in ("database.query(", "prisma.", "SUPABASE_SERVICE_ROLE", "STRIPE_SECRET", "PRIVATE_KEY"):
        require(forbidden not in server, f"Portal server must remain provider-neutral and unprivileged: {forbidden}")

    browser = read("apps/parent-portal/public/app.js")
    for forbidden in ("innerHTML", "outerHTML", "eval(", "localStorage", "sessionStorage"):
        require(forbidden not in browser, f"Unsafe browser primitive detected: {forbidden}")
    for endpoint in ("/api/session", "/api/dashboard", "/api/link-requests", "/api/privacy-requests"):
        require(endpoint in browser, f"Browser integration missing endpoint: {endpoint}")

    html = read("apps/parent-portal/public/index.html")
    for token in ("Skip to main content", "aria-live=\"polite\"", "Family Space", "privacy-ack", "Open secure demo"):
        require(token in html, f"Parent portal accessibility/product marker missing: {token}")
    css = read("apps/parent-portal/public/app.css")
    require("prefers-reduced-motion" in css and "focus-visible" in css and "min-height: 44px" in css, "Accessibility CSS baseline is incomplete")

    schema = json.loads(read("services/backend/contracts/parent-family-dashboard.schema.json"))
    require(schema.get("additionalProperties") is False, "Parent dashboard schema must fail closed on unknown root fields")
    require(schema.get("properties", {}).get("wellbeing", {}).get("additionalProperties") is False, "Wellbeing read model must be closed")

    openapi = read("services/backend/contracts/parent-portal.openapi.yaml")
    for token in ("parentSession", "server-side relationship authorization", "pending-backend-verification", "acknowledged: { const: true }"):
        require(token in openapi, f"Parent API contract missing security marker: {token}")
    require("direct database" in openapi.lower(), "OpenAPI must document browser/database separation")

    tests = read("apps/parent-portal/tests/parent-portal.test.mjs")
    for name in (
        "M4 authorization exposes only linked child profiles",
        "M4 dashboard is a minimized read model without forbidden telemetry",
        "M4 privacy operations require explicit guardian acknowledgement",
        "M4 HTTP boundary requires a session and prevents cross-profile access",
    ):
        require(name in tests, f"Missing M4.1 test: {name}")

    docs = read("docs/m4-parent-portal-mvp.md").lower()
    for boundary in ("source-complete", "production identity", "fails closed", "right to stop", "m4.2"):
        require(boundary in docs, f"M4.1 documentation boundary missing: {boundary}")
    adr = read("docs/adr/ADR-0001-parent-portal-stack.md").lower()
    require("dependency-minimized" in adr and "next.js" in adr and "migration trigger" in adr, "Parent portal stack ADR is incomplete")

    workflow = read(".github/workflows/repo-quality.yml")
    require("python scripts/validate-m4-parent-portal.py" in workflow, "M4.1 validator must run in Repository Quality")
    for command in ("npm ci", "npm run lint", "npm test"):
        require(command in workflow, f"Repository Quality must retain parent portal command: {command}")

    roadmap = read("docs/roadmap.md")
    require("M4.1 — parent portal MVP" in roadmap and "source-complete" in roadmap, "Roadmap must mark M4.1 source-complete")
    require("M4.2 — production identity, durable family linking and backend adapters: next" in roadmap, "Roadmap must identify M4.2 as next")
    require("M3.8 — vertical-slice certification infrastructure: source-complete" in roadmap, "M4 must not erase the M3.8 source/certification boundary")

    print("M4.1 Parent Portal MVP validated: server-side relationship guard, minimized family read model, explicit privacy/link requests, accessible UI and fail-closed production auth boundary are present.")


if __name__ == "__main__":
    main()
