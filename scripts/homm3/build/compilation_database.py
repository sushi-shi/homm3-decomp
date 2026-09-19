"""Generate clangd commands from the matching manifest and VC6 header mirror."""
from __future__ import annotations

import json
import os
import tempfile

from homm3 import manifest
from homm3.core import clang, common
from homm3.core import compiler_profile
from homm3.core.project import Project


def commands(data, root, compiler, includes):
    rows = []
    for unit in data.get("unit", []):
        flags = data["flags"][unit["flags"]]
        args = [compiler, *compiler_profile.arguments(flags, includes, root=root)]
        source = str(root / unit["source"])
        args.extend(["/c", source])
        rows.append({"directory": str(root), "file": source, "arguments": args})
    return rows


def refresh():
    root = common.HOMM3_DIR
    project = Project(root)
    inc = clang.mirror(root, project.toolchain) or root / "build/gen/msvc-include"
    rows = commands(manifest.load(root / "config/units.toml"), root,
                    clang.clang_bin() or "clang", [inc, *project.includes])
    dest = root / "compile_commands.json"
    text = json.dumps(rows, indent=2) + "\n"
    if dest.is_file() and dest.read_text() == text:
        return
    fd, name = tempfile.mkstemp(prefix=".compile_commands-", dir=root)
    try:
        with os.fdopen(fd, "w") as stream:
            stream.write(text)
        os.replace(name, dest)
    finally:
        if os.path.exists(name):
            os.unlink(name)


if __name__ == "__main__":
    refresh()
