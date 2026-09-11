#!/usr/bin/env python3
"""Validate P4 authored animation, VFX and presentation source contracts."""
from __future__ import annotations

import hashlib
import json
import math
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
MANIFEST = ROOT / "content/visual/authored/p4-motion-vfx-presentation.json"
GENERATOR = ROOT / "scripts/generate-p4-authored-motion-vfx-presentation.py"
V4 = ROOT / "content/visual/character/avatar-rig-v4.json"
V5 = ROOT / "content/visual/character/character-animation-v5.json"
V6 = ROOT / "content/visual/vfx/vfx-feedback-v6.json"
V7 = ROOT / "content/visual/presentation/presentation-camera-ui-v7.json"
P1 = ROOT / "content/visual/authored/authored-assets-p1.json"
P3 = ROOT / "content/visual/authored/character-p3-source-pack.json"
WORKFLOW = ROOT / ".github/workflows/repo-quality.yml"
NATIVE_WORKFLOW = ROOT / ".github/workflows/p4-authored-native-handoff.yml"
BLENDER = ROOT / "scripts/blender/export-p4-animation-fbx.py"
UNREAL = ROOT / "scripts/unreal/import-p4-animation-presentation.py"
DOC = ROOT / "docs/p4-authored-animation-vfx-presentation.md"


def fail(message: str) -> None:
    raise SystemExit("P4 validation failed: " + message)


def require(condition: bool, message: str) -> None:
    if not condition:
        fail(message)


def load(path: Path):
    return json.loads(path.read_text(encoding="utf-8"))


def close(a, b, eps=1e-6):
    return abs(float(a) - float(b)) <= eps


