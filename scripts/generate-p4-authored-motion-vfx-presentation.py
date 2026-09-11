#!/usr/bin/env python3
"""Generate the deterministic P4 animation/VFX/presentation source bundle."""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path

EXPECTED_SHA256 = "9193bbeb85a5f14e74d69df92ecae9a6e8414304a4337b1ee4912f71439e0ed3"


def key(t, r=None, p=None):
    out = {"t": round(t, 4)}
    if r is not None:
        out["r"] = [round(x, 3) for x in r]
    if p is not None:
        out["p"] = [round(x, 3) for x in p]
    return out


def clip(cid, name, duration, loop, layer, tracks):
    return {
        "id": cid,
        "assetName": name,
        "durationSeconds": duration,
        "fps": 30,
        "loop": loop,
        "layer": layer,
        "rootMotion": False,
        "tracks": tracks,
    }


def build_clips():
    clips = [
        clip("locomotion.idle", "A_WM_Child_Idle", 1.6, True, "full-body", {
            "pelvis": [key(0, p=[0,0,0]), key(.8, p=[0,0,1.2]), key(1.6, p=[0,0,0])],
            "chest": [key(0, r=[0,0,0]), key(.8, r=[-2,0,1]), key(1.6, r=[0,0,0])],
            "head": [key(0, r=[0,0,0]), key(.8, r=[1,0,-1]), key(1.6, r=[0,0,0])],
        }),
        clip("locomotion.start", "A_WM_Child_Start", .32, False, "full-body", {
            "root": [key(0, r=[0,0,0]), key(.32, r=[6,0,0])],
            "thigh_l": [key(0, r=[0,0,0]), key(.32, r=[-18,0,0])],
            "thigh_r": [key(0, r=[0,0,0]), key(.32, r=[18,0,0])],
            "upperarm_l": [key(0, r=[0,0,0]), key(.32, r=[20,0,0])],
            "upperarm_r": [key(0, r=[0,0,0]), key(.32, r=[-20,0,0])],
        }),
        clip("locomotion.walk", "A_WM_Child_Walk", .8, True, "full-body", {
            "pelvis": [key(0,p=[0,0,0]),key(.2,p=[0,0,1]),key(.4,p=[0,0,0]),key(.6,p=[0,0,1]),key(.8,p=[0,0,0])],
            "thigh_l": [key(0,r=[24,0,0]),key(.4,r=[-24,0,0]),key(.8,r=[24,0,0])],
            "thigh_r": [key(0,r=[-24,0,0]),key(.4,r=[24,0,0]),key(.8,r=[-24,0,0])],
            "calf_l": [key(0,r=[-8,0,0]),key(.2,r=[22,0,0]),key(.4,r=[4,0,0]),key(.8,r=[-8,0,0])],
            "calf_r": [key(0,r=[4,0,0]),key(.4,r=[-8,0,0]),key(.6,r=[22,0,0]),key(.8,r=[4,0,0])],
            "upperarm_l": [key(0,r=[-20,0,0]),key(.4,r=[20,0,0]),key(.8,r=[-20,0,0])],
            "upperarm_r": [key(0,r=[20,0,0]),key(.4,r=[-20,0,0]),key(.8,r=[20,0,0])],
        }),
        clip("locomotion.run", "A_WM_Child_Run", .56, True, "full-body", {
            "root": [key(0,r=[8,0,0]),key(.56,r=[8,0,0])],
            "pelvis": [key(0,p=[0,0,0]),key(.14,p=[0,0,2]),key(.28,p=[0,0,0]),key(.42,p=[0,0,2]),key(.56,p=[0,0,0])],
            "thigh_l": [key(0,r=[38,0,0]),key(.28,r=[-38,0,0]),key(.56,r=[38,0,0])],
            "thigh_r": [key(0,r=[-38,0,0]),key(.28,r=[38,0,0]),key(.56,r=[-38,0,0])],
            "calf_l": [key(0,r=[-5,0,0]),key(.14,r=[35,0,0]),key(.28,r=[5,0,0]),key(.56,r=[-5,0,0])],
            "calf_r": [key(0,r=[5,0,0]),key(.28,r=[-5,0,0]),key(.42,r=[35,0,0]),key(.56,r=[5,0,0])],
            "upperarm_l": [key(0,r=[-34,0,0]),key(.28,r=[34,0,0]),key(.56,r=[-34,0,0])],
            "upperarm_r": [key(0,r=[34,0,0]),key(.28,r=[-34,0,0]),key(.56,r=[34,0,0])],
        }),
        clip("locomotion.stop", "A_WM_Child_Stop", .30, False, "full-body", {
            "root": [key(0,r=[6,0,0]),key(.3,r=[0,0,0])],
            "pelvis": [key(0,p=[0,0,1]),key(.18,p=[0,0,-1.2]),key(.3,p=[0,0,0])],
            "thigh_l": [key(0,r=[-18,0,0]),key(.3,r=[0,0,0])],
            "thigh_r": [key(0,r=[18,0,0]),key(.3,r=[0,0,0])],
        }),
        clip("locomotion.turn-in-place", "A_WM_Child_Turn", .44, False, "full-body", {
            "pelvis": [key(0,r=[0,0,0]),key(.22,r=[0,18,0]),key(.44,r=[0,32,0])],
            "chest": [key(0,r=[0,0,0]),key(.22,r=[0,12,0]),key(.44,r=[0,20,0])],
            "head": [key(0,r=[0,0,0]),key(.22,r=[0,14,0]),key(.44,r=[0,8,0])],
            "foot_l": [key(0,r=[0,0,0]),key(.22,r=[0,-8,0]),key(.44,r=[0,0,0])],
        }),
        clip("locomotion.jump", "A_WM_Child_Jump", .34, False, "full-body", {
            "root": [key(0,r=[0,0,0]),key(.15,r=[-6,0,0]),key(.34,r=[6,0,0])],
            "pelvis": [key(0,p=[0,0,0]),key(.15,p=[0,0,-2]),key(.34,p=[0,0,2])],
            "thigh_l": [key(0,r=[0,0,0]),key(.34,r=[25,0,0])],
            "thigh_r": [key(0,r=[0,0,0]),key(.34,r=[20,0,0])],
            "calf_l": [key(0,r=[0,0,0]),key(.34,r=[-35,0,0])],
            "calf_r": [key(0,r=[0,0,0]),key(.34,r=[-30,0,0])],
        }),
        clip("locomotion.fall", "A_WM_Child_Fall", .50, True, "full-body", {
            "root": [key(0,r=[5,0,0]),key(.5,r=[9,0,0])],
            "upperarm_l": [key(0,r=[-18,0,-12]),key(.5,r=[-22,0,-15])],
            "upperarm_r": [key(0,r=[-18,0,12]),key(.5,r=[-22,0,15])],
            "thigh_l": [key(0,r=[18,0,0]),key(.5,r=[22,0,0])],
            "thigh_r": [key(0,r=[12,0,0]),key(.5,r=[16,0,0])],
        }),
        clip("locomotion.land", "A_WM_Child_Land", .32, False, "full-body", {
            "pelvis": [key(0,p=[0,0,0]),key(.10,p=[0,0,-4]),key(.22,p=[0,0,-1]),key(.32,p=[0,0,0])],
            "root": [key(0,r=[7,0,0]),key(.10,r=[14,0,0]),key(.32,r=[0,0,0])],
            "thigh_l": [key(0,r=[10,0,0]),key(.10,r=[32,0,0]),key(.32,r=[0,0,0])],
            "thigh_r": [key(0,r=[10,0,0]),key(.10,r=[32,0,0]),key(.32,r=[0,0,0])],
            "calf_l": [key(0,r=[-10,0,0]),key(.10,r=[-38,0,0]),key(.32,r=[0,0,0])],
            "calf_r": [key(0,r=[-10,0,0]),key(.10,r=[-38,0,0]),key(.32,r=[0,0,0])],
        }),
    ]
    interaction_specs = [
        ("interaction.build-place","A_WM_Child_BuildPlace",.36,"upper-body",[(0,[0,0,0]),(.18,[-35,-8,0]),(.36,[0,0,0])]),
        ("interaction.build-remove","A_WM_Child_BuildRemove",.48,"upper-body",[(0,[0,0,0]),(.24,[32,10,0]),(.48,[0,0,0])]),
        ("interaction.build-move","A_WM_Child_BuildMove",.55,"upper-body",[(0,[0,0,0]),(.28,[-24,0,10]),(.55,[0,0,0])]),
        ("interaction.measure","A_WM_Child_Measure",.60,"upper-body",[(0,[0,0,0]),(.30,[-18,-20,5]),(.60,[0,0,0])]),
        ("interaction.observe","A_WM_Child_Observe",.52,"upper-body-look",[(0,[0,0,0]),(.26,[-12,-8,14]),(.52,[0,0,0])]),
        ("interaction.inspect","A_WM_Child_Inspect",.62,"upper-body-look",[(0,[0,0,0]),(.31,[-28,-14,8]),(.62,[0,0,0])]),
        ("interaction.pickup","A_WM_Child_Pickup",.55,"upper-body",[(0,[0,0,0]),(.28,[-42,0,12]),(.55,[0,0,0])]),
        ("interaction.science-manipulate","A_WM_Child_ScienceManipulate",.72,"upper-body-precision",[(0,[0,0,0]),(.36,[-22,-16,6]),(.72,[0,0,0])]),
    ]
    for cid, name, duration, layer, samples in interaction_specs:
        tracks = {
            "upperarm_r": [key(t, r=r) for t, r in samples],
            "lowerarm_r": [key(t, r=[r[0]*.65,r[1]*.4,r[2]*.7]) for t, r in samples],
            "hand_r": [key(t, r=[r[0]*.25,r[1]*.2,r[2]*.8]) for t, r in samples],
            "chest": [key(0,r=[0,0,0]),key(duration/2,r=[-4,0,0]),key(duration,r=[0,0,0])],
        }
        if "look" in layer or "precision" in layer:
            tracks["head"] = [key(0,r=[0,0,0]),key(duration/2,r=[8,-6,0]),key(duration,r=[0,0,0])]
        clips.append(clip(cid, name, duration, False, layer, tracks))
    return clips


