"""Census of the Loki h3maped image: compiled objects and their functions.

Writes the retail facts under config/retail/h3maped-loki/:

  objects.tsv    one row per compiled object (its .eh_frame CIE, in link
                 order), its .text span, function counts, kind and file name
                 with the evidence that names it.
  functions.tsv  every function start: frame-described functions (FDEs,
                 including file-static ones) and exported functions of the
                 frameless C libraries, with object, placement, binding,
                 GNU v2 mangled name and its demangling.

`--check` regenerates in memory and fails when the committed tables differ.
"""
from __future__ import annotations

import argparse
import collections
import re
import struct
import subprocess
import sys

from homm3.core import common
from homm3.loki import toolchain
from homm3.loki.image import IMAGE, LokiImage

RETAIL = common.HOMM3_DIR / "config/retail" / IMAGE
SOURCE_SUFFIX = re.compile(rb"[A-Za-z_][\w .+-]*\.(?:cpp|cc|c)\0")


def demangle(names: list[str]) -> list[str]:
    """GNU v2 demangling with the era's binutils c++filt (modern c++filt dropped the ABI)."""
    if not names:
        return []
    root = toolchain.SYSROOT
    completed = subprocess.run(
        [str(toolchain.LOADER), "--library-path", f"{root}/lib:{root}/usr/lib", str(root / "usr/bin/c++filt")],
        input="\n".join(names) + "\n", capture_output=True, text=True, env=toolchain.environment(), check=True)
    out = completed.stdout.split("\n")[:len(names)]
    if len(out) != len(names):
        raise RuntimeError("c++filt returned a different number of lines")
    return out


def _file_strings(image: LokiImage) -> dict[int, str]:
    rodata = image.elf.section(".rodata")
    data = image.elf.bytes(rodata)
    out = {}
    for match in SOURCE_SUFFIX.finditer(data):
        start = data.rfind(b"\0", 0, match.start()) + 1
        name = data[start:match.end() - 1].decode("latin-1")
        if re.fullmatch(r"[A-Za-z_][\w .+-]*\.(cpp|cc|c)", name):
            out[rodata.addr + start] = name
    return out


def objects(image: LokiImage) -> list[dict]:
    functions = [f for f in image.functions if f.obj >= 0]
    by_object: dict[int, list] = collections.defaultdict(list)
    for function in functions:
        by_object[function.obj].append(function)
    files = _file_strings(image)
    text = image.elf.section(".text")
    code = image.elf.bytes(text)
    file_refs: dict[int, collections.Counter] = collections.defaultdict(collections.Counter)
    for offset in range(len(code) - 3):
        (value,) = struct.unpack_from("<I", code, offset)
        if value in files:
            owner = image.function_at(text.addr + offset)
            if owner is not None and owner.obj >= 0:
                file_refs[owner.obj][files[value]] += 1
    anonymous: dict[int, collections.Counter] = collections.defaultdict(collections.Counter)
    classes: dict[int, collections.Counter] = collections.defaultdict(collections.Counter)
    for function in functions:
        match = re.search(r"_GLOBAL_\.N\.(.+?\.(?:cpp|cc|c))", function.name)
        if match:
            anonymous[function.obj][match.group(1)] += 1
        match = re.match(r"(?:_\._|__|[A-Za-z_]\w*?__)(Q\d)?(\d+)([A-Za-z_]\w*)", function.name)
        if function.name and not function.linkonce and match and match.group(1) is None:
            length = int(match.group(2))
            classes[function.obj][match.group(3)[:length]] += 1
    rows = []
    linkonce_start = image.linkonce_start
    for index in sorted(by_object):
        members = by_object[index]
        own = [f for f in members if not f.linkonce]
        kept = [f for f in members if f.linkonce]
        start = min((f.address for f in own), default=0)
        end = max((f.address + f.size for f in own), default=0)
        name, evidence = "", ""
        if file_refs[index]:
            name, count = file_refs[index].most_common(1)[0]
            if not name.endswith(".c"):
                evidence = f"__FILE__ x{count}"
            else:
                name = ""
        if not name and anonymous[index]:
            name, count = anonymous[index].most_common(1)[0]
            evidence = f"anonymous namespace x{count}"
        if not name and classes[index]:
            top, count = classes[index].most_common(1)[0]
            evidence = f"class {top} x{count} (file name inferred)"
        if start >= linkonce_start or (start and start > image.c_library_end):
            kind = "libstdc++"
        else:
            kind = "project"
        rows.append(dict(object=index, kind=kind, text_start=start, text_end=end, functions=len(own),
                         static=sum(1 for f in own if f.bind == "static"), linkonce=len(kept),
                         file=name, evidence=evidence))
    return rows


def function_rows(image: LokiImage) -> list[dict]:
    names = demangle([f.name for f in image.functions if f.name])
    demangled = dict(zip([f.name for f in image.functions if f.name], names))
    rows = []
    for function in image.functions:
        placement = "linkonce" if function.linkonce else "text"
        rows.append(dict(address=function.address, size=function.size,
                         object=function.obj, placement=placement, bind=function.bind,
                         name=function.name, demangled=demangled.get(function.name, "")))
    return rows


def render(image: LokiImage) -> dict[str, str]:
    sha = image_sha()
    header = f"# Loki h3maped census (homm3 loki census); image sha256 {sha}\n"
    objs = objects(image)
    lines = [header + "# object\tkind\ttext_start\ttext_end\tfunctions\tstatic\tlinkonce\tfile\tevidence"]
    for row in objs:
        lines.append(f"{row['object']}\t{row['kind']}\t{row['text_start']:08x}\t{row['text_end']:08x}\t"
                     f"{row['functions']}\t{row['static']}\t{row['linkonce']}\t{row['file']}\t{row['evidence']}")
    funcs = [header + "# address\tsize\tobject\tplacement\tbind\tmangled\tdemangled"]
    for row in function_rows(image):
        funcs.append(f"{row['address']:08x}\t{row['size']}\t{row['object']}\t{row['placement']}\t"
                     f"{row['bind']}\t{row['name']}\t{row['demangled']}")
    return {"objects.tsv": "\n".join(lines) + "\n", "functions.tsv": "\n".join(funcs) + "\n"}


def image_sha() -> str:
    from homm3.loki.image import executable
    return executable().sha256


def main(argv=None) -> int:
    parser = argparse.ArgumentParser(prog="homm3 loki census", description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--check", action="store_true", help="fail if the committed tables are stale")
    args = parser.parse_args(argv)
    toolchain.stage()
    tables = render(LokiImage())
    stale = []
    for name, text in tables.items():
        path = RETAIL / name
        if args.check:
            if not path.is_file() or path.read_text() != text:
                stale.append(name)
        else:
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_text(text)
            print(f"[loki] wrote {path.relative_to(common.HOMM3_DIR)} ({text.count(chr(10)) - 2} rows)")
    if stale:
        print(f"[loki] stale census tables: {', '.join(stale)}; run `homm3 loki census`", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
