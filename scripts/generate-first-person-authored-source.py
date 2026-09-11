#!/usr/bin/env python3
"""Generate deterministic DCC-neutral source art for the World Makers first-person kit."""
from __future__ import annotations

import argparse
import hashlib
import json
import math
from pathlib import Path

BONES = [
    {"id": "root", "parent": None, "headCm": [0, 0, 0]},
    {"id": "upperarm_l", "parent": "root", "headCm": [0, -9, -4]},
    {"id": "lowerarm_l", "parent": "upperarm_l", "headCm": [18, -14, -10]},
    {"id": "hand_l", "parent": "lowerarm_l", "headCm": [36, -16, -14]},
    {"id": "upperarm_r", "parent": "root", "headCm": [0, 9, -4]},
    {"id": "lowerarm_r", "parent": "upperarm_r", "headCm": [18, 14, -10]},
    {"id": "hand_r", "parent": "lowerarm_r", "headCm": [36, 16, -14]},
]

DURATIONS = {
    "tool-raise": 0.42,
    "tool-lower": 0.36,
    "scan-anticipate": 0.28,
    "scan-hold": 1.20,
    "scan-settle": 0.34,
    "build-point": 0.48,
    "build-confirm": 0.36,
    "measure-focus": 0.60,
    "observe-focus": 0.52,
}

AMPLITUDES = {
    "tool-raise": (2, -4, 0),
    "tool-lower": (-2, 5, 0),
    "scan-anticipate": (1, -3, 2),
    "scan-hold": (0, -2, 1),
    "scan-settle": (-1, 2, 0),
    "build-point": (3, -5, -2),
    "build-confirm": (2, -7, 2),
    "measure-focus": (2, -3, 0),
    "observe-focus": (-1, 2, 0),
}


def norm(v):
    length = math.sqrt(sum(x * x for x in v)) or 1.0
    return tuple(x / length for x in v)


def basis(axis):
    ax = norm(axis)
    helper = (0, 0, 1) if abs(ax[2]) < 0.9 else (0, 1, 0)
    u = norm((
        ax[1] * helper[2] - ax[2] * helper[1],
        ax[2] * helper[0] - ax[0] * helper[2],
        ax[0] * helper[1] - ax[1] * helper[0],
    ))
    v = (
        ax[1] * u[2] - ax[2] * u[1],
        ax[2] * u[0] - ax[0] * u[2],
        ax[0] * u[1] - ax[1] * u[0],
    )
    return ax, u, v


def mesh(name, material, skinned=False):
    return {
        "name": name,
        "material": material,
        "skinned": skinned,
        "vertices": [],
        "normals": [],
        "uv0": [],
        "triangles": [],
        "weights": [],
    }


def append_mesh(dst, src):
    offset = len(dst["vertices"])
    for key in ("vertices", "normals", "uv0", "weights"):
        dst[key].extend(src[key])
    dst["triangles"].extend([[offset + a, offset + b, offset + c] for a, b, c in src["triangles"]])
    return dst


def tube(name, p0, p1, r0, r1, segments, material, bone0=None, bone1=None, skinned=False):
    out = mesh(name, material, skinned)
    _, u, v = basis(tuple(p1[i] - p0[i] for i in range(3)))
    rings = (
        (p0, r0, 0.0),
        (tuple((p0[i] + p1[i]) * 0.5 for i in range(3)), (r0 + r1) * 0.5, 0.5),
        (p1, r1, 1.0),
    )
    for point, radius, t in rings:
        for i in range(segments):
            angle = 2.0 * math.pi * i / segments
            radial = tuple(u[j] * math.cos(angle) + v[j] * math.sin(angle) for j in range(3))
            q = tuple(point[j] + radius * radial[j] for j in range(3))
            out["vertices"].append([round(x, 4) for x in q])
            out["normals"].append([round(x, 5) for x in radial])
            out["uv0"].append([round(i / segments, 5), t])
            if skinned:
                if bone1 and 0.0 < t < 1.0:
                    out["weights"].append([[bone0, 0.5], [bone1, 0.5]])
                else:
                    out["weights"].append([[bone0 if t < 0.5 or not bone1 else bone1, 1.0]])
            else:
                out["weights"].append([])
    for ring in range(2):
        base = ring * segments
        nxt = (ring + 1) * segments
        for i in range(segments):
            j = (i + 1) % segments
            out["triangles"].extend([[base + i, base + j, nxt + j], [base + i, nxt + j, nxt + i]])
    return out


