"""Pin and stage user-supplied CodeWarrior Pro 6 tools."""
from __future__ import annotations

import hashlib
import os
from pathlib import Path
import tempfile
import tomllib

from homm3.core import common


class ToolchainError(ValueError):
    pass


ROOT = common.HOMM3_DIR
DESTINATION = ROOT / "build/mac/toolchain"


def specification() -> dict:
    with (ROOT / "config/mac/toolchain.toml").open("rb") as stream:
        return tomllib.load(stream)


def stage(source: str | Path | None = None) -> Path:
    """Explicit path > env path > already staged copy; never search siblings."""
    supplied = source if source is not None else os.environ.get("HOMM3_MAC_TOOLCHAIN")
    origin = Path(supplied).expanduser().resolve() if supplied else DESTINATION
    hashes = specification()["files"]
    for name, expected in hashes.items():
        path = origin / name
        try:
            data = path.read_bytes()
        except OSError as exc:
            raise ToolchainError(f"cannot read {path}; set HOMM3_MAC_TOOLCHAIN or run `homm3 init --mac-toolchain DIR`: {exc}") from exc
        actual = hashlib.sha256(data).hexdigest()
        if actual != expected:
            raise ToolchainError(f"{path}: sha256 {actual} != pinned {expected}")
        target = DESTINATION / name
        if path != target:
            target.parent.mkdir(parents=True, exist_ok=True)
            if target.is_file() and target.read_bytes() == data:
                continue
            temporary = None
            try:
                with tempfile.NamedTemporaryFile(dir=target.parent, delete=False) as stream:
                    temporary = Path(stream.name)
                    stream.write(data)
                temporary.replace(target)
            finally:
                if temporary is not None:
                    temporary.unlink(missing_ok=True)
    return DESTINATION
