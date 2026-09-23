"""Verify and stage unmodified, user-supplied CodeWarrior library headers."""
from __future__ import annotations

import hashlib
import fcntl
import os
from pathlib import Path
import shutil
import tempfile
import tomllib

from homm3.core import common


def specification(root: Path) -> dict:
    return tomllib.loads((root / "config/mac/sdk.toml").read_text())


def _files(directory: Path) -> dict[str, bytes]:
    if not directory.is_dir():
        raise ValueError(f"missing CodeWarrior SDK directory: {directory}")
    result = {}
    for path in sorted(directory.rglob("*")):
        if path.is_symlink():
            raise ValueError(f"CodeWarrior SDK inputs must not be symlinks: {path}")
        if path.is_file():
            result[path.relative_to(directory).as_posix()] = path.read_bytes()
    return result


def tree_digest(files: dict[str, bytes]) -> str:
    """Hash names and exact bytes, including original encodings/line endings."""
    payload = b"".join(name.encode("utf-8") + b"\0"
                       + hashlib.sha256(data).hexdigest().encode("ascii") + b"\n"
                       for name, data in sorted(files.items()))
    return hashlib.sha256(payload).hexdigest()


def _verified(directory: Path, row: dict) -> dict[str, bytes]:
    files = _files(directory)
    if len(files) != row["file_count"] or tree_digest(files) != row["sha256"]:
        raise ValueError(f"{directory}: CodeWarrior SDK tree differs from config/mac/sdk.toml")
    return files


def stage(source: str | Path | None = None, *, root: Path | None = None) -> Path:
    """Use an explicit archive root, environment path, or the verified cache."""
    root = root or common.HOMM3_DIR
    destination = root / "build/mac/sdk"
    supplied = source if source is not None else os.environ.get("HOMM3_MAC_SDK")
    if not supplied:
        inputs(root)
        return destination
    origin = Path(supplied).expanduser().resolve()
    rows = specification(root)["trees"]
    # Verify every source before replacing any staged tree.
    captured = [(row, _verified(origin / row["source"], row)) for row in rows]
    destination.parent.mkdir(parents=True, exist_ok=True)
    with (destination.parent / "sdk-stage.lock").open("a") as lock:
        fcntl.flock(lock, fcntl.LOCK_EX)
        try:
            inputs(root)
            return destination
        except (ValueError, OSError):
            pass
        with tempfile.TemporaryDirectory(dir=destination.parent, prefix="sdk-stage-") as temporary:
            prepared = Path(temporary) / "sdk"
            for row, files in captured:
                for name, data in files.items():
                    path = prepared / row["name"] / name
                    path.parent.mkdir(parents=True, exist_ok=True)
                    path.write_bytes(data)
            if destination.exists():
                shutil.rmtree(destination)
            prepared.rename(destination)
    return destination


def inputs(root: Path) -> dict[str, bytes]:
    """All SDK inputs are fingerprinted; no regex approximation of includes."""
    result = {}
    for row in specification(root)["trees"]:
        directory = root / "build/mac/sdk" / row["name"]
        if not directory.is_dir():
            raise ValueError("CodeWarrior library headers are not staged; run "
                             "`homm3 mac sdk PATH` or set HOMM3_MAC_SDK")
        for name, data in _verified(directory, row).items():
            result[f"build/mac/sdk/{row['name']}/{name}"] = data
    return result


def include_dirs(root: Path) -> tuple[str, ...]:
    return tuple("build/mac/sdk/" + row["name"] for row in specification(root)["trees"])