def box(name, center, half, material):
    cx, cy, cz = center
    hx, hy, hz = half
    points = [(cx + sx * hx, cy + sy * hy, cz + sz * hz) for sx, sy, sz in (
        (-1, -1, -1), (1, -1, -1), (1, 1, -1), (-1, 1, -1),
        (-1, -1, 1), (1, -1, 1), (1, 1, 1), (-1, 1, 1),
    )]
    faces = [(0, 1, 2, 3), (4, 7, 6, 5), (0, 4, 5, 1), (1, 5, 6, 2), (2, 6, 7, 3), (4, 0, 3, 7)]
    out = mesh(name, material)
    for face in faces:
        a, b, c, d = [points[k] for k in face]
        n = norm((
            (b[1] - a[1]) * (c[2] - a[2]) - (b[2] - a[2]) * (c[1] - a[1]),
            (b[2] - a[2]) * (c[0] - a[0]) - (b[0] - a[0]) * (c[2] - a[2]),
            (b[0] - a[0]) * (c[1] - a[1]) - (b[1] - a[1]) * (c[0] - a[0]),
        ))
        base = len(out["vertices"])
        for uv, q in zip(((0, 0), (1, 0), (1, 1), (0, 1)), (a, b, c, d)):
            out["vertices"].append([round(x, 4) for x in q])
            out["normals"].append([round(x, 5) for x in n])
            out["uv0"].append(list(uv))
            out["weights"].append([])
        out["triangles"].extend([[base, base + 1, base + 2], [base, base + 2, base + 3]])
    return out


def ellipsoid(name, center, radii, segments, rings, material, bone):
    out = mesh(name, material, True)
    cx, cy, cz = center
    rx, ry, rz = radii
    for j in range(rings + 1):
        phi = math.pi * j / rings
        for i in range(segments):
            angle = 2.0 * math.pi * i / segments
            x = cx + rx * math.sin(phi) * math.cos(angle)
            y = cy + ry * math.sin(phi) * math.sin(angle)
            z = cz + rz * math.cos(phi)
            out["vertices"].append([round(x, 4), round(y, 4), round(z, 4)])
            n = norm(((x - cx) / (rx * rx), (y - cy) / (ry * ry), (z - cz) / (rz * rz)))
            out["normals"].append([round(x, 5) for x in n])
            out["uv0"].append([round(i / segments, 5), round(j / rings, 5)])
            out["weights"].append([[bone, 1.0]])
    for j in range(rings):
        for i in range(segments):
            k = (i + 1) % segments
            a = j * segments + i
            b = j * segments + k
            c = (j + 1) * segments + k
            d = (j + 1) * segments + i
            out["triangles"].extend([[a, b, c], [a, c, d]])
    return out


