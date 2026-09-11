#!/usr/bin/env python3
"""Repository Quality gate for N1 Native Authored Integration Candidate."""
from __future__ import annotations

import json
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
CONTRACT = ROOT / "content/visual/authored/n1-native-integration.json"
P1 = ROOT / "content/visual/authored/authored-assets-p1.json"
ASSESSOR = ROOT / "scripts/assess-n1-native-integration.py"
WORKFLOW = ROOT / ".github/workflows/n1-native-authored-integration.yml"
REPO_QUALITY = ROOT / ".github/workflows/repo-quality.yml"
DOC = ROOT / "docs/n1-native-authored-integration.md"
P2_IMPORT = ROOT / "scripts/unreal/import-p2-rainforest-assets.py"
P3_IMPORT = ROOT / "scripts/unreal/import-p3-character-assets.py"
P4_IMPORT = ROOT / "scripts/unreal/import-p4-animation-presentation.py"
P5_INVENTORY = ROOT / "scripts/unreal/collect-p5-native-inventory.py"


def require(condition: bool, message: str) -> None:
    if not condition:
        raise SystemExit("N1 validation failed: " + message)


def load(path: Path):
    return json.loads(path.read_text(encoding="utf-8"))


def main() -> None:
    required_files = (CONTRACT, P1, ASSESSOR, WORKFLOW, REPO_QUALITY, DOC, P2_IMPORT, P3_IMPORT, P4_IMPORT, P5_INVENTORY)
    missing = [str(p.relative_to(ROOT)) for p in required_files if not p.is_file()]
    require(not missing, f"missing required files: {missing}")

    contract = load(CONTRACT)
    require(contract.get("schemaVersion") == 1 and contract.get("phase") == "N1", "invalid N1 contract identity")
    require(contract.get("activationPolicy") == "all-or-nothing", "activation must remain all-or-nothing")
    require(contract.get("automaticRegistryMutation") is False, "N1 may not mutate the P1 registry automatically")
    expected = {
        "p1RegistryAssets": 16,
        "p2RainforestAssets": 9,
        "p3CharacterModules": 8,
        "p4AnimationAssets": 17,
        "p4LevelSequences": 2,
        "p5VfxAssets": 17,
        "p5PresentationAssets": 7,
    }
    require(contract.get("required") == expected, "N1 required-count contract drifted")
    rules = contract.get("candidateRules", {})
    for key in (
        "sameBuildCommit",
        "allP2ImportsPass",
        "allP3ImportsPass",
        "allP4AnimationImportsPass",
        "allP4SequenceContainersPresent",
        "allP5RegistryAssetsExist",
        "allP5AnimationsExist",
        "allP5VfxExist",
        "allP5PresentationAssetsExist",
        "candidateMayOnlySetAuthoredPresentTrue",
        "candidateMustPreserveEveryOtherP1Field",
        "humanReviewRequiredBeforeApplyingCandidate",
    ):
        require(rules.get(key) is True, f"candidate rule {key} must be true")

    p1 = load(P1)
    assets = p1.get("assets", [])
    require(len(assets) == 16, "P1 registry must contain exactly 16 targets")
    require(all(a.get("authoredPresent") is False for a in assets), "N1 source branch must remain fail-closed")

    subprocess.run([sys.executable, str(ASSESSOR), "--self-test"], check=True, cwd=ROOT)

    workflow = WORKFLOW.read_text(encoding="utf-8")
    for token in (
        "self-hosted",
        "Windows",
        "unreal",
        "generate-p2-rainforest-source.py",
        "export-p2-rainforest-fbx.py",
        "import-p2-rainforest-assets.py",
        "generate-p3-character-source.py",
        "build-p3-character-fbx.py",
        "import-p3-character-assets.py",
        "generate-p4-authored-motion-vfx-presentation.py",
        "export-p4-animation-fbx.py",
        "import-p4-animation-presentation.py",
        "collect-p5-native-inventory.py",
        "assess-n1-native-integration.py",
        "--require-candidate",
        "upload-artifact",
    ):
        require(token in workflow, f"N1 workflow missing {token}")

    repo_quality = REPO_QUALITY.read_text(encoding="utf-8")
    require("Validate N1 Native Authored Integration Candidate" in repo_quality, "Repository Quality does not execute N1 gate")

    assessor_text = ASSESSOR.read_text(encoding="utf-8")
    require("ACTIVATION_CANDIDATE" in assessor_text and "BLOCKED" in assessor_text, "assessor status model incomplete")
    require("authoredPresent\"] = True" in assessor_text, "candidate generation is missing")
    require("update_file" not in assessor_text and "git commit" not in assessor_text.lower(), "assessor must not mutate repository state")

    doc = DOC.read_text(encoding="utf-8").replace("`", "").lower()
    for phrase in (
        "activation_candidate",
        "all-or-nothing",
        "human review",
        "does not mutate",
        "native runner",
        "n2",
    ):
        require(phrase in doc, f"N1 documentation missing: {phrase}")

    print("N1 native integration validated: all-or-nothing candidate generation, same-build native inventory and fail-closed registry behavior verified.")


if __name__ == "__main__":
    main()
