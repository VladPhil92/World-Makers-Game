#!/usr/bin/env python3
"""Validate the World Makers cloud/runtime boundary without claiming live production health."""
from __future__ import annotations

import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
FILES = {
    "gateway": ROOT / "apps/worldmakers-api/src/gateway.mjs",
    "server": ROOT / "apps/worldmakers-api/src/server.mjs",
    "player_store": ROOT / "apps/player-dashboard/src/domain/supabase-profile-store.mjs",
    "parent_client": ROOT / "apps/parent-portal/src/domain/supabase-client.mjs",
    "migration": ROOT / "infra/supabase/migrations/20260915202500_secure_dashboard_profile_bridge_v1.sql",
    "workflow": ROOT / ".github/workflows/cloud-runtime-readiness.yml",
}


def require(condition: bool, message: str) -> None:
    if not condition:
        raise SystemExit(f"Cloud Runtime Readiness validation FAILED: {message}")


def text(name: str) -> str:
    path = FILES[name]
    require(path.is_file(), f"missing {path.relative_to(ROOT)}")
    return path.read_text(encoding="utf-8")


def main() -> int:
    gateway = text("gateway")
    server = text("server")
    player_store = text("player_store")
    parent_client = text("parent_client")
    migration = text("migration")
    workflow = text("workflow")

    require("https:" in gateway and "redirect: 'error'" in gateway, "runtime gateway must enforce HTTPS/no redirects")
    require("Idempotency-Key" in gateway and "expectedRevision" in gateway, "runtime writes must remain idempotent/revision controlled")
    require("cookie" not in gateway.lower(), "runtime gateway must not forward cookies")
    require("/api/health" in server and "/api/ready" in server, "runtime API needs liveness and readiness endpoints")
    require("WORLD_MAKERS_PROFILE_BRIDGE_SECRET" in player_store, "player dashboard must use the server-only profile bridge")
    require("wm_secure_player_profile" in player_store, "player dashboard must use secure profile RPC")
    require("wm_secure_player_profile" in parent_client, "parent privacy deletion must use secure profile RPC")
    require("wm_delete_player_profile" in parent_client, "legacy delete call must be explicitly intercepted")
    require("pg_advisory_xact_lock" in migration, "profile mutations must serialize first-write races")
    require("ec77329d084a6b40a6e54993cbca1ba2c789c837e02b61eb3a53a72cb6598991" in migration, "profile bridge digest drift")
    require("grant execute" in migration and "to anon, service_role" in migration, "secure wrapper RPC grants are incomplete")

    combined_apps = player_store + parent_client + gateway + server
    require("SUPABASE_SERVICE_ROLE_KEY" not in combined_apps, "app/runtime source must not depend on a Supabase service-role key")
    require("serviceRoleKey" not in combined_apps, "app/runtime source must not carry service-role credentials")
    require("Cloud Runtime Readiness" in workflow and "apps/worldmakers-api" in workflow, "dedicated CI workflow missing")

    package = json.loads((ROOT / "apps/worldmakers-api/package.json").read_text(encoding="utf-8"))
    require(package.get("engines", {}).get("node") == ">=22", "runtime API Node baseline mismatch")

    print("Cloud Runtime Readiness source contract: PASS")
    print("Live Railway/Supabase/CTG One health remains an operational verification step.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
