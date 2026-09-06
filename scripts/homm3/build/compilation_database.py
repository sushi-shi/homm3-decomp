"""Generate clangd commands from the matching manifest and VC6 header mirror."""
from __future__ import annotations

import json
import os
import tempfile

from homm3 import manifest
from homm3.core import clang, common
from homm3.core.cc_wrap import ZLIB_INC


def commands(data, root, compiler, includes):
    rows = []
    for unit in data.get("unit", []):
        flags = data["flags"][unit["flags"]]
        # Preserve per-TU ABI, defines and language; VC6 code-generation
        # switches have no meaning for editor parsing.
        args = [compiler, "--driver-mode=cl", f"--target={clang.TARGET}",
                "-fms-compatibility", "-fms-extensions",
                f"-fms-compatibility-version={clang.MSC_VER}",
                "-Wno-everything"]
        for flag in flags:
            if flag == "/GX":
                args.append("/EHsc")
            elif flag in ("/Gr", "/Gd", "/Gz", "/TC", "/TP", "/MT", "/MD") \
                    or flag.startswith(("/D", "/U", "/I")):
                args.append(flag)
        for inc in includes:
            args.extend(["-imsvc", str(inc)])
        source = str(root / unit["source"])
        args.extend(["/c", source])
        rows.append({"directory": str(root), "file": source, "arguments": args})
    return rows


def refresh():
    root = common.HOMM3_DIR
    inc = clang.mirror() or clang.MIRROR
    rows = commands(manifest.load(root / "config/units.toml"), root,
                    clang.clang_bin() or "clang", [inc, root / "include", root / ZLIB_INC])
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