def build_vfx():
    effects = [
        ("gameplay.build.place","gameplay","ring","NS_WM_Build_Place"),
        ("gameplay.build.remove","gameplay","burst","NS_WM_Build_Remove"),
        ("gameplay.build.move","gameplay","directional","NS_WM_Build_Move"),
        ("mission.measure.reveal","gameplay","ring","NS_WM_Measure_Reveal"),
        ("world.observe.reveal","gameplay","halo","NS_WM_Observe_Reveal"),
        ("science.chemistry.dissolution","chemistry","halo","NS_WM_Chem_Dissolution"),
        ("science.chemistry.saturation","chemistry","ring","NS_WM_Chem_Saturation"),
        ("science.chemistry.filtration","chemistry","directional","NS_WM_Chem_Filtration"),
        ("science.chemistry.reaction","chemistry","burst","NS_WM_Chem_Reaction"),
        ("science.physics.force","physics","directional","NS_WM_Physics_Force"),
        ("science.physics.circuit-flow","physics","ring","NS_WM_Physics_CircuitFlow"),
        ("science.biology.cell-energy","biology","halo","NS_WM_Bio_CellEnergy"),
        ("science.biology.plant-growth","biology","ring","NS_WM_Bio_PlantGrowth"),
        ("science.ecology.recovery","ecology","halo","NS_WM_Ecology_Recovery"),
        ("science.ecology.stress","ecology","burst","NS_WM_Ecology_Stress"),
        ("fantasy.portal.open","fantasy","halo","NS_WM_Fantasy_Portal"),
        ("fantasy.rune.activate","fantasy","burst","NS_WM_Fantasy_Rune"),
    ]
    shapes = {
        "ring": {"renderer":"sprite-ring","spawn":[24,48,80],"lifetime":.72,"speed":32,"scale":[.75,1.0],"motion":"radial"},
        "halo": {"renderer":"sprite-orbit","spawn":[18,42,72],"lifetime":.95,"speed":18,"scale":[.8,1.15],"motion":"orbital"},
        "burst": {"renderer":"sprite-burst","spawn":[28,64,120],"lifetime":.58,"speed":95,"scale":[.55,.95],"motion":"outward"},
        "directional": {"renderer":"ribbon-chevron","spawn":[12,28,52],"lifetime":.66,"speed":72,"scale":[.65,1.0],"motion":"vector"},
    }
    out = []
    for eid, domain, shape, name in effects:
        recipe = dict(shapes[shape])
        recipe.update({
            "colorRole": f"{domain}.accent",
            "intensityParameter": "User.Intensity",
            "directionParameter": "User.Direction",
            "reducedMotionScale": .35,
            "collisionEnabled": False,
            "lightRendererEnabled": False,
        })
        out.append({
            "id": eid,
            "domain": domain,
            "shape": shape,
            "assetName": name,
            "objectPath": f"/Game/WorldMakers/VFX/{name}.{name}",
            "recipe": recipe,
        })
    return out


