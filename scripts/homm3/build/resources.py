"""homm3.build.resources - compile src/heroes3.rc into the game's .res.

    python3 -m homm3.build.resources [--out build/exe/heroes3.res]

The pinned VC6 SP3 RC.EXE compiles the resource script; every payload of
the produced .res (type, name, language, bytes and order) is then compared
with the retail `.rsrc` leaves in both directions, and any difference fails
and removes the .res. Retail payload order is the data order in `.rsrc`,
which LINK and CVTRES take from the .res record order, so the script's
statement order is retail's.

The program icon is the one binary resource. The script is compiled from a
temporary stage beside a copy of it, `heroes3.ico`, rebuilt as an ordinary
.ico container from the user's retail RT_ICON payloads and RT_GROUP_ICON
directory; it never becomes a repository input. LINK converts the .res with
the pinned CVTRES 5.00.1736, whose objects carry the @comp.id 6/1735 that
retail's Rich header records.
"""
from __future__ import annotations

import argparse
import shutil
import struct
import sys
import tempfile
from pathlib import Path

from homm3.core import common
from homm3.core.images import path as _image_path

ROOT = common.HOMM3_DIR
SCRIPT = ROOT / "src/heroes3.rc"
OUT = ROOT / _image_path("build/exe/heroes3.res")
ICON = "heroes3.ico"
RT_ICON, RT_GROUP_ICON = 3, 14


def _align4(value: int) -> int:
    return (value + 3) & ~3


def read_res(blob: bytes) -> list[dict]:
    """RES32 records in file order, without the leading null record."""
    def identifier(offset: int):
        if struct.unpack_from("<H", blob, offset)[0] == 0xFFFF:
            return struct.unpack_from("<H", blob, offset + 2)[0], offset + 4
        end = offset
        while blob[end:end + 2] != b"\0\0":
            end += 2
        return blob[offset:end].decode("utf-16le"), end + 2

    records, offset = [], 0
    while offset + 8 <= len(blob):
        size, header = struct.unpack_from("<II", blob, offset)
        rtype, cursor = identifier(offset + 8)
        name, cursor = identifier(cursor)
        language = struct.unpack_from("<IHH", blob, _align4(cursor))[2]
        data = blob[offset + header:offset + header + size]
        if not (rtype == 0 and name == 0):
            records.append({"type": rtype, "name": name, "language": language, "data": data})
        offset = _align4(offset + header + size)
    return records


def retail_resources(data: bytes) -> list[dict]:
    """The PE resource leaves of an image, in payload (data RVA) order."""
    from homm3.verify.link_diff import Pe
    pe = Pe.read(data)
    section = pe.section(".rsrc")
    tree = data[section[3]:section[3] + section[4]]

    def name(value: int):
        if not value & 0x80000000:
            return value
        at = value & 0x7FFFFFFF
        length = struct.unpack_from("<H", tree, at)[0]
        return tree[at + 2:at + 2 + 2 * length].decode("utf-16le")

    leaves = []

    def walk(at: int, path: list) -> None:
        named, ids = struct.unpack_from("<HH", tree, at + 12)
        for index in range(named + ids):
            key, target = struct.unpack_from("<II", tree, at + 16 + 8 * index)
            if target & 0x80000000:
                walk(target & 0x7FFFFFFF, path + [name(key)])
                continue
            rva, size = struct.unpack_from("<II", tree, target)
            rtype, rname = path
            leaves.append({"type": rtype, "name": rname, "language": name(key),
                           "rva": rva, "data": pe.read_at(rva, size)})

    walk(0, [])
    return sorted(leaves, key=lambda leaf: leaf["rva"])


