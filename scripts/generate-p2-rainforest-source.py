#!/usr/bin/env python3
"""Generate the P2 World Makers rainforest source-art pack as deterministic OBJ files.

The generated meshes are original low-poly source geometry, not Unreal .uasset files.
They use centimeters, +Z up and +X forward, and encode authored LOD groups, UV0,
normals and material assignments. P2 later converts them to FBX for the native UE import path.
"""
from __future__ import annotations

import argparse
import hashlib
import math
from pathlib import Path

MTL = """# World Makers P2 rainforest source materials\nnewmtl WM_Ground\nKd 0.28 0.22 0.12\nNs 8\nnewmtl WM_Terrain\nKd 0.33 0.26 0.15\nNs 10\nnewmtl WM_Bark\nKd 0.24 0.14 0.08\nNs 8\nnewmtl WM_Foliage\nKd 0.15 0.42 0.20\nNs 12\nnewmtl WM_Stone\nKd 0.32 0.34 0.31\nNs 14\nnewmtl WM_Water\nKd 0.12 0.42 0.55\nNs 30\n"""

EXPECTED_SHA256 = {
    "SM_WM_RF_Ground_A.obj": "b7722d6d2d2905115f5ce2393488375a18dc3113f09a2e6a30d764d0368318e2",
    "SM_WM_RF_TerrainMound_A.obj": "5906046b5ec7a817e37ce1022484d1202f5e34e6b4f5f731c38291a285d9f14c",
    "SM_WM_RF_Tree_A.obj": "99d90e32a894e29d4b9ea051d884f4eea76e0d6c52427f7bd45af7d0e9c90af9",
    "SM_WM_RF_Tree_B.obj": "14d212d4d88f20713a20060c835643072bfcb0d5749c060b6eb28bdbeaf3f34e",
    "SM_WM_RF_Tree_C.obj": "0369101816fa3afc6197830c48583a7a1b48b3d2f1bac8e205262ad70629fff2",
    "SM_WM_RF_Understory_A.obj": "de97758e9b6907481874df12d0351fd0b66389a29300bb8a302a03ed1ac09249",
    "SM_WM_RF_Rock_A.obj": "ec2f86e31e4b11c379169ac9f256676ddb82d43073119c46369f1efd97879b18",
    "SM_WM_RF_WaterEdge_A.obj": "093f4775b6caee766626ca61dd44208f8947342918a3faadf2656fd41b3f84c4",
    "SM_WM_RF_HeroCeiba.obj": "409f956bb9e244737da5c82c6e715278fd694d503509942df5b8b615b3b61457",
}


def ellipsoid(name, rx, ry, rz, seg, rings, mat, z0=0.0, cx=0.0, cy=0.0):
    v, n, uv, f = [], [], [], []
    for j in range(rings + 1):
        ph = math.pi * j / rings
        for i in range(seg):
            a = 2.0 * math.pi * i / seg
            x = cx + rx * math.sin(ph) * math.cos(a)
            y = cy + ry * math.sin(ph) * math.sin(a)
            z = z0 + rz * math.cos(ph)
            v.append((x, y, z))
            nx, ny, nz = (x-cx)/(rx*rx), (y-cy)/(ry*ry), (z-z0)/(rz*rz)
            length = math.sqrt(nx*nx + ny*ny + nz*nz) or 1.0
            n.append((nx/length, ny/length, nz/length))
            uv.append((i/seg, j/rings))
    for j in range(rings):
        for i in range(seg):
            ni = (i + 1) % seg
            a, b = j*seg+i, j*seg+ni
            c, d = (j+1)*seg+ni, (j+1)*seg+i
            f += [(a,b,c), (a,c,d)]
    return dict(name=name, mat=mat, v=v, n=n, uv=uv, f=f)