def build_presentation():
    camera_modes = [
        {"id":"explore","armLengthCm":500,"fovDegrees":72,"offsetZCm":65,"blendSeconds":.34,"holdSeconds":0.0},
        {"id":"build","armLengthCm":580,"fovDegrees":77,"offsetZCm":78,"blendSeconds":.32,"holdSeconds":.65},
        {"id":"observe","armLengthCm":405,"fovDegrees":65,"offsetZCm":72,"blendSeconds":.28,"holdSeconds":.85},
        {"id":"science","armLengthCm":365,"fovDegrees":62,"offsetZCm":70,"blendSeconds":.30,"holdSeconds":.90},
        {"id":"dialogue","armLengthCm":345,"fovDegrees":60,"offsetZCm":75,"blendSeconds":.30,"holdSeconds":1.20},
        {"id":"adventure-reveal","armLengthCm":650,"fovDegrees":69,"offsetZCm":105,"blendSeconds":.45,"holdSeconds":1.35},
    ]
    cues = [
        "presentation.build.confirm","presentation.observe.focus","presentation.science.focus",
        "presentation.adventure.reveal","presentation.mission.changed","presentation.ecology.changed",
    ]
    ui_motion = [{
        "id": cue,
        "durationSeconds": .48 if "reveal" in cue else .32,
        "opacity": [[0,0],[.18,1],[.82,1],[1,0]],
        "translationYpx": [[0,12],[1,0]],
        "scale": [[0,.98],[1,1.0]],
    } for cue in cues]
    return {
        "cameraModes": camera_modes,
        "reducedMotion": {
            "maxArmDeltaFromExploreCm":80,"maxFovDeltaFromExploreDegrees":3,
            "maxOffsetZDeltaFromExploreCm":16,"maxBlendSeconds":.18,
            "maxHoldSeconds":.70,"uiTranslationPx":0,"uiScale":1.0,
        },
        "uiMotion": ui_motion,
        "sequences": [
            {
                "id":"adventure-reveal","assetName":"LS_WM_AdventureReveal",
                "objectPath":"/Game/WorldMakers/Presentation/Sequences/LS_WM_AdventureReveal.LS_WM_AdventureReveal",
                "durationSeconds":2.35,
                "phases":[
                    {"name":"enter","start":0,"end":.45,"cameraMode":"adventure-reveal"},
                    {"name":"hold","start":.45,"end":1.70,"cameraMode":"adventure-reveal"},
                    {"name":"exit","start":1.70,"end":2.35,"cameraMode":"explore"},
                ],
                "locksInput":False,"forcesViewTarget":False,"changesGameplayTimeScale":False,
            },
            {
                "id":"science-reveal","assetName":"LS_WM_ScienceReveal",
                "objectPath":"/Game/WorldMakers/Presentation/Sequences/LS_WM_ScienceReveal.LS_WM_ScienceReveal",
                "durationSeconds":1.95,
                "phases":[
                    {"name":"enter","start":0,"end":.30,"cameraMode":"science"},
                    {"name":"hold","start":.30,"end":1.35,"cameraMode":"science"},
                    {"name":"exit","start":1.35,"end":1.95,"cameraMode":"explore"},
                ],
                "locksInput":False,"forcesViewTarget":False,"changesGameplayTimeScale":False,
            },
        ],
    }