def icon_container(retail: list[dict]) -> bytes:
    """The .ico RC splits back into the retail RT_ICON + RT_GROUP_ICON."""
    groups = [r for r in retail if r["type"] == RT_GROUP_ICON]
    if len(groups) != 1:
        raise ValueError(f"expected one retail RT_GROUP_ICON, found {len(groups)}")
    group = groups[0]["data"]
    reserved, kind, count = struct.unpack_from("<HHH", group, 0)
    if (reserved, kind) != (0, 1) or not count or len(group) != 6 + 14 * count:
        raise ValueError(f"unexpected retail RT_GROUP_ICON directory {group.hex()}")
    entries, images = [], []
    offset = 6 + 16 * count
    for index in range(count):
        width, height, colors, flags, planes, bits, size, ordinal = struct.unpack_from(
            "<BBBBHHIH", group, 6 + 14 * index)
        image = next((r["data"] for r in retail
                      if r["type"] == RT_ICON and r["name"] == ordinal), None)
        if image is None or len(image) != size:
            raise ValueError(f"retail RT_ICON {ordinal} does not match its group entry")
        entries.append(struct.pack("<BBBBHHII", width, height, colors, flags, planes,
                                   bits, size, offset))
        images.append(image)
        offset += size
    return struct.pack("<HHH", 0, 1, count) + b"".join(entries) + b"".join(images)


def compare(ours: list[dict], retail: list[dict]) -> list[str]:
    """Every payload mismatch, both directions; empty when exact."""
    problems = []
    if len(ours) != len(retail):
        problems.append(f"payload count {len(ours)} != retail {len(retail)}")
    for index, (a, b) in enumerate(zip(ours, retail)):
        mine = (a["type"], a["name"], a["language"])
        theirs = (b["type"], b["name"], b["language"])
        if mine != theirs:
            problems.append(f"[{index}] identity {mine} != retail {theirs}")
        elif a["data"] != b["data"]:
            first = next((i for i, (x, y) in enumerate(zip(a["data"], b["data"]))
                          if x != y), min(len(a["data"]), len(b["data"])))
            problems.append(f"[{index}] {mine}: bytes differ (sizes "
                            f"{len(a['data'])}/{len(b['data'])}, first at {first:#x})")
    return problems


def compile_resources(script: Path = SCRIPT, out: Path = OUT) -> Path:
    """Compile and gate the script; returns the .res path."""
    from homm3.build.link import run_wine
    from homm3.core.cc_wrap import ensure_wineserver, find_ci, msvc_dir, winepath_w
    from homm3.verify.link_diff import link_view, read_edits
    rc = find_ci(msvc_dir() / "bin", "rc.exe")
    if rc is None:
        raise ValueError(f"no RC.EXE under {msvc_dir()}/bin")
    retail, _findings = link_view(bytes(common.load_image()[0].data), read_edits())
    leaves = retail_resources(retail)
    out = Path(out)
    out.parent.mkdir(parents=True, exist_ok=True)
    out.unlink(missing_ok=True)
    ensure_wineserver()
    with tempfile.TemporaryDirectory(prefix=".rc-", dir=out.parent) as stage:
        staged = Path(stage) / script.name
        shutil.copyfile(script, staged)
        (Path(stage) / ICON).write_bytes(icon_container(leaves))
        include = winepath_w(msvc_dir() / "include")
        output, code = run_wine(["wine", str(rc), "/r", f"/i{include}",
                                 f"/fo{winepath_w(out)}", staged.name], stage)
    if code or not out.is_file():
        tail = "\n".join(output.strip().splitlines()[-12:])
        raise ValueError(f"RC.EXE produced no .res (exit {code}):\n{tail}")
    problems = compare(read_res(out.read_bytes()), leaves)
    if problems:
        out.unlink(missing_ok=True)
        raise ValueError(f"{script.name} differs from the retail resources:\n  "
                         + "\n  ".join(problems))
    return out


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--src", type=Path, default=SCRIPT)
    parser.add_argument("--out", type=Path, default=OUT)
    args = parser.parse_args(argv)
    try:
        out = compile_resources(args.src, args.out)
    except (ValueError, OSError) as exc:
        print(f"[rc] {exc}", file=sys.stderr)
        return 1
    print(f"[rc] {args.src.name} -> {out} (every payload equals retail)")
    return 0


def logged_main(argv: list[str] | None = None) -> int:
    import shlex
    from homm3.core.usage import append, run_logged
    argv = list(sys.argv[1:] if argv is None else argv)
    command = shlex.join(["python3", "-m", "homm3.build.resources", *argv])
    return run_logged(main, argv, lambda rc, **meta: append(
        ROOT / "build/homm3_usage.log", command, rc, **meta), failure_rc=1)


if __name__ == "__main__":
    sys.exit(logged_main())
