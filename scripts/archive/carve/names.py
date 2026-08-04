#!/usr/bin/env python3
"""homm3.carve.names - TEMPORARY bulk export of function-name candidates.

rva -> name/signature/file/line, drawn from the two external symbol sources:

  NH3API      wrapper headers embed retail VAs in call-macro bodies
              (`THISCALL_2(void, 0x58FA40, this, other)`); the enclosing
              declarator gives name+args, the enclosing class the scope.
              EXTERNAL AND UNVERIFIED - may describe HD Mod; rows are
              candidates, not the official map (CLAUDE.md evidence tiers).
  Dreamcast   the CodeView dump of the Dreamcast build: original qualified
              names, parameter names/types (S_REGREL32), return types
              (LF_MFUNCTION/LF_PROCEDURE), source FILE and LINE (SRCLINES),
              and the SH4 body size (Cb). No x86 addresses - joined to rvas
              by qualified-name agreement with an rva-anchored source.
  vc6-archive rva->symbol rows the DNA masked matcher already proved against
              retail bytes (evidence/retail-function-libraries.tsv) - the only
              rows here that are retail-proven; also joined to Dreamcast for
              file/line (the Dreamcast build linked the same zlib).

This is bulk extraction scaffolding: the CSV dissolves into the source tree
as real annotations later, and the script retires with the carve package.
"""
from __future__ import annotations

import csv
import re
import sys
from collections import defaultdict
from pathlib import Path

from homm3.carve import common

SYMBOLS_DIR = common.HOMM3_DIR.parent / "homm3-symbols"
DUMP = SYMBOLS_DIR / "HoMM3-Dreamcast-Dump/dump.txt"
NH3API_ROOT = SYMBOLS_DIR / "NH3API/nh3api"
OUT = common.EVIDENCE_DIR / "retail-function-names.csv"

PRIMITIVES = {
    "T_VOID": "void", "T_NOTYPE": "", "T_INT1": "signed char",
    "T_CHAR": "signed char", "T_UCHAR": "unsigned char", "T_RCHAR": "char",
    "T_WCHAR": "wchar_t", "T_SHORT": "short", "T_USHORT": "unsigned short",
    "T_INT2": "short", "T_UINT2": "unsigned short", "T_INT4": "int",
    "T_UINT4": "unsigned", "T_LONG": "long", "T_ULONG": "unsigned long",
    "T_INT8": "__int64", "T_UINT8": "unsigned __int64",
    "T_QUAD": "__int64", "T_UQUAD": "unsigned __int64",
    "T_REAL32": "float", "T_REAL64": "double", "T_REAL80": "long double",
    "T_BOOL08": "bool", "T_HRESULT": "HRESULT",
}


# --- Dreamcast CodeView dump ---------------------------------------------

