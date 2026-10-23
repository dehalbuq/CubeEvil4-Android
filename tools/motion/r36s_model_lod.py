#!/usr/bin/env python3
"""Conservative OFFLINE display-list LOD for selected user-owned GameCube RE4 .bin models.

Not a runtime GX draw filter: works before gameplay on one identified static
model at a time. Uses meshbin.py for a byte-sized, game-format round trip.
This experimental tool does not read/write DAS archives or inject assets.
"""
from __future__ import annotations

import argparse
from collections import Counter, defaultdict
from copy import deepcopy
import json
import math
from pathlib import Path
import sys

# The repository's existing ModelData parser and byte-exact serialiser.
from meshbin import parse, serialise, Prim, OP_TRIANGLES


def centroid_normal(mesh, index, normals):
    idxs = normals.get(index, ())
    if len(idxs) != 1:
        return None
    normal = mesh.nrm[next(iter(idxs))]
    xyz = normal[:3]
    d = math.sqrt(sum(float(v) * float(v) for v in xyz))
    return tuple(float(v) / d for v in xyz) if d > 0 else None


def geometric_normal(mesh, verts):
    pts = [mesh.vtx[x[0]][:3] for x in verts]
    a, b, c = pts
    u = [b[i] - a[i] for i in range(3)]
    v = [c[i] - a[i] for i in range(3)]
    return (u[1] * v[2] - u[2] * v[1],
            u[2] * v[0] - u[0] * v[2],
            u[0] * v[1] - u[1] * v[0])


def skip_or_reject_nonstatic(mesh):
    # Never modify morph targets or weighted/skeletal animations.
    if mesh.shapes is not None or mesh.n_parts != 1:
        return "animated_or_multipart_model"
    if len(mesh.weights) > 1 or any(w.num > 1 for w in mesh.weights):
        return "weighted_model"
    if not mesh.vtx or not mesh.parts:
        return "empty_model"
    return None


def build_candidate(mesh, cell_fraction, allow_alpha=False):
    model = deepcopy(mesh)
    xyz = [v[:3] for v in mesh.vtx]
    mn = [min(p[a] for p in xyz) for a in range(3)]
    mx = [max(p[a] for p in xyz) for a in range(3)]
    diag = math.sqrt(sum((mx[a] - mn[a]) ** 2 for a in range(3)))
    cell = max(1.0, cell_fraction * diag)
    owners = Counter()
    for part in mesh.parts:
        for ix in {v[0] for prim in part.prims for v in prim.verts}:
            owners[ix] += 1
    stats = dict(parts_changed=0, parts_ineligible=0, triangles_before=mesh.n_triangles(),
                 triangles_after=mesh.n_triangles(), display_bytes_saved=0, edges_locked=0,
                 vertices_clustered=0, cell_fraction=cell_fraction,
                 max_position_error_percent=0.0)
    max_displacement = 0.0

    for pi, part in enumerate(mesh.parts):
        if (part.flags & 4) and not allow_alpha:
            stats["parts_ineligible"] += 1
            continue  # Alpha cutouts are often vegetation; do not risk holes.
        tris = [tri for prim in part.prims for tri in _triangles(prim)]
        if len(tris) < 32:
            stats["parts_ineligible"] += 1
            continue
        edge_count = Counter()
        usage = defaultdict(set)  # per-position (normal, texcoord, colour)
        for t in tris:
            for v in t:
                usage[v[0]].add((v[1], v[2], v[3]))
            pos = [v[0] for v in t]
            for a, b in ((0, 1), (1, 2), (2, 0)):
                if pos[a] != pos[b]:
                    edge_count[tuple(sorted((pos[a], pos[b])))] += 1
        locked = set()
        for edge, count in edge_count.items():
            if count != 2:
                locked.update(edge)
        locked.update(ix for ix, n in owners.items() if n > 1)
        locked.update(ix for ix, attrs in usage.items() if len(attrs) > 1)
        stats["edges_locked"] += len(locked)
        # Position clustering only inside well-formed, manifold interiors.
        groups = defaultdict(list)
        for index in usage:
            if index in locked:
                continue
            x, y, z, joint = mesh.vtx[index]
            k = (int(math.floor((x - mn[0]) / cell)),
                 int(math.floor((y - mn[1]) / cell)),
                 int(math.floor((z - mn[2]) / cell)), joint)
            groups[k].append(index)
        remap = {}
        for points in groups.values():
            if len(points) <= 1:
                continue
            # Avoid collapsing vertices across hard normal discontinuities.
            normals = {i: centroid_normal(mesh, i, defaultdict(set, {
                i: {next(iter(usage[i]))[0]}
            })) for i in points}
            for root in sorted(points):
                if root in remap:
                    continue
                remap[root] = root
                nroot = normals[root]
                if nroot is None:
                    continue
                for child in sorted(points):
                    if child <= root or child in remap or normals[child] is None:
                        continue
                    if sum(a*b for a,b in zip(nroot,normals[child])) < 0.985:
                        continue
                    d = math.sqrt(sum((mesh.vtx[root][i]-mesh.vtx[child][i])**2 for i in range(3)))
                    if d > cell:
                        continue
                    remap[child] = root
                    max_displacement = max(max_displacement, d)
        stats["vertices_clustered"] += sum(1 for ix, dest in remap.items() if ix != dest)
        if not any(k != v for k, v in remap.items()):
            continue
        changed = False
        nextprims = []
        for prim in part.prims:
            before = len(_triangles(prim))
            new_triangles = []
            for tri in _triangles(prim):
                after = [((remap.get(v[0], v[0])), v[1], v[2], v[3]) for v in tri]
                if len(set(v[0] for v in after)) < 3:
                    continue
                src_n, dst_n = geometric_normal(mesh, tri), geometric_normal(mesh, after)
                s2 = sum(v*v for v in src_n)
                d2 = sum(v*v for v in dst_n)
                # Don't flip faces or dramatically alter the triangle normal.
                if s2 and d2 and sum(a*b for a,b in zip(src_n,dst_n)) < 0.8 * math.sqrt(s2*d2):
                    after = list(tri)
                new_triangles.append(after)
            after_count = len(new_triangles)
            newvertices = [v for tri in new_triangles for v in tri]
            # Convert strips/quads only when it ALSO reduces GX stream bytes.
            # This prevents CPU/IPC regressions from decomposing triangle strips.
            oldbytes = 3 + 8 * len(prim.verts)
            newbytes = 3 + 8 * len(newvertices)
            if 0 < after_count < before and newbytes < oldbytes and len(newvertices) <= 65535:
                nextprims.append(Prim(OP_TRIANGLES, newvertices))
                stats["triangles_after"] -= before - after_count
                stats["display_bytes_saved"] += oldbytes - newbytes
                changed = True
            else:
                nextprims.append(deepcopy(prim))
        if changed:
            model.parts[pi].prims = nextprims
            model.parts[pi].n_poly = sum(len(_triangles(pr)) for pr in nextprims)
            stats["parts_changed"] += 1
    if diag:
        stats["max_position_error_percent"] = round(100.0 * max_displacement / diag, 4)
    stats["triangles_removed"] = stats["triangles_before"] - stats["triangles_after"]
    stats["reduction_percent"] = round(100.0 * stats["triangles_removed"] / max(1, stats["triangles_before"]), 3)
    return model, stats