def arms(segments):
    out = mesh("first-person-arms", "WM_FP_Skin", True)
    for side, sign in (("l", -1), ("r", 1)):
        upper = f"upperarm_{side}"
        lower = f"lowerarm_{side}"
        hand = f"hand_{side}"
        p0, p1, p2 = (0, 9 * sign, -4), (18, 14 * sign, -10), (36, 16 * sign, -14)
        out = append_mesh(out, tube(f"upper_{side}", p0, p1, 4.8, 4.2, segments, "WM_FP_Skin", upper, lower, True))
        out = append_mesh(out, tube(f"lower_{side}", p1, p2, 4.2, 3.5, segments, "WM_FP_Skin", lower, hand, True))
        out = append_mesh(out, ellipsoid(f"hand_{side}", (41, 16.5 * sign, -14.5), (7.5, 4.8, 3.6), max(5, segments), max(3, segments // 2), "WM_FP_Skin", hand))
    return out


def scanner(segments):
    out = mesh("scanner", "WM_FP_Tool")
    for part in (
        box("body", (0, 0, 0), (7, 3.5, 2.5), "WM_FP_Tool"),
        tube("sensor", (7, 0, 0), (13, 0, 0), 3.0, 2.1, segments, "WM_FP_Emissive"),
        box("grip", (-2, 0, -5), (2.3, 2.4, 4.8), "WM_FP_Tool"),
    ):
        out = append_mesh(out, part)
    return out


def build_tool(segments):
    out = mesh("build-tool", "WM_FP_Tool")
    for part in (
        box("body", (0, 0, 0), (6.5, 3.5, 2.6), "WM_FP_Tool"),
        box("grip", (-2, 0, -5), (2.2, 2.2, 4.5), "WM_FP_Tool"),
        tube("emitter_a", (6.5, -2.2, 0), (13, -2.2, 0), 1.2, 0.8, max(4, segments - 2), "WM_FP_Emissive"),
        tube("emitter_b", (6.5, 2.2, 0), (13, 2.2, 0), 1.2, 0.8, max(4, segments - 2), "WM_FP_Emissive"),
    ):
        out = append_mesh(out, part)
    return out


def measure_tool(segments):
    out = mesh("measure-tool", "WM_FP_Tool")
    for part in (
        box("body", (0, 0, 0), (7, 2.6, 2.0), "WM_FP_Tool"),
        tube("laser", (7, 0, 0), (12, 0, 0), 1.7, 1.1, max(4, segments - 2), "WM_FP_Emissive"),
        box("screen", (-1, -3.1, 1.3), (2.8, 0.5, 1.5), "WM_FP_Screen"),
    ):
        out = append_mesh(out, part)
    return out


def wrist_device(segments):
    out = mesh("wrist-device", "WM_FP_Tool")
    for part in (
        tube("cuff", (-2, 0, 0), (2, 0, 0), 4.0, 4.0, max(5, segments), "WM_FP_Tool"),
        box("screen", (1.5, 0, 3.7), (3.2, 2.6, 0.6), "WM_FP_Screen"),
    ):
        out = append_mesh(out, part)
    return out


def clip(action, duration):
    frames = round(duration * 30)
    ax, ay, az = AMPLITUDES[action]
    keys = []
    for frame, scale in ((0, 0), (frames // 2, 1), (frames, 0)):
        keys.append({
            "frame": frame,
            "bones": {
                "root": {"locationCm": [0, 0, 0], "rotationDeg": [0, 0, 0]},
                "hand_l": {
                    "locationCm": [round(ax * 0.3 * scale, 3), round(-0.8 * scale, 3), round(az * 0.2 * scale, 3)],
                    "rotationDeg": [round(ay * 0.4 * scale, 3), 0, round(-2 * scale, 3)],
                },
                "hand_r": {
                    "locationCm": [round(ax * 0.55 * scale, 3), round(0.6 * scale, 3), round(az * 0.3 * scale, 3)],
                    "rotationDeg": [round(ay * 0.7 * scale, 3), round(1.5 * scale, 3), round(2 * scale, 3)],
                },
            },
        })
    return {"actionId": action, "fps": 30, "durationSeconds": duration, "frameCount": frames + 1, "loop": action == "scan-hold", "keys": keys}


def generate():
    return {
        "schemaVersion": 1,
        "bundleId": "visual.first-person-authored-source.v1",
        "units": "centimeters",
        "axes": {"forward": "+X", "right": "+Y", "up": "+Z"},
        "materials": {
            "WM_FP_Skin": {"baseColor": [0.66, 0.40, 0.27], "roughness": 0.58},
            "WM_FP_Tool": {"baseColor": [0.08, 0.22, 0.31], "roughness": 0.42},
            "WM_FP_Emissive": {"baseColor": [0.12, 0.68, 0.88], "emissive": 1.4, "roughness": 0.30},
            "WM_FP_Screen": {"baseColor": [0.05, 0.16, 0.22], "emissive": 0.9, "roughness": 0.22},
        },
        "skeleton": {"id": "SKEL_WM_FirstPersonArms", "bones": BONES},
        "assets": {
            "arms.firstperson": {"assetType": "skeletal-mesh", "lods": [arms(8), arms(6), arms(4)]},
            "tool.scanner": {"assetType": "static-mesh", "lods": [scanner(8), scanner(6), scanner(4)]},
            "tool.build": {"assetType": "static-mesh", "lods": [build_tool(8), build_tool(6), build_tool(4)]},
            "tool.measure": {"assetType": "static-mesh", "lods": [measure_tool(8), measure_tool(6), measure_tool(4)]},
            "device.wrist": {"assetType": "static-mesh", "lods": [wrist_device(8), wrist_device(6), wrist_device(5)]},
        },
        "animations": [clip(action, duration) for action, duration in DURATIONS.items()],
        "productionRules": {
            "maxSkinInfluences": 2,
            "maxMaterialSlots": 3,
            "collision": "none",
            "castsShadow": False,
            "ownerOnly": True,
            "rootMotion": False,
        },
    }


def serialize(data):
    return json.dumps(data, sort_keys=True, separators=(",", ":")) + "\n"


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--output", required=True)
    parser.add_argument("--print-sha", action="store_true")
    args = parser.parse_args()
    text = serialize(generate())
    path = Path(args.output)
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(text, encoding="utf-8")
    if args.print_sha:
        print(hashlib.sha256(text.encode("utf-8")).hexdigest())


if __name__ == "__main__":
    main()