class Dump:
    def __init__(self, text: str):
        self.types = self._parse_types(text)
        self.procs = self._parse_symbols(text)
        self._parse_srclines(text)

    RECORD = re.compile(r"^0x([0-9a-f]{4,}) : Length = \d+, Leaf = 0x[0-9a-f]+"
                        r" (LF_\w+)(.*)$")

    def _parse_types(self, text):
        types = {}
        start = text.index("*** GLOBAL TYPES")
        end = text.index("*** SYMBOLS")
        current = None
        for line in text[start:end].splitlines():
            m = self.RECORD.match(line)
            if m:
                current = {"kind": m.group(2), "body": [m.group(3)]}
                types[int(m.group(1), 16)] = current
            elif current is not None:
                current["body"].append(line)
        for record in types.values():
            record["body"] = "\n".join(record["body"])
        return types

    def render(self, code: str, depth=0) -> str:
        """Best-effort C rendering of a cvdump type reference."""
        code = code.strip()
        if depth > 6:
            return code
        m = re.match(r"T_32P(\w+)", code)
        if m and "T_" + m.group(1) in PRIMITIVES:
            return PRIMITIVES["T_" + m.group(1)] + "*"
        if code in PRIMITIVES:
            return PRIMITIVES[code]
        if not code.startswith("0x"):
            return code
        record = self.types.get(int(code, 16))
        if record is None:
            return code
        body, kind = record["body"], record["kind"]
        def ref(pattern):
            m = re.search(pattern, body)
            return m.group(1) if m else None
        if kind == "LF_POINTER":
            element = ref(r"Element type : (\S+?),?\s")
            element = element or ref(r"Element type : (\S+)$")
            return self.render(self._strip(element), depth + 1) + "*"
        if kind == "LF_MODIFIER":
            target = ref(r"modifies type (\S+)$") or ref(r"modifies type (\S+?),")
            return "const " + self.render(self._strip(target), depth + 1)
        if kind in ("LF_INTERFACE", "LF_CLASS", "LF_STRUCTURE"):
            return ref(r"class name = (\S+)") or code
        if kind == "LF_ENUM":
            return ref(r"enum name = (\S+)") or code
        if kind == "LF_UNION":
            return ref(r"union name = (\S+)") or code
        if kind == "LF_ARRAY":
            element = ref(r"Element type = (\S+)$") or ref(r"Element type = (\S+?),")
            return self.render(self._strip(element), depth + 1) + "[]"
        if kind in ("LF_PROCEDURE", "LF_MFUNCTION"):
            ret = ref(r"Return type = (\S+?),")
            return self.render(self._strip(ret), depth + 1) + " (*)()"
        return code

    @staticmethod
    def _strip(code):
        if code is None:
            return ""
        m = re.match(r"T_\w+", code)
        return m.group(0) if m else code.rstrip(",")

    PROC = re.compile(r"S_[GL]PROC32: \[0001:([0-9A-F]{8})\], "
                      r"Cb: ([0-9A-F]{8}), Type:\s+(\S+), (.+)$")
    REGREL = re.compile(r"S_REGREL32: \S+, Type:\s+([^,]+), (.+)$")

    def _parse_symbols(self, text):
        procs = []
        start = text.index("*** SYMBOLS")
        end = text.index("*** Compacted Global Symbols")
        module = "?"
        current = None
        for line in text[start:end].splitlines():
            if "S_OBJNAME" in line:
                module = line.rsplit("/", 1)[-1].rsplit("\\", 1)[-1].strip()
                continue
            m = self.PROC.search(line)
            if m:
                current = {"offset": int(m.group(1), 16),
                           "cb": int(m.group(2), 16),
                           "type": m.group(3), "name": m.group(4).strip(),
                           "module": module, "args": [], "file": "",
                           "line": ""}
                procs.append(current)
                continue
            if current is not None:
                if "S_ENDARG" in line:
                    current = None
                    continue
                a = self.REGREL.search(line)
                if a:
                    type_code = self._strip(a.group(1).strip())
                    current["args"].append(
                        (self.render(type_code), a.group(2).strip()))
        return procs

    FILE_RANGE = re.compile(r"^\s+(\S+), 0001:([0-9A-F]{8})-([0-9A-F]{8}), "
                            r"line/addr pairs = (\d+)")
    PAIRS = re.compile(r"(\d+)\s+([0-9A-F]{8})")

    def _parse_srclines(self, text):
        start = text.index("*** SRCLINES")
        end = text.index("*** SEGMENT MAP")
        by_addr = {}
        current_file = None
        for line in text[start:end].splitlines():
            m = self.FILE_RANGE.match(line)
            if m:
                current_file = m.group(1)
                continue
            if current_file and re.match(r"^\s+\d+ [0-9A-F]{8}", line):
                for number, addr in self.PAIRS.findall(line):
                    by_addr.setdefault(int(addr, 16),
                                       (current_file, int(number)))
        for proc in self.procs:
            hit = by_addr.get(proc["offset"])
            if hit:
                proc["file"], proc["line"] = hit[0], hit[1]

    def signature(self, proc) -> str:
        record = self.types.get(int(proc["type"], 16)) \
            if proc["type"].startswith("0x") else None
        ret = ""
        if record:
            m = re.search(r"Return type = (\S+?),", record["body"])
            if m:
                ret = self.render(self._strip(m.group(1)))
        args = ", ".join(f"{t} {n}".strip() for t, n in proc["args"]
                         if n != "this")
        return f"{ret} {proc['name']}({args})".strip()


# --- NH3API wrapper headers ----------------------------------------------

MACRO = re.compile(r"\b(THISCALL|FASTCALL|STDCALL|CDECL)_(\d+)\s*\(\s*"
                   r"([^,()]*(?:\([^()]*\))?[^,()]*)\s*,\s*(0x[0-9A-Fa-f]{6,8})")
