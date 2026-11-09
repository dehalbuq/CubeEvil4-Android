#!/usr/bin/env python3
"""Read dArkOSRE snapshot without extraction or executing any user-provided file.
Never uploads anything. Designed for private reports only.
"""
from __future__ import annotations
import argparse
import hashlib
import json
import pathlib
import re
import sys
import tarfile
import zipfile

LIMIT = 8 * 1024 * 1024  # single member
TOTAL_LIMIT = 32 * 1024 * 1024
MEMBER_LIMIT = 400

def load(path: pathlib.Path):
    files = {}
    total = 0
    if zipfile.is_zipfile(path):
        with zipfile.ZipFile(path) as z:
            members = [i for i in z.infolist() if not i.is_dir()]
            if len(members) > MEMBER_LIMIT:
                raise ValueError("Too many members")
            for i in members:
                mode = (i.external_attr >> 16) & 0o170000
                if mode == 0o120000:
                    raise ValueError(f"Symlink member refused: {i.filename}")
                if i.filename in files:
                    raise ValueError(f"Duplicate archive member: {i.filename}")
                if i.file_size > LIMIT or total + i.file_size > TOTAL_LIMIT:
                    raise ValueError(f"Archive member/total exceeds limit: {i.filename}")
                content = z.read(i)
                total += len(content)
                files[i.filename] = content
            return files
    if tarfile.is_tarfile(path):
        with tarfile.open(path, "r:*") as t:
            members = t.getmembers()
            if len(members) > MEMBER_LIMIT:
                raise ValueError("Too many members")
            for member in members:
                if not member.isfile():
                    if member.isdir():
                        continue
                    raise ValueError(f"Special tar member refused: {member.name}")
                if member.name in files:
                    raise ValueError(f"Duplicate archive member: {member.name}")
                if member.size > LIMIT or total + member.size > TOTAL_LIMIT:
                    raise ValueError(f"Archive member/total exceeds limit: {member.name}")
                f = t.extractfile(member)
                if f is not None:
                    content=f.read()
                    files[member.name]=content
                    total+=len(content)
            return files
    raise ValueError("Provide a ZIP or TAR.GZ snapshot")

def verify(path: pathlib.Path) -> dict:
    all_files = load(path)
    expected_prefix = "darkosre-profile/"
    files = {}
    for name, data in all_files.items():
        p = pathlib.PurePosixPath(name)
        if p.is_absolute() or ".." in p.parts or not name.startswith(expected_prefix):
            raise ValueError(f"Unexpected unsafe member: {name}")
        if any(part in {"serial-number", ".ssh", "prod.keys", "title.keys", "disc0", "nintendo"} for part in p.parts):
            raise ValueError(f"Unexpected sensitive member: {name}")
        files[name[len(expected_prefix):]] = data
    if "profile.env" not in files or "SHA256SUMS" not in files:
        raise ValueError("Missing profile.env / SHA256SUMS")
    profile = {}
    for line in files["profile.env"].decode("utf-8", "replace").splitlines():
        if "=" in line:
            key, value = line.split("=", 1)
            if re.fullmatch(r"[A-Z0-9_]{2,50}", key):
                profile[key] = value[:100]
    if profile.get("PROFILE_VERSION") != "1":
        raise ValueError("Unsupported profile schema")
    checked = 0
    manifest_set = set()
    for line in files["SHA256SUMS"].decode("utf-8", "replace").splitlines():
        m = re.fullmatch(r"([a-f0-9]{64})\s+\./(.+)", line)
        if not m:
            raise ValueError("Malformed SHA256SUMS entry")
        digest, filename = m.groups()
        if filename in manifest_set or filename == "SHA256SUMS":
            raise ValueError(f"Duplicate or invalid manifest entry: {filename}")
        manifest_set.add(filename)
        if filename not in files or hashlib.sha256(files[filename]).hexdigest() != digest:
            raise ValueError(f"Hash mismatch: {filename}")
        checked += 1
    if checked < 4:
        raise ValueError("Unexpectedly small manifest")
    if manifest_set != set(files) - {"SHA256SUMS"}:
        raise ValueError("Archive has unmanifested files or incomplete manifest")
    return {"status": "PASS", "files": len(files), "hashes_verified": checked,
            "arch": profile.get("ARCH", "unknown"),
            "kernel": profile.get("KERNEL_RELEASE", "unknown"),
            "reported_ram_mib": profile.get("RAM_TOTAL_MIB", "unknown"),
            "cpu_logical": profile.get("CPU_LOGICAL", "unknown"),
            "cloud_gpu_equivalent": False,
            "note": "Validation only: cannot reproduce Mali-G31 timings on Mesa software"}

def main() -> int:
    p = argparse.ArgumentParser()
    p.add_argument("archive", type=pathlib.Path)
    args = p.parse_args()
    try:
        report = verify(args.archive)
    except (ValueError, OSError, EOFError, zipfile.BadZipFile, tarfile.TarError) as exc:
        print(f"SNAPSHOT_VALIDATION=FAIL reason={exc}", file=sys.stderr)
        return 1
    print("SNAPSHOT_VALIDATION=PASS")
    print(json.dumps(report, ensure_ascii=False, indent=2))
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