def cylinder(name, h, r0, r1, seg, mat, leanx=0.0, leany=0.0):
    v, n, uv, f = [], [], [], []
    for z, r, k in ((0.0, r0, 0.0), (h, r1, 1.0)):
        for i in range(seg):
            a = 2.0 * math.pi * i / seg
            v.append((r*math.cos(a)+leanx*k, r*math.sin(a)+leany*k, z))
            n.append((math.cos(a), math.sin(a), 0.0))
            uv.append((i/seg, k))
    for i in range(seg):
        j = (i + 1) % seg
        a, b, c, d = i, j, seg+j, seg+i
        f += [(a,b,c), (a,c,d)]
    return dict(name=name, mat=mat, v=v, n=n, uv=uv, f=f)


def disc(name, radius, seg, mat, zwarp=0.0):
    v, n, uv, f = [(0.0,0.0,0.0)], [(0.0,0.0,1.0)], [(0.5,0.5)], []
    for i in range(seg):
        a = 2.0 * math.pi * i / seg
        rr = radius * (1.0 + 0.05 * math.sin(i * 2.17))
        v.append((rr*math.cos(a), rr*math.sin(a), zwarp*math.sin(i*1.9)))
        n.append((0.0,0.0,1.0))
        uv.append((0.5+0.5*math.cos(a), 0.5+0.5*math.sin(a)))
    for i in range(seg):
        f.append((0, i+1, (i+1) % seg + 1))
    return dict(name=name, mat=mat, v=v, n=n, uv=uv, f=f)


def leaves(name, count, radius, mat):
    v, n, uv, f = [], [], [], []
    for k in range(count):
        a = 2.0 * math.pi * k / count
        cx, cy = radius*0.3*math.cos(a), radius*0.3*math.sin(a)
        z, w, h = (k % 2)*radius*0.1, radius*0.45, radius*0.9
        dx, dy, base = math.cos(a)*w/2.0, math.sin(a)*w/2.0, len(v)
        v += [(cx-dx,cy-dy,z),(cx+dx,cy+dy,z),(cx+dx,cy+dy,z+h),(cx-dx,cy-dy,z+h)]
        normal = (math.sin(a), -math.cos(a), 0.0)
        n += [normal] * 4
        uv += [(0,0),(1,0),(1,1),(0,1)]
        f += [(base,base+1,base+2),(base,base+2,base+3)]
    return dict(name=name, mat=mat, v=v, n=n, uv=uv, f=f)


def merge(name, mat, parts):
    v, n, uv, f = [], [], [], []
    for part in parts:
        offset = len(v)
        v += part["v"]; n += part["n"]; uv += part["uv"]
        f += [tuple(offset+i for i in tri) for tri in part["f"]]
    return dict(name=name, mat=mat, v=v, n=n, uv=uv, f=f)