CLASS = re.compile(r"^\s*(?:struct|class|NH3API_VIRTUAL_CLASS|"
                   r"NH3API_VIRTUAL_STRUCT)\s+(?:NH3API_\w+\s+)*"
                   r"([A-Za-z_]\w*)(?![\w<>]*;)")
DECL = re.compile(r"([A-Za-z_~][\w]*)\s*\(")
KEYWORDS = {"return", "if", "while", "for", "sizeof", "switch", "THISCALL",
            "FASTCALL", "STDCALL", "CDECL", "get_global_var_ref"}


def _declarator(lines, index):
    """Name + raw arg text of the function enclosing a wrapper macro line."""
    for back in range(0, 7):
        if index - back < 0:
            break
        line = lines[index - back]
        if back and line.lstrip().startswith((":", ",")):
            continue  # constructor member-initializer list, not the declarator
        candidates = [(m.start(), m.group(1)) for m in DECL.finditer(line)
                      if m.group(1) not in KEYWORDS
                      and not m.group(1).endswith("CALL")
                      and not re.match(r"^[A-Z0-9_]+$", m.group(1))]
        if back == 0 and candidates:
            macro_at = MACRO.search(line)
            candidates = [c for c in candidates
                          if macro_at is None or c[0] < macro_at.start()]
        if not candidates:
            continue
        at, name = candidates[-1]
        rest = line[at + len(name):]
        depth = 0
        args = ""
        for ch in rest:
            if ch == "(":
                depth += 1
                if depth == 1:
                    continue
            if ch == ")":
                depth -= 1
                if depth == 0:
                    return name, " ".join(args.split())
            if depth >= 1:
                args += ch
        return name, " ".join(args.split())
    return None, ""


def parse_nh3api():
    """Brace-depth class stack so nested `struct vftable_t` blocks resolve to
    their OUTER class (NH3API declares virtual wrappers inside them)."""
    rows = defaultdict(list)
    for path in sorted(NH3API_ROOT.rglob("*.hpp")):
        lines = path.read_text(errors="replace").splitlines()
        stack = []  # (class_name, depth at which its braces opened)
        depth = 0
        pending = None
        for index, line in enumerate(lines):
            cm = CLASS.match(line)
            if cm:
                pending = cm.group(1)
            scope = next((name for name, _d in reversed(stack)
                          if "vftable" not in name),
                         stack[-1][0] if stack else "")
            for m in MACRO.finditer(line):
                cc, _n, ret, address = m.groups()
                name, args = _declarator(lines, index)
                if name is None:
                    name, args = "?", ""
                rows[int(address, 16)].append({
                    "name": name, "args": args, "ret": " ".join(ret.split()),
                    "cc": cc.lower(), "class": scope,
                    "where": f"{path.relative_to(NH3API_ROOT)}:{index + 1}"})
            for ch in line:
                if ch == "{":
                    depth += 1
                    if pending is not None:
                        stack.append((pending, depth))
                        pending = None
                elif ch == "}":
                    if stack and stack[-1][1] == depth:
                        stack.pop()
                    depth = max(0, depth - 1)
            if pending is not None and line.rstrip().endswith(";"):
                pending = None  # forward declaration
    return rows


# --- join + export --------------------------------------------------------

def dc_index(dump):
    by_name = defaultdict(list)
    for proc in dump.procs:
        base = re.sub(r"@\d+$", "", proc["name"])
        by_name[base].append(proc)
    return by_name


def candidate_keys(entry):
    cls, name = entry["class"], entry["name"]
    keys = []
    if cls:
        if name == cls:
            keys.append(f"{cls}::{cls}")
        keys.append(f"{cls}::{name}")
    keys.append(name)
    return keys


