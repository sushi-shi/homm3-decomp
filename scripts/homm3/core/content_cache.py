"""Disposable derived indexes keyed by content hashes (build/gen/cache).

The same discipline as the ICF and retail-twin indexes in
homm3.build.normalize_objs: an entry is reused only when the content hashes
of its inputs and of the implementation that produced it are unchanged, so
a cache can only cost time, never change a result. Timestamps are never
consulted.
"""
from __future__ import annotations

import hashlib
import os
import pickle
from pathlib import Path


def content_key(*parts: bytes | str | Path) -> str:
    """Digest of literal parts and of file contents (for Path parts)."""
    digest = hashlib.sha256()
    for part in parts:
        if isinstance(part, Path):
            part = hashlib.sha256(part.read_bytes()).digest()
        digest.update(part.encode() if isinstance(part, str) else part)
        digest.update(b"\0")
    return digest.hexdigest()


def cache_dir(root: Path | None = None) -> Path:
    if root is None:
        from homm3.core import common
        root = common.HOMM3_DIR
    return root / "build/gen/cache"


def load(name: str, root: Path | None = None) -> dict:
    try:
        with open(cache_dir(root) / name, "rb") as stream:
            payload = pickle.load(stream)
        return payload if isinstance(payload, dict) else {}
    except (OSError, EOFError, pickle.UnpicklingError, AttributeError, ImportError,
            ValueError, TypeError, IndexError):
        return {}


def store(name: str, payload: dict, root: Path | None = None) -> None:
    """Atomically replace one cache file; a failed write only costs speed."""
    try:
        directory = cache_dir(root)
        directory.mkdir(parents=True, exist_ok=True)
        temporary = directory / f".{name}.{os.getpid()}.tmp"
        with open(temporary, "wb") as stream:
            pickle.dump(payload, stream, protocol=pickle.HIGHEST_PROTOCOL)
        os.replace(temporary, directory / name)
    except OSError:
        pass
