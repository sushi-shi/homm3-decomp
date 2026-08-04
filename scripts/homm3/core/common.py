#!/usr/bin/env python3
"""homm3.core.common - the shared primitives the runtime pipeline needs.

ONLY what build/ and match/ actually use lives here (measured, not
wholesale): repo paths, the pinned-image hash gate, the PE image loader,
and the provenance header. Carve-specific machinery (the sanitized
llvm-objdump copy, target.json intake stamp, TSV read/write conventions)
stays in homm3.carve.common, which imports these primitives from here.

The gate is HARD: any byte deviation from the recorded sha256/size aborts.
Game bytes are never copied into the repo - the exe is referenced in place
($HOMM3_EXE, then ../orig/, then ../decomp-attempt-1/build/orig/).
"""
from __future__ import annotations

import datetime
import hashlib
import os
import sys
from pathlib import Path

SCRIPT_DIR = Path(__file__).resolve().parent
HOMM3_DIR = Path(os.environ.get("HOMM3_DIR") or next(
    (p for p in SCRIPT_DIR.parents if (p / "flake.nix").exists()), SCRIPT_DIR))
# evidence/ holds GENERATED analysis deliverables (scaffolding, slated for
# removal); config/ holds hand-admitted retail inventories + build manifests
EVIDENCE_DIR = HOMM3_DIR / "evidence"

TARGET_SHA256 = "057c9d88e7206f6669a4615de2c6e02ab6c4e2d570a9e2badf07fe0bd6247274"
TARGET_SIZE = 2732032
IMAGE_BASE = 0x400000

EXE_CANDIDATES = [
    HOMM3_DIR / "../orig/HEROES3.EXE",
    HOMM3_DIR / "../decomp-attempt-1/build/orig/HEROES3.EXE",
]


def die(msg: str) -> None:
    print(f"[homm3] ERROR: {msg}", file=sys.stderr)
    sys.exit(1)


def sha256_of(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1 << 20), b""):
            digest.update(chunk)
    return digest.hexdigest()


def resolve_exe() -> Path:
    env = os.environ.get("HOMM3_EXE")
    candidates = ([Path(env)] if env else []) + EXE_CANDIDATES
    for cand in candidates:
        if cand.is_file():
            return cand.resolve()
    die("HEROES3.EXE not found; set $HOMM3_EXE or place it at ../orig/")


def gate_exe(path: Path) -> dict:
    """The hard gate: refuse to analyze anything but the pinned pressing."""
    size = path.stat().st_size
    if size != TARGET_SIZE:
        die(f"{path}: size {size} != pinned {TARGET_SIZE}")
    sha = sha256_of(path)
    if sha != TARGET_SHA256:
        die(f"{path}: sha256 {sha} != pinned {TARGET_SHA256}")
    return {"path": str(path), "sha256": sha, "size": size,
            "image_base": IMAGE_BASE}


def load_image():
    """(Image, info) over the gated REAL exe - the runtime's reader.

    No sanitized copy and no target.json stamp: those are carve concerns
    (its llvm-objdump channels need a header-tweaked working copy); the
    runtime only reads sections and bytes."""
    from homm3.core.image import Image
    exe = resolve_exe()
    info = gate_exe(exe)
    return Image(str(exe)), info


def provenance(generator: str, extra: list[str] | None = None) -> list[str]:
    lines = [
        f"# generator: {generator}",
        f"# exe: HEROES3.EXE sha256={TARGET_SHA256} size={TARGET_SIZE}",
        f"# date: {datetime.date.today().isoformat()}",
        "# ANALYSIS OUTPUT, NOT RETAIL EVIDENCE",
    ]
    return lines + list(extra or [])