def main() -> None:
    required = (MANIFEST, GENERATOR, V4, V5, V6, V7, P1, P3, WORKFLOW, NATIVE_WORKFLOW, BLENDER, UNREAL, DOC)
    missing = [str(p.relative_to(ROOT)) for p in required if not p.is_file()]
    require(not missing, f"missing required files: {missing}")

    manifest = load(MANIFEST)
    require(manifest.get("schemaVersion") == 1 and manifest.get("phase") == "P4", "invalid P4 manifest identity")
    expected_hash = manifest.get("source", {}).get("sha256", "")
    require(len(expected_hash) == 64, "P4 source hash missing")

    with tempfile.TemporaryDirectory(prefix="wm-p4-") as tmp:
        a = Path(tmp) / "a.json"
        b = Path(tmp) / "b.json"
        subprocess.run([sys.executable, str(GENERATOR), "--output", str(a)], check=True, cwd=ROOT)
        subprocess.run([sys.executable, str(GENERATOR), "--output", str(b)], check=True, cwd=ROOT)
        raw_a, raw_b = a.read_bytes(), b.read_bytes()
        require(raw_a == raw_b, "P4 generator is not deterministic")
        require(hashlib.sha256(raw_a).hexdigest() == expected_hash, "P4 generated source hash differs from manifest")
        source = json.loads(raw_a)

    v4, v5, v6, v7, p1, p3 = map(load, (V4, V5, V6, V7, P1, P3))
    joint_ids = {j["id"] for j in v4["joints"]}
    require(len(joint_ids) == 19, "P4 requires the exact V4 19-joint rig")
    require(p3["skeleton"]["jointCount"] == 19 and p3["runtime"]["rootMotionDefault"] is False, "P3 skeleton/root-motion contract changed")

    expected_clip_ids = set(v5["locomotionStates"]) | {a["id"] for a in v5["interactionActions"]}
    clips = source["animation"]["clips"]
    require(len(clips) == manifest["animation"]["clipCount"] == 17, "P4 must author exactly 17 initial clips")
    require({c["id"] for c in clips} == expected_clip_ids == set(manifest["animation"]["requiredClipIds"]), "P4 clips drifted from V5 semantics")
    v5_actions = {a["id"]: a for a in v5["interactionActions"]}
    for clip in clips:
        require(clip["fps"] == 30 and clip["rootMotion"] is False, f"{clip['id']} must remain 30 fps and in-place")
        duration = float(clip["durationSeconds"])
        require(math.isfinite(duration) and 0.15 <= duration <= 2.0, f"invalid clip duration: {clip['id']}")
        if clip["id"] in v5_actions:
            require(close(duration, v5_actions[clip["id"]]["defaultDurationSeconds"]), f"interaction duration drift: {clip['id']}")
            require(clip["layer"] == v5_actions[clip["id"]]["layer"], f"interaction layer drift: {clip['id']}")
        require(isinstance(clip["tracks"], dict) and clip["tracks"], f"clip has no authored tracks: {clip['id']}")
        for bone, samples in clip["tracks"].items():
            require(bone in joint_ids, f"{clip['id']} references unknown joint {bone}")
            require(samples and all(isinstance(s, dict) for s in samples), f"empty track {clip['id']}:{bone}")
            times = [float(s["t"]) for s in samples]
            require(times == sorted(times) and times[0] >= 0 and times[-1] <= duration + 1e-6, f"invalid key times: {clip['id']}:{bone}")
            for sample in samples:
                if "r" in sample:
                    require(len(sample["r"]) == 3 and all(math.isfinite(float(v)) and abs(float(v)) <= 75 for v in sample["r"]), f"unsafe rotation bound: {clip['id']}:{bone}")
                if "p" in sample:
                    require(len(sample["p"]) == 3 and all(math.isfinite(float(v)) and abs(float(v)) <= 8 for v in sample["p"]), f"unsafe translation bound: {clip['id']}:{bone}")
                    if bone == "root":
                        require(all(close(v, 0) for v in sample["p"]), f"root translation forbidden: {clip['id']}")
    require(source["animation"]["rootMotionDefault"] is False, "P4 cannot enable root motion")
    require(source["animation"]["animationBlueprintPlan"]["readModel"] == v5["animBPReadModel"], "AnimBP plan must consume the V5 read-model exactly")
    require(source["animation"]["animationBlueprintPlan"]["movementAuthority"] == "CharacterMovementComponent", "animation cannot own movement")

    v6_by_id = {e["id"]: e for e in v6["effects"]}
    effects = source["vfx"]["effects"]
    require(len(effects) == manifest["vfx"]["effectCount"] == 17, "P4 must cover all 17 V6 effects")
    require({e["id"] for e in effects} == set(v6_by_id) == set(manifest["vfx"]["requiredEffectIds"]), "P4 VFX IDs drifted from V6")
    tier_limits = [v6["qualityTiers"][k]["maxNiagaraParticlesPerEffect"] for k in ("low", "mid", "high")]
    for effect in effects:
        upstream = v6_by_id[effect["id"]]
        require(effect["domain"] == upstream["domain"] and effect["shape"] == upstream["shape"], f"VFX semantic drift: {effect['id']}")
        expected_path = upstream["niagaraTarget"] + "." + upstream["niagaraTarget"].rsplit("/", 1)[-1]
        require(effect["objectPath"] == expected_path, f"VFX target drift: {effect['id']}")
        recipe = effect["recipe"]
        require(len(recipe["spawn"]) == 3 and all(int(recipe["spawn"][i]) <= int(tier_limits[i]) for i in range(3)), f"VFX particle budget exceeded: {effect['id']}")
        require(0 < float(recipe["reducedMotionScale"]) <= 0.35, f"reduced-motion bound invalid: {effect['id']}")
        require(recipe["collisionEnabled"] is False and recipe["lightRendererEnabled"] is False, f"tablet-safe VFX contract broken: {effect['id']}")
    require(source["vfx"]["presentationOnly"] is True and v6["gameplayAuthority"] is False and v6["evidenceAuthority"] is False, "VFX cannot gain gameplay/evidence authority")

    presentation = source["presentation"]
    v7_modes = {m["id"]: m for m in v7["cameraModes"]}
    require({m["id"] for m in presentation["cameraModes"]} == set(v7_modes), "P4 camera modes drifted from V7")
    for mode in presentation["cameraModes"]:
        require(mode == v7_modes[mode["id"]], f"camera profile drift: {mode['id']}")
    require(presentation["reducedMotion"] == v7["reducedMotion"], "P4 reduced-motion camera contract drifted")
    require({u["id"] for u in presentation["uiMotion"]} == set(v7["uiCues"]), "P4 UI cue set drifted from V7")
    require(len(presentation["sequences"]) == manifest["presentation"]["sequenceCount"] == 2, "P4 requires two initial reveal timelines")
    for sequence in presentation["sequences"]:
        require(float(sequence["durationSeconds"]) <= float(v7["cinematicBeat"]["maxDurationSeconds"]), f"sequence too long: {sequence['id']}")
        require(sequence["locksInput"] is False and sequence["forcesViewTarget"] is False and sequence["changesGameplayTimeScale"] is False, f"intrusive cinematic behavior forbidden: {sequence['id']}")
        phases = sequence["phases"]
        require([p["name"] for p in phases] == ["enter", "hold", "exit"], f"invalid microbeat phases: {sequence['id']}")
        require(close(phases[0]["start"], 0) and close(phases[-1]["end"], sequence["durationSeconds"]), f"sequence range mismatch: {sequence['id']}")

    boundary = source["productionBoundary"]
    require(boundary == {
        "sourceAnimationCurvesPresent": True,
        "sourceVfxRecipesPresent": True,
        "sourcePresentationTimelinesPresent": True,
        "nativeAnimationAssetsPresent": False,
        "nativeNiagaraAssetsPresent": False,
        "nativeLevelSequencesPresent": False,
        "nativeImportRequired": True,
        "humanReviewRequired": True,
        "deviceReviewRequired": True,
    }, "P4 production boundary must remain fail-closed")

    p1_by_id = {a["id"]: a for a in p1["assets"]}
    for asset_id in ("character.player.child-explorer", "animation.player.blueprint", "vfx.science.master", "presentation.adventure-reveal", "presentation.camera-data"):
        require(asset_id in p1_by_id and p1_by_id[asset_id]["authoredPresent"] is False, f"P4 may not auto-activate {asset_id}")

    workflow = WORKFLOW.read_text(encoding="utf-8")
    require("Validate P4 Authored Animation VFX Presentation" in workflow, "Repository Quality does not execute P4 gate")
    native = NATIVE_WORKFLOW.read_text(encoding="utf-8")
    for token in ("self-hosted", "Blender", "UnrealEditor-Cmd.exe", "generate-p4-authored-motion-vfx-presentation.py", "export-p4-animation-fbx.py", "import-p4-animation-presentation.py"):
        require(token in native, f"P4 native workflow missing {token}")

    unreal_text = UNREAL.read_text(encoding="utf-8")
    require("authored-assets-p1.json" not in unreal_text, "P4 Unreal bridge may not edit the P1 authored manifest")
    for forbidden in ('"authoredPresent": true', "authoredPresent = True", "authoredPresent=True"):
        require(forbidden not in unreal_text, "P4 Unreal bridge may not activate authored assets")
    require('"authoredPresentMutated": False' in unreal_text, "P4 native report must explicitly record no authoredPresent mutation")

    doc = DOC.read_text(encoding="utf-8").replace("`", "")
    for phrase in ("17 animation clips", "17 VFX recipes", "two microsequences", "procedural fallback", "P5"):
        require(phrase.lower() in doc.lower(), f"P4 documentation missing: {phrase}")

    print("P4 authored source validated: 17 clips, 17 causal VFX recipes, V7 camera/UI timelines, deterministic hash and fail-closed native handoff.")


if __name__ == "__main__":
    main()
