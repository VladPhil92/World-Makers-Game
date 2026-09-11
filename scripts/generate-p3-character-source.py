#!/usr/bin/env python3
"""Generate the P3 Child Explorer source mesh bundle.

The bundle is DCC-neutral JSON containing real low-poly geometry, UV0, normals,
triangles, 19-joint skin weights and authored LODs for the base avatar and
modular cosmetics. Blender consumes the bundle to create a rigged FBX payload.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import math
from pathlib import Path

EXPECTED_SHA256 = "48144053f0907c387ecd12803a39f7f34bfbc8f3f5578557bb7863ae59b9f15f"

JOINTS = [
    {"id":"root","parent":None,"headCm":[0.0,0.0,0.0]},
    {"id":"pelvis","parent":"root","headCm":[0.0,0.0,74.0]},
    {"id":"spine","parent":"pelvis","headCm":[0.0,0.0,90.0]},
    {"id":"chest","parent":"spine","headCm":[0.0,0.0,110.0]},
    {"id":"neck","parent":"chest","headCm":[0.0,0.0,124.0]},
    {"id":"head","parent":"neck","headCm":[0.0,0.0,140.0]},
    {"id":"jaw","parent":"head","headCm":[8.0,0.0,136.0]},
    {"id":"upperarm_l","parent":"chest","headCm":[0.0,-19.0,112.0]},
    {"id":"lowerarm_l","parent":"upperarm_l","headCm":[0.0,-42.0,103.0]},
    {"id":"hand_l","parent":"lowerarm_l","headCm":[1.0,-60.0,95.0]},
    {"id":"upperarm_r","parent":"chest","headCm":[0.0,19.0,112.0]},
    {"id":"lowerarm_r","parent":"upperarm_r","headCm":[0.0,42.0,103.0]},
    {"id":"hand_r","parent":"lowerarm_r","headCm":[1.0,60.0,95.0]},
    {"id":"thigh_l","parent":"pelvis","headCm":[0.0,-9.0,72.0]},
    {"id":"calf_l","parent":"thigh_l","headCm":[0.0,-9.0,42.0]},
    {"id":"foot_l","parent":"calf_l","headCm":[10.0,-9.0,10.0]},
    {"id":"thigh_r","parent":"pelvis","headCm":[0.0,9.0,72.0]},
    {"id":"calf_r","parent":"thigh_r","headCm":[0.0,9.0,42.0]},
    {"id":"foot_r","parent":"calf_r","headCm":[10.0,9.0,10.0]},
]

MATERIALS = {
    "WM_Skin":[0.63,0.40,0.27],
    "WM_Hair":[0.10,0.07,0.05],
    "WM_Top":[0.10,0.36,0.55],
    "WM_Bottom":[0.20,0.24,0.31],
    "WM_Footwear":[0.12,0.12,0.14],
    "WM_Accessory":[0.73,0.48,0.12],
}

def _norm(v):
    l=math.sqrt(sum(x*x for x in v)) or 1.0
    return tuple(x/l for x in v)

def _orthonormal_basis(axis):
    ax=_norm(axis)
    helper=(0.0,0.0,1.0) if abs(ax[2]) < 0.9 else (0.0,1.0,0.0)
    u=_norm((ax[1]*helper[2]-ax[2]*helper[1], ax[2]*helper[0]-ax[0]*helper[2], ax[0]*helper[1]-ax[1]*helper[0]))
    v=(ax[1]*u[2]-ax[2]*u[1], ax[2]*u[0]-ax[0]*u[2], ax[0]*u[1]-ax[1]*u[0])
    return ax,u,v

def _module(name, material, skinned):
    return {"name":name,"material":material,"skinned":skinned,"vertices":[],"normals":[],"uv0":[],"triangles":[],"weights":[]}

def _add_mesh(dst, src):
    off=len(dst["vertices"])
    dst["vertices"].extend(src["vertices"]); dst["normals"].extend(src["normals"]); dst["uv0"].extend(src["uv0"])
    dst["triangles"].extend([[off+a,off+b,off+c] for a,b,c in src["triangles"]])
    dst["weights"].extend(src["weights"])
    return dst

def ellipsoid(name, center, radii, seg, rings, material, bone, skinned=True):
    m=_module(name, material, skinned)
    cx,cy,cz=center; rx,ry,rz=radii
    for j in range(rings+1):
        ph=math.pi*j/rings
        for i in range(seg):
            a=2*math.pi*i/seg
            x=cx+rx*math.sin(ph)*math.cos(a); y=cy+ry*math.sin(ph)*math.sin(a); z=cz+rz*math.cos(ph)
            m["vertices"].append([round(x,4),round(y,4),round(z,4)])
            n=_norm(((x-cx)/(rx*rx),(y-cy)/(ry*ry),(z-cz)/(rz*rz)))
            m["normals"].append([round(n[0],5),round(n[1],5),round(n[2],5)])
            m["uv0"].append([round(i/seg,5),round(j/rings,5)])
            m["weights"].append([[bone,1.0]] if skinned else [])
    for j in range(rings):
        for i in range(seg):
            ni=(i+1)%seg
            a=j*seg+i; b=j*seg+ni; c=(j+1)*seg+ni; d=(j+1)*seg+i
            m["triangles"] += [[a,b,c],[a,c,d]]
    return m

def tube(name, p0, p1, r0, r1, seg, material, bone0, bone1=None, skinned=True):
    m=_module(name, material, skinned)
    axis=(p1[0]-p0[0],p1[1]-p0[1],p1[2]-p0[2]); _,u,v=_orthonormal_basis(axis)
    for ring,(p,r,t) in enumerate(((p0,r0,0.0),(((p0[0]+p1[0])*0.5,(p0[1]+p1[1])*0.5,(p0[2]+p1[2])*0.5),(r0+r1)*0.5,0.5),(p1,r1,1.0))):
        for i in range(seg):
            a=2*math.pi*i/seg
            radial=(u[0]*math.cos(a)+v[0]*math.sin(a),u[1]*math.cos(a)+v[1]*math.sin(a),u[2]*math.cos(a)+v[2]*math.sin(a))
            q=(p[0]+r*radial[0],p[1]+r*radial[1],p[2]+r*radial[2])
            m["vertices"].append([round(q[0],4),round(q[1],4),round(q[2],4)])
            m["normals"].append([round(radial[0],5),round(radial[1],5),round(radial[2],5)])
            m["uv0"].append([round(i/seg,5),t])
            if skinned:
                if bone1 and bone1 != bone0:
                    m["weights"].append([[bone0,round(1.0-t,4)],[bone1,round(t,4)]] if 0.0 < t < 1.0 else [[bone0 if t == 0.0 else bone1,1.0]])
                else:
                    m["weights"].append([[bone0,1.0]])
            else: m["weights"].append([])
    for ring in range(2):
        base=ring*seg; nxt=(ring+1)*seg
        for i in range(seg):
            j=(i+1)%seg
            m["triangles"] += [[base+i,base+j,nxt+j],[base+i,nxt+j,nxt+i]]
    return m

def box(name, center, half, material, bone=None, skinned=True):
    cx,cy,cz=center; hx,hy,hz=half
    corners=[(cx+sx*hx,cy+sy*hy,cz+sz*hz) for sx,sy,sz in [(-1,-1,-1),(1,-1,-1),(1,1,-1),(-1,1,-1),(-1,-1,1),(1,-1,1),(1,1,1),(-1,1,1)]]
    faces=[(0,1,2,3),(4,7,6,5),(0,4,5,1),(1,5,6,2),(2,6,7,3),(4,0,3,7)]
    m=_module(name, material, skinned)
    for face in faces:
        a,b,c,d=[corners[k] for k in face]
        n=_norm(((b[1]-a[1])*(c[2]-a[2])-(b[2]-a[2])*(c[1]-a[1]),(b[2]-a[2])*(c[0]-a[0])-(b[0]-a[0])*(c[2]-a[2]),(b[0]-a[0])*(c[1]-a[1])-(b[1]-a[1])*(c[0]-a[0])))
        base=len(m["vertices"])
        for uv,q in zip(((0,0),(1,0),(1,1),(0,1)),(a,b,c,d)):
            m["vertices"].append([round(x,4) for x in q]); m["normals"].append([round(x,5) for x in n]); m["uv0"].append(list(uv)); m["weights"].append([[bone,1.0]] if skinned else [])
        m["triangles"] += [[base,base+1,base+2],[base,base+2,base+3]]
    return m

def tetra(name, center, scale, material):
    cx,cy,cz=center; sx,sy,sz=scale
    pts=[(cx+sx,cy,cz),(cx-sx*0.6,cy+sy,cz-sz*0.5),(cx-sx*0.6,cy-sy,cz-sz*0.5),(cx-sx*0.3,cy,cz+sz)]
    faces=[(0,1,2),(0,3,1),(0,2,3),(1,3,2)]
    m=_module(name, material, False)
    for face in faces:
        a,b,c=[pts[k] for k in face]
        n=_norm(((b[1]-a[1])*(c[2]-a[2])-(b[2]-a[2])*(c[1]-a[1]),(b[2]-a[2])*(c[0]-a[0])-(b[0]-a[0])*(c[2]-a[2]),(b[0]-a[0])*(c[1]-a[1])-(b[1]-a[1])*(c[0]-a[0])))
        base=len(m["vertices"])
        for uv,q in zip(((0,0),(1,0),(0.5,1)),(a,b,c)):
            m["vertices"].append([round(x,4) for x in q]); m["normals"].append([round(x,5) for x in n]); m["uv0"].append(list(uv)); m["weights"].append([])
        m["triangles"].append([base,base+1,base+2])
    return m

def body_lod(lod, seg):
    m=_module("body","WM_Skin",True)
    m=_add_mesh(m, ellipsoid("head",(0,0,143),(13,14.5,15),seg,max(3,seg//2),"WM_Skin","head"))
    m=_add_mesh(m, tube("neck",(0,0,122),(0,0,131),5.5,5.2,seg,"WM_Skin","neck","head"))
    m=_add_mesh(m, tube("torso",(0,0,73),(0,0,122),15,18,seg,"WM_Skin","pelvis","chest"))
    for side,sy in (("l",-1),("r",1)):
        ua=f"upperarm_{side}"; la=f"lowerarm_{side}"; hand=f"hand_{side}"
        p0=(0,19*sy,112); p1=(0,42*sy,103); p2=(1,60*sy,95)
        m=_add_mesh(m,tube(ua,p0,p1,6.0,5.2,seg,"WM_Skin",ua,la))
        m=_add_mesh(m,tube(la,p1,p2,5.2,4.2,seg,"WM_Skin",la,hand))
        m=_add_mesh(m,ellipsoid(hand,(2,63*sy,93),(6.5,5.0,7.5),max(5,seg),max(3,seg//2),"WM_Skin",hand))
        th=f"thigh_{side}"; ca=f"calf_{side}"; ft=f"foot_{side}"
        q0=(0,9*sy,72); q1=(0,9*sy,42); q2=(4,9*sy,12)
        m=_add_mesh(m,tube(th,q0,q1,8.2,7.0,seg,"WM_Skin",th,ca))
        m=_add_mesh(m,tube(ca,q1,q2,7.0,5.8,seg,"WM_Skin",ca,ft))
        m=_add_mesh(m,ellipsoid(ft,(10,9*sy,7),(13,7.5,7),max(5,seg),max(3,seg//2),"WM_Skin",ft))
    return m

def top_lod(lod, seg):
    m=_module("top","WM_Top",True)
    m=_add_mesh(m,tube("top-shell",(0,0,78),(0,0,120),16.2,19.5,seg,"WM_Top","pelvis","chest"))
    for side,sy in (("l",-1),("r",1)):
        m=_add_mesh(m,tube("sleeve_"+side,(0,18*sy,112),(0,29*sy,108),7.0,6.5,seg,"WM_Top","upperarm_"+side))
    return m

def bottom_lod(lod, seg):
    m=_module("bottom","WM_Bottom",True)
    m=_add_mesh(m,tube("waist",(0,0,70),(0,0,82),16.5,17,seg,"WM_Bottom","pelvis"))
    for side,sy in (("l",-1),("r",1)):
        m=_add_mesh(m,tube("short_"+side,(0,9*sy,72),(0,9*sy,56),9.0,8.5,seg,"WM_Bottom","thigh_"+side))
    return m

def footwear_lod(lod, seg):
    m=_module("footwear","WM_Footwear",True)
    for side,sy in (("l",-1),("r",1)):
        m=_add_mesh(m,ellipsoid("shoe_"+side,(11,9*sy,7),(14.5,8.4,7.8),max(5,seg),max(3,seg//2),"WM_Footwear","foot_"+side))
    return m

def hair_lod(lod, seg):
    m=_module("hair","WM_Hair",True)
    m=_add_mesh(m,ellipsoid("hair-cap",(-1,0,150.5),(13.8,15.2,7.0),seg,max(3,seg//2),"WM_Hair","head"))
    m=_add_mesh(m,box("hair-lock",(-8,0,143),(4.5,12,6),"WM_Hair","head",True))
    return m

def static_accessory(name, kind, lod):
    if kind=="head-accessory":
        return box(name,(0,0,4),(8.5,9.5,2.2),"WM_Accessory",None,False) if lod == 0 else tetra(name,(0,0,3),(8.5,9.5,4.0),"WM_Accessory")
    if kind=="back-accessory":
        return box(name,(-6,0,-4),(5.5,12,14),"WM_Accessory",None,False) if lod == 0 else tetra(name,(-6,0,-4),(7,12,16),"WM_Accessory")
    return box(name,(9,0,0),(12,2.2,2.2),"WM_Accessory",None,False) if lod == 0 else tetra(name,(9,0,0),(12,3,3),"WM_Accessory")

MODULES = [
    ("body","SK_WM_ChildExplorer","skinned",4,"WM_Skin"),
    ("hair","SK_WM_ChildExplorer_Hair_A","skinned",3,"WM_Hair"),
    ("top","SK_WM_ChildExplorer_Top_A","skinned",4,"WM_Top"),
    ("bottom","SK_WM_ChildExplorer_Bottom_A","skinned",4,"WM_Bottom"),
    ("footwear","SK_WM_ChildExplorer_Footwear_A","skinned",4,"WM_Footwear"),
    ("head-accessory","SM_WM_ChildExplorer_HeadAccessory_A","socket",2,"WM_Accessory"),
    ("back-accessory","SM_WM_ChildExplorer_BackAccessory_A","socket",2,"WM_Accessory"),
    ("hand-prop","SM_WM_ChildExplorer_HandProp_A","socket",2,"WM_Accessory"),
]
SEGMENTS=[12,8,6,5]

def build_bundle():
    modules=[]
    for slot,asset,kind,lod_count,mat in MODULES:
        lods=[]
        for lod in range(lod_count):
            seg=SEGMENTS[min(lod,len(SEGMENTS)-1)]
            if slot=="body": mesh=body_lod(lod,seg)
            elif slot=="hair": mesh=hair_lod(lod,seg)
            elif slot=="top": mesh=top_lod(lod,seg)
            elif slot=="bottom": mesh=bottom_lod(lod,seg)
            elif slot=="footwear": mesh=footwear_lod(lod,seg)
            else: mesh=static_accessory(slot,slot,lod)
            lods.append({
                "lod":lod,"vertices":mesh["vertices"],"normals":mesh["normals"],"uv0":mesh["uv0"],
                "triangles":mesh["triangles"],"weights":mesh["weights"],
                "triangleCount":len(mesh["triangles"])
            })
        modules.append({"slot":slot,"assetName":asset,"kind":kind,"material":mat,"lods":lods})
    return {
        "schemaVersion":1,
        "units":"centimeters",
        "upAxis":"Z",
        "forwardAxis":"X",
        "baseHeightCm":158.0,
        "headsTall":5.0,
        "retargetPose":"neutral-a-pose",
        "skeleton":{"joints":JOINTS,"maxInfluences":4},
        "materials":MATERIALS,
        "modules":modules,
        "privacy":{"biometricCapture":False,"photoAvatarGeneration":False,"childVoiceFaceTraining":False},
    }

def canonical_bytes(bundle):
    return (json.dumps(bundle,sort_keys=True,separators=(",",":"),ensure_ascii=False)+"\n").encode("utf-8")

def main():
    ap=argparse.ArgumentParser()
    ap.add_argument("--output-dir",default="SourceArt/WorldMakers/Characters/Player/P3")
    ap.add_argument("--verify",action="store_true")
    args=ap.parse_args()
    out=Path(args.output_dir); out.mkdir(parents=True,exist_ok=True)
    payload=canonical_bytes(build_bundle())
    digest=hashlib.sha256(payload).hexdigest()
    if args.verify and digest != EXPECTED_SHA256:
        raise SystemExit(f"hash mismatch for WM_ChildExplorer_P3.source.json: {digest}")
    (out/"WM_ChildExplorer_P3.source.json").write_bytes(payload)
    print(f"Generated P3 Child Explorer source bundle: sha256={digest}")

if __name__=="__main__":
    main()
