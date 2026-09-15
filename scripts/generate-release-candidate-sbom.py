#!/usr/bin/env python3
"""Generate a deterministic SPDX 2.3 source/dependency SBOM for a World Makers RC commit."""
from __future__ import annotations

import argparse
import hashlib
import json
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
UPROJECT = ROOT / "game/WorldMakers.uproject"
ENGINE_VERSION = ROOT / "game/UNREAL_ENGINE_VERSION"


def spdx_id(value: str) -> str:
    safe = re.sub(r"[^A-Za-z0-9.-]+", "-", value).strip("-") or "unknown"
    return "SPDXRef-" + safe[:180]


def sha256(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as fh:
        for chunk in iter(lambda: fh.read(1024 * 1024), b""):
            h.update(chunk)
    return h.hexdigest()


def add_package(packages: dict[str, dict], name: str, version: str, supplier: str, source: str) -> None:
    key = f"{name}@{version}|{source}"
    packages[key] = {
        "name": name,
        "SPDXID": spdx_id(key),
        "versionInfo": version or "NOASSERTION",
        "downloadLocation": "NOASSERTION",
        "filesAnalyzed": False,
        "licenseConcluded": "NOASSERTION",
        "licenseDeclared": "NOASSERTION",
        "supplier": supplier,
        "externalRefs": [{"referenceCategory": "OTHER", "referenceType": "worldmakers-source", "referenceLocator": source}],
    }


def package_lock_packages(path: Path, out: dict[str, dict]) -> None:
    try:
        data = json.loads(path.read_text(encoding="utf-8-sig"))
    except Exception:
        return
    rel = path.relative_to(ROOT).as_posix()
    packages = data.get("packages")
    if isinstance(packages, dict):
        for location, meta in sorted(packages.items()):
            if not location or not isinstance(meta, dict):
                continue
            name = meta.get("name") or location.rsplit("node_modules/", 1)[-1]
            version = str(meta.get("version") or "NOASSERTION")
            add_package(out, str(name), version, "NOASSERTION", f"{rel}:{location}")
        return
    dependencies = data.get("dependencies")
    if isinstance(dependencies, dict):
        for name, meta in sorted(dependencies.items()):
            if isinstance(meta, dict):
                add_package(out, str(name), str(meta.get("version") or "NOASSERTION"), "NOASSERTION", rel)


def generate(commit: str) -> dict:
    engine = ENGINE_VERSION.read_text(encoding="utf-8").strip()
    uproject = json.loads(UPROJECT.read_text(encoding="utf-8"))
    packages: dict[str, dict] = {}
    add_package(packages, "World Makers", commit[:12], "Organization: CTG One Technology", "repository-root")
    add_package(packages, "Unreal Engine", engine, "Organization: Epic Games", "game/UNREAL_ENGINE_VERSION")
    for plugin in sorted(uproject.get("Plugins", []), key=lambda row: row.get("Name", "")):
        if plugin.get("Enabled") is True and plugin.get("Name"):
            add_package(packages, f"Unreal Plugin: {plugin['Name']}", engine, "NOASSERTION", "game/WorldMakers.uproject")
    lockfiles = sorted(ROOT.rglob("package-lock.json"))
    for path in lockfiles:
        if any(part in {"node_modules", "artifacts", ".git"} for part in path.parts):
            continue
        package_lock_packages(path, packages)
    lockfile_evidence = [{"path": p.relative_to(ROOT).as_posix(), "sha256": sha256(p)} for p in lockfiles if "node_modules" not in p.parts and "artifacts" not in p.parts]
    namespace = f"https://worldmakers.ctgone.com/spdx/{commit}"
    return {
        "spdxVersion": "SPDX-2.3",
        "dataLicense": "CC0-1.0",
        "SPDXID": "SPDXRef-DOCUMENT",
        "name": f"World-Makers-RC-{commit[:12]}",
        "documentNamespace": namespace,
        "creationInfo": {"creators": ["Tool: scripts/generate-release-candidate-sbom.py"], "comment": "Deterministic source/dependency inventory; generated without network resolution."},
        "packages": [packages[key] for key in sorted(packages)],
        "annotations": [{"annotationType": "OTHER", "annotator": "Tool: World Makers RC SBOM", "annotationDate": "1970-01-01T00:00:00Z", "comment": json.dumps({"repositoryCommit": commit, "lockfiles": lockfile_evidence}, sort_keys=True)}],
    }


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--commit", required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    if not re.fullmatch(r"[0-9a-f]{40}", args.commit):
        parser.error("--commit must be a full lowercase git SHA")
    payload = generate(args.commit)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(payload, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    print(f"SPDX 2.3 SBOM generated: {args.output} ({len(payload['packages'])} packages/components)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