def main(argv=None) -> int:
    functions = {int(r["rva"], 16): int(r["size"]) for r in common.read_tsv(
        common.need(common.CARVE_DIR / "functions.tsv", "audit"))}
    entries = sorted(functions)
    import bisect

    def state(rva):
        if rva in functions:
            return "entry"
        i = bisect.bisect_right(entries, rva) - 1
        if i >= 0 and entries[i] <= rva < entries[i] + functions[entries[i]]:
            return "interior"
        return "outside"

    if not DUMP.is_file() or not NH3API_ROOT.is_dir():
        common.die(f"symbol sources missing under {SYMBOLS_DIR}")
    print("[carve names] parsing Dreamcast CodeView dump ...", flush=True)
    dump = Dump(DUMP.read_text(errors="replace"))
    by_name = dc_index(dump)
    print(f"[carve names] {len(dump.procs)} Dreamcast procs, "
          f"{len(dump.types)} type records")
    print("[carve names] parsing NH3API wrappers ...", flush=True)
    nh3api = parse_nh3api()
    print(f"[carve names] {len(nh3api)} distinct NH3API-addressed calls")

    out_rows = []
    matched_dc = 0
    for va in sorted(nh3api):
        rva = va - common.IMAGE_BASE
        alternates = nh3api[va]
        chosen, proc = None, None
        for entry in alternates:
            for key in candidate_keys(entry):
                hits = by_name.get(key)
                if hits:
                    chosen, proc = entry, hits[0]
                    break
            if proc:
                break
        if chosen is None:
            chosen = alternates[0]
        qualified = (f"{chosen['class']}::{chosen['name']}"
                     if chosen["class"] and not chosen["name"].startswith(
                         chosen["class"]) else chosen["name"])
        if proc:
            matched_dc += 1
            name = proc["name"]
            signature = dump.signature(proc)
            source_file, line, dc_size = proc["file"], proc["line"], proc["cb"]
            sources = "nh3api+dreamcast"
        else:
            name = qualified
            signature = (f"{chosen['ret']} {qualified}({chosen['args']})"
                         .strip())
            source_file, line, dc_size = "", "", ""
            sources = "nh3api"
        out_rows.append({
            "rva": f"0x{rva:x}", "name": name, "signature": signature,
            "convention": chosen["cc"], "source_file": source_file,
            "line": line, "dc_size": dc_size, "carve_state": state(rva),
            "sources": sources, "evidence": chosen["where"],
            "notes": f"{len(alternates)} wrapper(s)"
                     + ("" if len(alternates) == 1 else " (alternates exist)"),
        })

    # retail-proven archive symbols, enriched with Dreamcast file/line
    lib_path = common.EVIDENCE_DIR / "retail-function-libraries.tsv"
    if lib_path.is_file():
        for r in common.read_tsv(lib_path):
            symbol = r["symbol"]
            if symbol in ("-", "?") or symbol.startswith("ambiguous") \
                    or symbol.startswith(".text"):
                continue
            base = re.sub(r"^[_@]", "", re.sub(r"@\d+$", "", symbol))
            proc = (by_name.get(base) or [None])[0]
            out_rows.append({
                "rva": r["rva"], "name": base if proc else symbol,
                "signature": dump.signature(proc) if proc else "",
                "convention": "", "source_file": proc["file"] if proc else "",
                "line": proc["line"] if proc else "",
                "dc_size": proc["cb"] if proc else "",
                "carve_state": state(int(r["rva"], 16)),
                "sources": f"{r['evidence']}" + ("+dreamcast" if proc else ""),
                "evidence": f"{r['library']}/{r['member']}",
                "notes": r["confidence"]})

    out_rows.sort(key=lambda row: int(row["rva"], 16))
    with OUT.open("w", newline="") as fh:
        fh.write("# GENERATED bulk export (temporary): python3 -m homm3.carve"
                 " names\n")
        for line_ in common.provenance("homm3.carve.names"):
            fh.write(line_ + "\n")
        fh.write("# NH3API rows are EXTERNAL-UNVERIFIED candidates (may "
                 "describe HD Mod);\n# Dreamcast fields are Dreamcast-build "
                 "evidence (SH4), joined by name;\n# only masked-archive/"
                 "masked-zlib-obj rows are retail-byte-proven.\n"
                 "# carve_state=interior means the address is NOT a function "
                 "entry in the\n# pinned image (byte-verified mid-instruction "
                 "in samples): such NH3API\n# addresses describe some other "
                 "pressing and must not name pinned-image\n# functions; only "
                 "carve_state=entry rows are usable name candidates.\n")
        writer = csv.DictWriter(fh, fieldnames=list(out_rows[0].keys()))
        writer.writeheader()
        writer.writerows(out_rows)

    states = defaultdict(int)
    for row in out_rows:
        states[row["carve_state"]] += 1
    print(f"[carve names] {len(out_rows)} rows -> {OUT} "
          f"({matched_dc} NH3API rows joined to Dreamcast; "
          f"states {dict(states)})")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
