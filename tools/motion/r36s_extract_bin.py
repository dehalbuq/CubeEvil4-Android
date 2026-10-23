#!/usr/bin/env python3
"""Extract *copies* of valid GameCube Resident Evil 4 ModelData BIN entries.

Input must be one .das (room), .drs (player/enemy), or .evd (event) file
from the user's own game extraction. Reads archives using the repo's existing
tools/motion/archive.py. Does not repack archives or change game assets.
"""
import argparse
from pathlib import Path
import sys

from archive import Archive
from meshbin import parse


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("archive", type=Path, help="original GC room r100.das, character .drs, or event .evd")
    parser.add_argument("--out", type=Path, default=Path("modelos"), help="directory for copies of .bin models")
    parser.add_argument("--list", action="store_true", help="only list valid models; do not write .bin files")
    parser.add_argument("--deep", action="store_true", help="include nested models in ETM entries")
    args = parser.parse_args()
    if not args.archive.is_file():
        parser.error(f"archive not found: {args.archive}")
    if args.archive.suffix.lower() not in {".das", ".drs", ".evd"}:
        parser.error("supported inputs: GameCube .das, .drs and .evd (not an ISO or .dat)")
    if not args.list:
        args.out.mkdir(parents=True, exist_ok=True)
    n = 0
    skipped = 0
    for entry in Archive(str(args.archive)).entries(tag="BIN", deep=args.deep):
        try:
            model = parse(entry.data)
        except (ValueError, IndexError, AssertionError, KeyError):
            skipped += 1
            continue
        # Each table index is unique, and nested ETM labels are disambiguated.
        nested = "".join(c if c.isascii() and c.isalnum() else "_" for c in (entry.sub or "root"))
        name = f"{args.archive.stem}_{entry.index:03d}_{nested}.bin"
        output = args.out / name
        print(f"BIN={entry.index} triangles={model.n_triangles():6d} "
              f"parts={len(model.parts):3d} bones={model.n_parts:3d} "
              f"alpha_parts={sum(bool(p.flags & 4) for p in model.parts)} -> {output}")
        if not args.list:
            if output.exists():
                print(f"  SKIP: already exists, will not overwrite {output}")
                continue
            output.write_bytes(entry.data)
        n += 1
    print(f"Valid BIN models: {n}; incompatible BIN entries ignored: {skipped}")
    if n == 0:
        print("No standalone ModelData BIN found in this archive. Try another .das/.drs or --deep.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
