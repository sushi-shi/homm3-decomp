#!/usr/bin/env python3
"""homm3.core.common - the shared primitives the runtime pipeline needs.

ONLY what build/ and match/ actually use lives here (measured, not
wholesale): repo paths, the pinned-image hash gate, the PE image loader,
and the provenance header. The carve-era machinery that once surrounded
these primitives has been retired; Git history retains the bootstrap tools.

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

from homm3.core.project import Project

SCRIPT_DIR = Path(__file__).resolve().parent
HOMM3_DIR = Path(os.environ.get("HOMM3_DIR") or next(
    (p for p in SCRIPT_DIR.parents if (p / "flake.nix").exists()), SCRIPT_DIR))

# Offline annotation/provenance facts are admitted project data. Operations
# that read executable bytes use Project.image and its parsed layout.
_spec = Project(HOMM3_DIR).specification['inputs']['retail']
TARGET_SHA256 = _spec['sha256']
TARGET_SIZE = _spec['size']
IMAGE_BASE = _spec['image_base']
BUILD_EXE = HOMM3_DIR / _spec['path']


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
    image = Image(str(exe))
    if image.image_base != IMAGE_BASE:
        die(f"image base {image.image_base:#x} != admitted {IMAGE_BASE:#x}")
    info["image_base"] = image.image_base
    return image, info


def provenance(generator: str, extra: list[str] | None = None) -> list[str]:
    lines = [
        f"# generator: {generator}",
        f"# exe: {BUILD_EXE.name} sha256={TARGET_SHA256} size={TARGET_SIZE}",
        f"# date: {datetime.date.today().isoformat()}",
        "# ANALYSIS OUTPUT, NOT RETAIL EVIDENCE",
    ]
    return lines + list(extra or [])