def tree(style, lod, seg):
    h = {"A":340.0,"B":300.0,"C":280.0}[style]
    lx, ly = ((40.0,18.0) if style == "B" else (0.0,0.0))
    result = [cylinder(f"LOD{lod}_Bark", h, 30.0 if style != "C" else 36.0, 14.0, seg, "WM_Bark", lx, ly)]
    if style == "A":
        result.append(ellipsoid(f"LOD{lod}_Foliage",95,88,120,max(5,seg),max(3,seg//2),"WM_Foliage",h+55,lx,ly))
    elif style == "B":
        a = ellipsoid("x",90,75,78,max(5,seg),max(3,seg//2),"WM_Foliage",h+55,lx-50,ly)
        b = ellipsoid("x",82,92,70,max(5,seg),max(3,seg//2),"WM_Foliage",h+80,lx+55,ly+20)
        result.append(merge(f"LOD{lod}_Foliage","WM_Foliage",[a,b]))
    else:
        a = ellipsoid("x",135,120,58,max(5,seg),max(3,seg//2),"WM_Foliage",h+55,lx,ly)
        b = ellipsoid("x",72,68,50,max(5,seg),max(3,seg//2),"WM_Foliage",h+120,lx,ly)
        result.append(merge(f"LOD{lod}_Foliage","WM_Foliage",[a,b]))
    return result


def obj_text(name, groups):
    lines = [f"# World Makers P2 authored source mesh: {name}", "# Units: centimeters | Up: +Z | Forward: +X", "mtllib WM_Rainforest_P2.mtl"]
    vo = vto = vno = 0
    for group in groups:
        lines.append(f"o {name}_{group['name']}")
        lines += ["v %.4f %.4f %.4f" % x for x in group["v"]]
        lines += ["vt %.5f %.5f" % x for x in group["uv"]]
        lines += ["vn %.5f %.5f %.5f" % x for x in group["n"]]
        lines.append("usemtl " + group["mat"])
        for face in group["f"]:
            lines.append("f " + " ".join(f"{vo+i+1}/{vto+i+1}/{vno+i+1}" for i in face))
        vo += len(group["v"]); vto += len(group["uv"]); vno += len(group["n"])
    return "\n".join(lines) + "\n"


def build_assets():
    assets = {}
    assets["SM_WM_RF_Ground_A.obj"] = obj_text("SM_WM_RF_Ground_A", [disc(f"LOD{i}",650,s,"WM_Ground",4) for i,s in enumerate((32,18,10))])
    assets["SM_WM_RF_TerrainMound_A.obj"] = obj_text("SM_WM_RF_TerrainMound_A", [ellipsoid(f"LOD{i}",180,150,70,s,max(3,s//2),"WM_Terrain",45) for i,s in enumerate((18,12,8))])
    for style in "ABC":
        groups = []
        for i, seg in enumerate((12,8,5)):
            groups += tree(style, i, seg)
        assets[f"SM_WM_RF_Tree_{style}.obj"] = obj_text(f"SM_WM_RF_Tree_{style}", groups)
    assets["SM_WM_RF_Understory_A.obj"] = obj_text("SM_WM_RF_Understory_A", [leaves(f"LOD{i}",count,75,"WM_Foliage") for i,count in enumerate((8,5))])
    assets["SM_WM_RF_Rock_A.obj"] = obj_text("SM_WM_RF_Rock_A", [ellipsoid(f"LOD{i}",78,68,52,s,max(3,s//2),"WM_Stone",35) for i,s in enumerate((12,8,5))])
    assets["SM_WM_RF_WaterEdge_A.obj"] = obj_text("SM_WM_RF_WaterEdge_A", [disc(f"LOD{i}",240,s,"WM_Water",0) for i,s in enumerate((18,10))])
    hero = []
    for i, seg in enumerate((14,10,7,5)):
        hero += [
            cylinder(f"LOD{i}_Bark",560,72,34,seg,"WM_Bark",28,-14),
            ellipsoid(f"LOD{i}_Foliage",245,205,118,max(6,seg),max(3,seg//2),"WM_Foliage",598,28,-14),
        ]
    assets["SM_WM_RF_HeroCeiba.obj"] = obj_text("SM_WM_RF_HeroCeiba", hero)
    return assets


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--output-dir", default="SourceArt/WorldMakers/Environment/Rainforest/GeneratedP2")
    parser.add_argument("--verify", action="store_true")
    args = parser.parse_args()
    out = Path(args.output_dir)
    out.mkdir(parents=True, exist_ok=True)
    assets = build_assets()
    for filename, text in assets.items():
        digest = hashlib.sha256(text.encode("utf-8")).hexdigest()
        if args.verify and digest != EXPECTED_SHA256[filename]:
            raise SystemExit(f"hash mismatch for {filename}: {digest}")
        (out / filename).write_text(text, encoding="utf-8", newline="\n")
    (out / "WM_Rainforest_P2.mtl").write_text(MTL, encoding="utf-8", newline="\n")
    print(f"Generated {len(assets)} rainforest source meshes in {out}")


if __name__ == "__main__":
    main()