def _triangles(primitive):
    # Match the existing meshbin.Part.triangles() decode, without a temporary Part.
    return _triangles_manual(primitive)


def _triangles_manual(p):
    from meshbin import OP_QUADS, OP_STRIP
    v=p.verts
    if p.op == OP_TRIANGLES:
        return [tuple(v[i:i+3]) for i in range(0, len(v)-2, 3)]
    if p.op == OP_QUADS:
        out=[]
        for i in range(0,len(v)-3,4):
            out += [(v[i],v[i+1],v[i+2]),(v[i],v[i+2],v[i+3])]
        return out
    if p.op == OP_STRIP:
        return [(v[i],v[i+1],v[i+2]) if i%2==0
                else (v[i+1],v[i],v[i+2]) for i in range(len(v)-2)]
    raise ValueError("Unsupported GX primitive")


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("input", type=Path, help="user-owned standalone static ModelData .bin")
    parser.add_argument("--output",type=Path,help="write modified BIN; otherwise ANALYZE only")
    parser.add_argument("--target",type=float,default=.5,help="desired fraction of triangles, default 0.5")
    parser.add_argument("--max-error",type=float,default=.025,
                        help="max cell diameter as fraction of bbox diagonal, default 2.5%%")
    parser.add_argument("--allow-alpha",action="store_true",
                        help="unsafe experiment: include cutout/alpha parts (vegetation can degrade)")
    args=parser.parse_args()
    if not (0.1 <= args.target <= 1) or not (0 < args.max_error <= .1):
        parser.error("target in [0.1,1], max-error in (0,0.1]")
    original=args.input.read_bytes()
    mesh=parse(original)
    reason=skip_or_reject_nonstatic(mesh)
    if reason:
        print(json.dumps({"status":"SKIPPED","reason":reason,"original_triangles":mesh.n_triangles()},indent=2))
        return 0
    best=None
    for f in [args.max_error/16,args.max_error/8,args.max_error/4,args.max_error/2,args.max_error]:
        model, stat=build_candidate(mesh,f,args.allow_alpha)
        if best is None or stat["triangles_removed"] > best[1]["triangles_removed"]:
            best=(model,stat)
        if stat["triangles_after"] <= mesh.n_triangles()*args.target:
            best=(model,stat)
            break
    model,stat=best
    stat["mode"]="GENERATE" if args.output else "ANALYZE"
    stat["status"]="LOD_GENERATED" if stat["triangles_removed"] else "NO_SAFE_REDUCTION_FOUND"
    stat["original_bytes"]=len(original)
    if args.output and stat["triangles_removed"]:
        blob=serialise(model)
        if len(blob) != len(original):
            raise RuntimeError("Unsafe size change: original archive layout must be preserved")
        check=parse(blob)
        assert check.n_triangles() == stat["triangles_after"]
        assert len(check.vtx)==len(mesh.vtx) and len(check.parts)==len(mesh.parts)
        if args.output.resolve()==args.input.resolve():
            raise RuntimeError("Refusing in-place modification of original game model")
        args.output.parent.mkdir(parents=True,exist_ok=True)
        args.output.write_bytes(blob)
        stat["output"]=str(args.output)
        stat["output_bytes"]=len(blob)
    print(json.dumps(stat,indent=2,sort_keys=True))
    return 0


if __name__=="__main__":
    sys.exit(main())
