#!/usr/bin/env python3
"""Validate the World Makers G2 authored vertical-slice contract.

Hosted/default mode validates the production contract and authoring infrastructure.
--require-authored-map additionally requires the real Unreal .umap, Git LFS
tracking, and deliberate authored rainforest takeover approval.
"""

from __future__ import annotations

import argparse
import json
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
CONTRACT_PATH = ROOT / "content" / "production" / "g2-authored-vertical-slice-v1.json"
REVIEW_TEMPLATE = ROOT / "content" / "production" / "g2-route-review-template.json"
REGISTRY_PATH = ROOT / "content" / "visual" / "authored" / "authored-assets-p1.json"
UPROJECT_PATH = ROOT / "game" / "WorldMakers.uproject"
ENGINE_VERSION_PATH = ROOT / "game" / "UNREAL_ENGINE_VERSION"
GITATTRIBUTES_PATH = ROOT / ".gitattributes"
AUTHOR_SCRIPT = ROOT / "scripts" / "unreal" / "author-g2-certification-map.py"
INSPECT_SCRIPT = ROOT / "scripts" / "unreal" / "inspect-g2-certification-map.py"
ORCHESTRATOR = ROOT / "scripts" / "run-g2-authored-certification.ps1"
DOC_PATH = ROOT / "docs" / "g2-authored-vertical-slice.md"
WORKFLOW_PATH = ROOT / ".github" / "workflows" / "g2-authored-vertical-slice.yml"
AUTHOR_LAUNCHER = ROOT / "WorldMakers-G2-Author.cmd"
CERTIFY_LAUNCHER = ROOT / "WorldMakers-G2-Certify.cmd"

EXPECTED_SCHEMA = "worldmakers.g2-authored-vertical-slice.v1"
EXPECTED_ENGINE = "5.8.2"
EXPECTED_MAP_PACKAGE = "/Game/WorldMakers/Maps/WM_PrototypeCertification"
EXPECTED_MAP_DISK = "game/Content/WorldMakers/Maps/WM_PrototypeCertification.umap"
EXPECTED_GAME_MODE = "/Script/WorldMakers.WMGameMode"
EXPECTED_ACTORS = {
    ("player-start", "/Script/Engine.PlayerStart", "WM_G2_PlayerStart"),
    ("rainforest-environment", "/Script/WorldMakers.WMCaribbeanRainforestPrototype", "WM_G2_Rainforest"),
    ("mission-geometry", "/Script/WorldMakers.WMMissionGeometryActor", "WM_G2_MissionGeometry"),
}
EXPECTED_ASSET_IDS = {
    "environment.rainforest.ground.a",
    "environment.rainforest.terrain.a",
    "environment.rainforest.tree.a",
    "environment.rainforest.tree.b",
    "environment.rainforest.tree.c",
    "environment.rainforest.understory.a",
    "environment.rainforest.rock.a",
    "environment.rainforest.water-edge.a",
    "environment.rainforest.hero-ceiba",
}
EXPECTED_CAPABILITIES = {
    "spawn-and-first-person-control",
    "observe-and-scan",
    "collect-resource",
    "perform-science-interaction",
    "craft-or-transform-material",
    "build-or-place-intervention",
    "trigger-visible-ecosystem-consequence",
    "complete-mission-evidence",
    "save-and-load-state",
}


def fail(message: str) -> None:
    print(f"ERROR: {message}")
    raise SystemExit(1)


def require(condition: bool, message: str) -> None:
    if not condition:
        fail(message)


def read_text(path: Path) -> str:
    require(path.is_file(), f"missing required file: {path.relative_to(ROOT)}")
    return path.read_text(encoding="utf-8-sig")


def read_json(path: Path):
    try:
        return json.loads(read_text(path))
    except json.JSONDecodeError as exc:
        fail(f"invalid JSON in {path.relative_to(ROOT)}: {exc}")


def require_tokens(path: Path, tokens: tuple[str, ...]) -> None:
    content = read_text(path)
    for token in tokens:
        require(token in content, f"{path.relative_to(ROOT)} missing required token: {token}")