def build_bundle():
    return {
        "schemaVersion": 1,
        "productionPhase": "P4",
        "animation": {
            "skeleton": "SKEL_WM_ChildExplorer",
            "fps": 30,
            "rootMotionDefault": False,
            "clips": build_clips(),
            "animationBlueprintPlan": {
                "assetName": "ABP_WM_ChildExplorer",
                "objectPath": "/Game/WorldMakers/Characters/Player/ABP_WM_ChildExplorer.ABP_WM_ChildExplorer",
                "readModel": [
                    "locomotionStateId","speedAlpha","isAirborne","stateAgeSeconds",
                    "aimYawDegrees","aimPitchDegrees","interactionActionId","interactionAlpha",
                ],
                "states": ["idle","start","walk","run","stop","turn-in-place","jump","fall","land"],
                "upperBodySlot": "UpperBody",
                "movementAuthority": "CharacterMovementComponent",
                "rootMotion": False,
            },
        },
        "vfx": {
            "presentationOnly": True,
            "effects": build_vfx(),
            "qualityTiers": {"low":{"maxParticles":48},"mid":{"maxParticles":96},"high":{"maxParticles":180}},
        },
        "presentation": build_presentation(),
        "productionBoundary": {
            "sourceAnimationCurvesPresent": True,
            "sourceVfxRecipesPresent": True,
            "sourcePresentationTimelinesPresent": True,
            "nativeAnimationAssetsPresent": False,
            "nativeNiagaraAssetsPresent": False,
            "nativeLevelSequencesPresent": False,
            "nativeImportRequired": True,
            "humanReviewRequired": True,
            "deviceReviewRequired": True,
        },
    }


def serialize(payload):
    return json.dumps(payload, indent=2, sort_keys=True, separators=(",", ": ")) + "\n"


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--output", required=True)
    parser.add_argument("--verify", action="store_true")
    args = parser.parse_args()
    raw = serialize(build_bundle())
    digest = hashlib.sha256(raw.encode("utf-8")).hexdigest()
    if args.verify and digest != EXPECTED_SHA256:
        raise SystemExit(f"P4 bundle hash mismatch: {digest} != {EXPECTED_SHA256}")
    out = Path(args.output)
    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_text(raw, encoding="utf-8")
    payload = json.loads(raw)
    print(f"P4 source bundle: {out} sha256={digest} clips={len(payload['animation']['clips'])} vfx={len(payload['vfx']['effects'])}")


if __name__ == "__main__":
    main()
