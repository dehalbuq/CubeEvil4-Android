#!/usr/bin/env python3
"""Synthetic GameCube ModelData BIN tests; never use or redistribute game assets."""
from __future__ import annotations
from copy import deepcopy
import sys
import unittest
from pathlib import Path

sys.path.insert(0,str(Path(__file__).resolve().parents[1]/"tools"/"motion"))
from meshbin import Mesh, Head, Weight, Prim, Part, VERSION_NEW, FLAG_NRM_S8, FLAG_TEX_S16_CLR, OP_TRIANGLES, align, parse, serialise
from r36s_model_lod import build_candidate, skip_or_reject_nonstatic


def make_grid(nx=24, ny=24, alpha=False):
    verts=[(x,y,0,0) for y in range(ny+1) for x in range(nx+1)]
    def v(x,y):return (y*(nx+1)+x,0,0,0)
    corners=[]
    for y in range(ny):
        for x in range(nx):
            a,b,c,d=v(x,y),v(x+1,y),v(x+1,y+1),v(x,y+1)
            corners.extend([a,b,c,a,c,d])
    p=Prim(OP_TRIANGLES,corners)
    part=Part(4 if alpha else 0, 0,0,0,0,(0,0,0),0,0,0,0,0,
              nx*ny*2,align(3+8*len(corners)),[p])
    return Mesh(VERSION_NEW, FLAG_TEX_S16_CLR|FLAG_NRM_S8, 0,1,1,0,0,
                [Head(0,255,0,0,(0.,0.,0.))], verts, [(0,0,64,0)],
                [0xffffffff],[(0,0)],[Weight((0,0,0),1,(100,0,0,0))],
                [part], [], [0], None, 0xcd)


class LODTests(unittest.TestCase):
    def test_roundtrip_fixture(self):
        m=make_grid()
        blob=serialise(m)
        p=parse(blob)
        self.assertEqual(p.n_triangles(),24*24*2)
        self.assertEqual(blob,serialise(p))

    def test_opaque_static_mesh_is_simplified_and_length_kept(self):
        src=make_grid()
        before=serialise(src)
        after,stats=build_candidate(src,0.1)
        self.assertGreater(stats["triangles_removed"],0)
        self.assertLess(stats["triangles_after"],stats["triangles_before"])
        self.assertLess(stats["display_bytes_saved"],len(before))
        self.assertEqual(len(serialise(after)),len(before))
        p=parse(serialise(after))
        self.assertEqual(p.n_triangles(),stats["triangles_after"])
        self.assertEqual(serialise(src),before,"must not edit user's source instance")

    def test_alpha_cutout_untouched(self):
        src=make_grid(alpha=True)
        out,stats=build_candidate(src,0.1)
        self.assertEqual(stats["triangles_removed"],0)
        self.assertEqual(serialise(src),serialise(out))

    def test_morph_and_multipart_rejected(self):
        src=make_grid()
        src.shapes=[[]]
        self.assertEqual(skip_or_reject_nonstatic(src),"animated_or_multipart_model")
        src.shapes=None
        src.heads.append(Head(1,0,0,0,(0.,0.,0.)))
        self.assertEqual(skip_or_reject_nonstatic(src),"animated_or_multipart_model")


if __name__=="__main__":
    unittest.main()