def git_lfs_filter_for(path: Path) -> str:
    relative = path.relative_to(ROOT).as_posix()
    proc = subprocess.run(
        ["git", "-C", str(ROOT), "check-attr", "filter", "--", relative],
        capture_output=True,
        text=True,
        check=False,
    )
    require(proc.returncode == 0, f"git check-attr failed for {relative}")
    return proc.stdout.strip()


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--require-authored-map", action="store_true")
    args = parser.parse_args()

    contract = read_json(CONTRACT_PATH)
    require(contract.get("schema") == EXPECTED_SCHEMA, "G2 schema drift")
    require(contract.get("status") == "source-ready-native-authorship-required", "G2 source/native status drift")
    require(contract.get("engine", {}).get("expectedVersion") == EXPECTED_ENGINE, "G2 engine version drift")
    require(contract.get("engine", {}).get("project") == "game/WorldMakers.uproject", "G2 project path drift")

    map_contract = contract.get("map", {})
    require(map_contract.get("packagePath") == EXPECTED_MAP_PACKAGE, "G2 map package path drift")
    require(map_contract.get("diskPath") == EXPECTED_MAP_DISK, "G2 map disk path drift")
    require(map_contract.get("gitLfsRequired") is True, "G2 map must require Git LFS")
    require(contract.get("worldSettings", {}).get("gameModeClass") == EXPECTED_GAME_MODE, "G2 GameMode drift")

    actual_actors = {
        (entry.get("role"), entry.get("classPath"), entry.get("label"))
        for entry in contract.get("requiredAuthoredActors", [])
    }
    require(actual_actors == EXPECTED_ACTORS, "G2 authored actor contract drift")

    actual_asset_ids = set(contract.get("requiredAuthoredEnvironmentAssetIds", []))
    require(actual_asset_ids == EXPECTED_ASSET_IDS, "G2 rainforest asset ID set drift")
    require(contract.get("assetRegistry") == "content/visual/authored/authored-assets-p1.json", "G2 registry path drift")

    route = contract.get("route", {})
    require(set(route.get("requiredCapabilities", [])) == EXPECTED_CAPABILITIES, "G2 route capability set drift")
    require(route.get("manualRouteReviewRequired") is True, "G2 must require manual route review")
    require(route.get("manualReviewTemplate") == "content/production/g2-route-review-template.json", "G2 review template path drift")

    certification = contract.get("certification", {})
    for field in (
        "requiresG1Certified",
        "requiresAuthoredMap",
        "requiresNativeMapInspection",
        "requiresAllEnvironmentAssetsLoadable",
        "requiresAuthoredPresentForEnvironmentAssets",
        "requiresManualRouteReview",
    ):
        require(certification.get(field) is True, f"G2 certification must require {field}")
    require(certification.get("resultStates") == ["CERTIFIED", "BLOCKED", "NON_CERTIFYING_PASS"], "G2 result-state contract drift")

    engine_version = read_text(ENGINE_VERSION_PATH).strip()
    require(engine_version == EXPECTED_ENGINE, f"UNREAL_ENGINE_VERSION is {engine_version!r}, expected {EXPECTED_ENGINE}")

    uproject = read_json(UPROJECT_PATH)
    enabled_plugins = {p.get("Name") for p in uproject.get("Plugins", []) if p.get("Enabled") is True}
    require("PythonScriptPlugin" in enabled_plugins, "G2 authoring requires PythonScriptPlugin")
    require("EditorScriptingUtilities" in enabled_plugins, "G2 authoring requires EditorScriptingUtilities")

    attrs = read_text(GITATTRIBUTES_PATH)
    require("*.umap filter=lfs" in attrs, "*.umap must remain tracked by Git LFS")
    require("*.uasset filter=lfs" in attrs, "*.uasset must remain tracked by Git LFS")

    registry = read_json(REGISTRY_PATH)
    entries = registry.get("assets", [])
    ids = [entry.get("id") for entry in entries]
    require(len(ids) == len(set(ids)), "P1 authored asset registry contains duplicate IDs")
    by_id = {entry.get("id"): entry for entry in entries}
    for asset_id in EXPECTED_ASSET_IDS:
        require(asset_id in by_id, f"P1 registry missing G2 asset ID: {asset_id}")
        entry = by_id[asset_id]
        require(entry.get("kind") == "StaticMesh", f"G2 environment asset must be StaticMesh: {asset_id}")
        object_path = entry.get("objectPath", "")
        require(object_path.startswith("/Game/WorldMakers/Environment/Rainforest/"), f"unstable G2 object path: {asset_id}")
        require(entry.get("fallbackMode") == "procedural", f"G2 takeover must preserve procedural fallback before approval: {asset_id}")

    review = read_json(REVIEW_TEMPLATE)
    require(review.get("schema") == "worldmakers.g2-route-review.v1", "G2 review template schema drift")
    require(review.get("status") == "pending", "G2 review template must remain pending")
    require(set(review.get("capabilities", {}).keys()) == EXPECTED_CAPABILITIES, "G2 review template capability drift")
    require(all(not bool(v.get("passed")) for v in review.get("capabilities", {}).values()), "G2 review template may not pre-pass capabilities")

    require_tokens(AUTHOR_SCRIPT, (EXPECTED_MAP_PACKAGE, "WM_G2_PlayerStart", "WM_G2_Rainforest", "WM_G2_MissionGeometry", "new_level", "save_current_level"))
    require_tokens(INSPECT_SCRIPT, (EXPECTED_MAP_PACKAGE, EXPECTED_GAME_MODE, "authoredPresent", "g2-native-map-inspection"))
    require_tokens(ORCHESTRATOR, ("-AuthorMap", "run-g1-native-certification.ps1", "--require-authored-map", "CERTIFIED", "NON_CERTIFYING_PASS", "BLOCKED"))
    require_tokens(AUTHOR_LAUNCHER, ("run-g2-authored-certification.ps1", "-AuthorMap"))
    require_tokens(CERTIFY_LAUNCHER, ("run-g2-authored-certification.ps1",))
    require_tokens(DOC_PATH, ("G2", EXPECTED_MAP_PACKAGE, "WorldMakers-G2-Author.cmd", "WorldMakers-G2-Certify.cmd", "G3"))
    require_tokens(WORKFLOW_PATH, ("G2 Authored Vertical Slice", "validate-g2-authored-vertical-slice.py", "self-hosted", "g2-native-map-inspection"))

    map_disk = ROOT / EXPECTED_MAP_DISK
    takeover_ready = all(bool(by_id[asset_id].get("authoredPresent")) for asset_id in EXPECTED_ASSET_IDS)

    if args.require_authored_map:
        require(map_disk.is_file(), f"real Unreal map is missing: {EXPECTED_MAP_DISK}")
        require("filter: lfs" in git_lfs_filter_for(map_disk), "G2 .umap is not resolved to Git LFS filter=lfs")
        require(takeover_ready, "all nine G2 rainforest assets must be deliberately authoredPresent=true before G2 certification")

    print("World Makers G2 Authored Vertical Slice source contract: PASS")
    print(f"  Map present: {map_disk.is_file()}")
    print(f"  Rainforest authored takeover approved: {takeover_ready}")
    print(f"  Require authored map mode: {args.require_authored_map}")
    if not map_disk.is_file():
        print("  Native authoring remains required; source validation is not G2 certification.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
