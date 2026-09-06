#!/usr/bin/env python3
"""homm3.core.common - the shared primitives the runtime pipeline needs.

ONLY what build/ and match/ actually use lives here (measured, not
wholesale): repo paths, the pinned-image hash gate, the PE image loader,
and the provenance header. The carve-era machinery that once surrounded
these primitives is retired under scripts/archive/carve/.

The gate is HARD: any byte deviation from the recorded sha256/size aborts.
The retail exe is provided via $HOMM3_EXE or `homm3 init --exe` and COPIED
into gitignored build/orig/ after the hash gate passes; every later use
re-verifies the copy, so a wrong or tampered file can never be mistaken
for the pinned pressing. Game bytes never enter the repository.
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

BUILD_EXE = HOMM3_DIR / "build/orig/HEROES3.EXE"


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
    """The byte-verified working copy, staged from an explicit input if needed."""
    from homm3.core import inputs
    try:
        return inputs.stage_executable(inputs.RETAIL)
    except inputs.InputError as exc:
        die(str(exc))


def gate_exe(path: Path) -> dict:
    """The hard gate: refuse to analyze anything but the pinned pressing."""
    from homm3.core import inputs
    try:
        inputs.read_verified(inputs.RETAIL, path)
    except inputs.InputError as exc:
        die(str(exc))
    return {"path": str(path), "sha256": TARGET_SHA256, "size": TARGET_SIZE,
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
