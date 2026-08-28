#!/usr/bin/env python3
"""Read-only Dreamcast source-shape navigation (`homm3 dreamcast`).

The Dreamcast executable is an older RoE pressing built for WinCE/SH4.  Its
NB11 CodeView stream preserves names, source lines, lexical blocks and a
lower-bound local inventory that retail Complete's optimized, stripped x86
image does not.  This command joins those records into one function dossier.

Subcommands
-----------
  show SELECTOR [--json]
        Function identity, signature/retail bridges, parameters and locals,
        lexical scopes, and a statement-ordered call/branch listing.

        SELECTOR is one of:
          0x00403ee0                 retail VA (or 0x3ee0 retail RVA)
          dc:0x1190                  Dreamcast .text offset
          adventuremapwindow.obj:0x1190
          TAdventureMapWindow::ClearBottomView

  asm SELECTOR [--blocks] [--no-breakpoints] [--json]
        SH4 assembly labelled with CodeView breakpoint/source rows. --blocks
        adds inferred CFG predecessor/successor headers; lexical S_BLOCK32
        scopes remain separately labelled.

  find TEXT [--module MODULE] [--limit N] [--json]
        Search the complete Dreamcast procedure roster.  Exact names are not
        required; boundaries, module, source location and any retail bridge
        are shown.

  stats [--json]
        Corpus coverage and bridge counts.

All results are ANALYSIS OUTPUT about another pressing.  A Dreamcast address,
line or call is never retail address/byte evidence; retail promotion still
requires corroboration from the pinned Complete executable.
"""
from __future__ import annotations

import argparse
import csv
import json
import re
import shlex
import sys
from collections import Counter, defaultdict
from dataclasses import dataclass
from pathlib import Path
from typing import Any, Iterable, TextIO

from homm3.analysis import dc_asm, dc_lines, dc_srclines
from homm3.core import common


FUNCTIONS = common.EVIDENCE_DIR / "dreamcast/functions.csv"
VARIABLES = common.EVIDENCE_DIR / "dreamcast/variables.csv"
BRIDGES = common.EVIDENCE_DIR / "retail-dc-name-map.csv"
SRC_DIR = common.HOMM3_DIR / "src"
LOG = common.HOMM3_DIR / "build/homm3_dreamcast.log"

DC_EXE_SHA256 = \
    "cdbc7e75bd7d057171fa12b728aaaee01c1db133fff350b034950dd21dd07736"
DC_EXE_SIZE = 8425752
AUTHORITY = "Dreamcast RoE/WinCE reference; analysis output, not retail evidence"


class DreamcastError(ValueError):
    pass


@dataclass(frozen=True)
class Claim:
    va: int
    module: str
    dc_offset: int
    path: str
    line: int


def _csv_rows(path: Path) -> list[dict[str, str]]:
    if not path.is_file():
        raise DreamcastError(f"missing corpus file: {path}")
    with path.open(newline="") as fh:
        return list(csv.DictReader(line for line in fh
                                   if not line.startswith("#")))


def _integer(value: str | int) -> int:
    return value if isinstance(value, int) else int(value, 0)


def _basename(path: str) -> str:
    return path.rsplit("\\", 1)[-1].rsplit("/", 1)[-1]


def _source_claims(src_dir: Path = SRC_DIR) -> list[Claim]:
    claims: list[Claim] = []
    for path in sorted(src_dir.glob("*.cpp")):
        text = path.read_text(errors="replace")
        module = path.stem + ".obj"
        for match in dc_srclines.CLAIM_RE.finditer(text):
            claims.append(Claim(
                va=int(match.group(1), 16), module=module,
                dc_offset=int(match.group(2), 16),
                path=str(path.relative_to(common.HOMM3_DIR)),
                line=text.count("\n", 0, match.start()) + 1))
    return claims


class Corpus:
    """Materialized CSV indexes.  Optional rows make resolution hermetic in tests."""

    def __init__(self, *, functions: list[dict[str, str]] | None = None,
                 variables: list[dict[str, str]] | None = None,
                 bridges: list[dict[str, str]] | None = None,
                 claims: list[Claim] | None = None):
        self.functions = _csv_rows(FUNCTIONS) if functions is None else functions
        self.variables = _csv_rows(VARIABLES) if variables is None else variables
        self.bridges = _csv_rows(BRIDGES) if bridges is None else bridges
        self.claims = _source_claims() if claims is None else claims

        self.by_key: dict[tuple[str, int], dict[str, str]] = {}
        self.by_offset: dict[int, list[dict[str, str]]] = defaultdict(list)
        self.by_name: dict[str, list[dict[str, str]]] = defaultdict(list)
        for row in self.functions:
            key = (row["module"], _integer(row["offset"]))
            self.by_key[key] = row
            self.by_offset[key[1]].append(row)
            self.by_name[row["name"]].append(row)

        self.variables_by_key: dict[tuple[str, str], list[dict[str, str]]] = \
            defaultdict(list)
        for row in self.variables:
            self.variables_by_key[(row["module"], row["proc"])].append(row)

        self.bridges_by_key: dict[tuple[str, int], list[dict[str, str]]] = \
            defaultdict(list)
        self.bridges_by_rva: dict[int, list[dict[str, str]]] = defaultdict(list)
        for row in self.bridges:
            key = (row["dc_module"], _integer(row["dc_offset"]))
            self.bridges_by_key[key].append(row)
            self.bridges_by_rva[_integer(row["rva"])].append(row)

        self.claims_by_key: dict[tuple[str, int], list[Claim]] = defaultdict(list)
        self.claims_by_va: dict[int, list[Claim]] = defaultdict(list)
        for claim in self.claims:
            self.claims_by_key[(claim.module, claim.dc_offset)].append(claim)
            self.claims_by_va[claim.va].append(claim)

    @staticmethod
    def key(row: dict[str, str]) -> tuple[str, int]:
        return row["module"], _integer(row["offset"])

    def _one(self, selector: str, rows: Iterable[dict[str, str]]) \
            -> dict[str, str]:
        unique = {self.key(row): row for row in rows}
        if not unique:
            raise DreamcastError(f"{selector}: no Dreamcast procedure matches")
        if len(unique) != 1:
            shown = sorted(unique.values(), key=lambda r: (r["module"],
                                                            _integer(r["offset"])))
            detail = ", ".join(
                f"{r['module']}:{r['offset']} {r['name']}" for r in shown[:8])
            more = f" (+{len(shown) - 8} more)" if len(shown) > 8 else ""
            raise DreamcastError(f"{selector}: ambiguous: {detail}{more}")
        return next(iter(unique.values()))

    def resolve(self, selector: str) -> dict[str, str]:
        """Resolve retail address, explicit DC address, module:offset or name."""
        if selector.lower().startswith("dc:"):
            try:
                off = _integer(selector.split(":", 1)[1])
            except ValueError as exc:
                raise DreamcastError(f"{selector}: invalid DC offset") from exc
            return self._one(selector, self.by_offset.get(off, ()))

        module_match = re.fullmatch(r"([^:]+\.obj):(0x[0-9a-fA-F]+)", selector)
        if module_match:
            key = (module_match.group(1), int(module_match.group(2), 16))
            row = self.by_key.get(key)
            if row is None:
                raise DreamcastError(f"{selector}: no Dreamcast roster row")
            return row

        if re.fullmatch(r"0x[0-9a-fA-F]+", selector):
            value = int(selector, 16)
            va = value if value >= common.IMAGE_BASE else value + common.IMAGE_BASE
            rva = va - common.IMAGE_BASE
            rows: list[dict[str, str]] = []
            for claim in self.claims_by_va.get(va, ()):
                row = self.by_key.get((claim.module, claim.dc_offset))
                if row is not None:
                    rows.append(row)
            for bridge in self.bridges_by_rva.get(rva, ()):
                row = self.by_key.get((bridge["dc_module"],
                                       _integer(bridge["dc_offset"])))
                if row is not None:
                    rows.append(row)
            if not rows:
                raise DreamcastError(
                    f"{selector}: no source claim or retail/DC bridge; use "
                    "dc:0xOFF for a Dreamcast offset")
            return self._one(selector, rows)

        exact = self.by_name.get(selector, ())
        if exact:
            return self._one(selector, exact)
        folded = [row for name, rows in self.by_name.items()
                  if name.lower() == selector.lower() for row in rows]
        if folded:
            return self._one(selector, folded)
        needle = selector.lower()
        return self._one(selector, (row for row in self.functions
                                    if needle in row["name"].lower()))

    def find(self, pattern: str, module: str | None = None) \
            -> list[dict[str, str]]:
        needle = pattern.lower()
        if module and not module.endswith(".obj"):
            module += ".obj"
        rows = [row for row in self.functions
                if needle in row["name"].lower()
                and (module is None or row["module"] == module)]
        return sorted(rows, key=lambda r: (r["module"], _integer(r["offset"]),
                                           r["name"]))


def _gate_dc_exe(path: Path) -> bytes:
    if not path.is_file():
        raise DreamcastError(f"Dreamcast executable not found: {path}")
    size = path.stat().st_size
    if size != DC_EXE_SIZE:
        raise DreamcastError(
            f"{path}: size {size} != pinned Dreamcast size {DC_EXE_SIZE}")
    digest = common.sha256_of(path)
    if digest != DC_EXE_SHA256:
        raise DreamcastError(
            f"{path}: sha256 {digest} != pinned Dreamcast {DC_EXE_SHA256}")
    return path.read_bytes()


def _top_level_scopes(name: str) -> list[str]:
    parts: list[str] = []
    depth = start = index = 0
    while index < len(name):
        char = name[index]
        if char == "<":
            depth += 1
        elif char == ">" and depth:
            depth -= 1
        elif char == ":" and index + 1 < len(name) \
                and name[index + 1] == ":" and depth == 0:
            parts.append(name[start:index])
            start = index + 2
            index += 1
        index += 1
    parts.append(name[start:])
    return parts


def _call_kind(name: str | None, size: int | None) -> str | None:
    if not name:
        return None
    if name.startswith("??0"):
        return "constructor"
    if name.startswith(("??1", "??_E", "??_G")):
        return "destructor"
    scopes = _top_level_scopes(name)
    if scopes[-1].startswith("~"):
        return "destructor"
    if len(scopes) >= 2:
        owner = scopes[-2].split("<", 1)[0]
        method = scopes[-1].split("<", 1)[0]
        if method == owner:
            return "constructor"
    if size is not None and size <= 16:
        return "tiny-helper"
    return "call"


def _retail_bridges(corpus: Corpus, key: tuple[str, int]) -> list[dict[str, Any]]:
    out = []
    for row in corpus.bridges_by_key.get(key, ()):
        rva = _integer(row["rva"])
        out.append({
            "va": common.IMAGE_BASE + rva,
            "rva": rva,
            "size": _integer(row["size"]),
            "role": row["role"],
            "name": row["name"],
            "signature": row["signature"],
            "source": row["source"],
        })
    return out


def build_dossier(corpus: Corpus, row: dict[str, str]) -> dict[str, Any]:
    if not dc_lines.DUMP.is_file():
        raise DreamcastError(f"Dreamcast CodeView dump not found: {dc_lines.DUMP}")
    off, cb = _integer(row["offset"]), _integer(row["cb"])
    key = corpus.key(row)
    dump = dc_lines._dump_lines()
    proc = dc_lines.find_proc(dump, off)
    if proc is None:
        raise DreamcastError(f"dc {off:#x}: no S_GPROC32/S_LPROC32 record")
    proc_name, proc_cb, _raw_locals, blocks = proc
    if proc_cb != cb or proc_name != row["name"]:
        raise DreamcastError(
            f"dc {off:#x}: CSV/dump disagreement ({row['name']} {cb} B vs "
            f"{proc_name} {proc_cb} B)")

    data = _gate_dc_exe(dc_lines.EXE)
    symbols = dc_lines.symbol_map(dump)
    statements = dc_lines.line_table(dump, off, cb)
    bounds = [addr for addr, _line, _file in statements] + [off + cb]
    try:
        asm_view = dc_asm.build_view(row, dump, data)
    except dc_asm.AsmError as exc:
        raise DreamcastError(str(exc)) from exc
    control = dc_asm.control_events(asm_view, data)

    call_rows: list[dict[str, Any]] = []
    statement_rows: list[dict[str, Any]] = []
    for index, (addr, line, source) in enumerate(statements):
        end = bounds[index + 1]
        statement_events = [event for site, event in control.items()
                            if addr <= site < end]
        calls = [event["call_target_va"] for event in statement_events
                 if "call_target_va" in event]
        branches = sum(event.get("conditional_branch", False)
                       for event in statement_events)
        rendered_calls = []
        for target in calls:
            name = symbols.get(target) if target is not None else None
            dc_target = None
            target_row = None
            if target is not None and dc_lines.POOL_BASE <= target:
                candidate = target - dc_lines.POOL_BASE
                hits = corpus.by_offset.get(candidate, ())
                if len(hits) == 1:
                    dc_target, target_row = candidate, hits[0]
                    # S_GPROC32 carries the human procedure name while a
                    # colliding S_PUB32 record may carry only its decoration.
                    # Prefer the typed roster identity for helper/RAII reads.
                    name = target_row["name"]
            target_size = _integer(target_row["cb"]) if target_row else None
            call = {
                "target_va": target,
                "dc_offset": dc_target,
                "name": name,
                "dc_size": target_size,
                "kind": _call_kind(name, target_size),
            }
            rendered_calls.append(call)
            call_rows.append({"statement": index, "line": line, **call})
        statement_rows.append({
            "address": addr,
            "size": end - addr,
            "line": line,
            "source": source,
            "branches": branches,
            "scope_opens": sum(1 for start, _size in blocks if start == addr),
            "scope_closes": sum(1 for start, size in blocks
                                if start + size == addr),
            "calls": rendered_calls,
        })

    variables = corpus.variables_by_key.get((row["module"], row["name"]), ())
    params = [dict(item) for item in variables if item["kind"] == "param"]
    locals_ = [dict(item) for item in variables if item["kind"] != "param"]
    bridges = _retail_bridges(corpus, key)
    claims = [{"va": claim.va, "path": claim.path, "line": claim.line}
              for claim in corpus.claims_by_key.get(key, ())]

    return {
        "authority": AUTHORITY,
        "function": {
            "name": row["name"], "module": row["module"],
            "kind": row["kind"], "dc_offset": off, "dc_size": cb,
            "dc_end": off + cb, "source": row["file"],
            "declaration_line": _integer(row["line"]),
            "debug_start": _integer(row["debug_start"]),
            "debug_end": _integer(row["debug_end"]),
        },
        "signatures": sorted({bridge["signature"] for bridge in bridges
                              if bridge["signature"]}),
        "retail_bridges": bridges,
        "retail_source_claims": claims,
        "parameters": params,
        "locals": locals_,
        "scopes": [{"start": start, "end": start + size, "size": size}
                   for start, size in blocks],
        "statements": statement_rows,
        "summary": {
            "statement_rows": len(statement_rows),
            "distinct_source_lines": len({(s["source"], s["line"])
                                           for s in statement_rows}),
            "conditional_branches": sum(s["branches"] for s in statement_rows),
            "calls": len(call_rows),
            "unique_named_callees": len({c["name"] for c in call_rows
                                          if c["name"]}),
            "constructor_calls": sum(c["kind"] == "constructor"
                                     for c in call_rows),
            "destructor_calls": sum(c["kind"] == "destructor"
                                    for c in call_rows),
            "tiny_helper_calls": sum(c["kind"] == "tiny-helper"
                                     for c in call_rows),
        },
    }


def _hex(value: int | None, width: int = 0) -> str:
    if value is None:
        return "<indirect>"
    return f"0x{value:0{width}x}" if width else f"{value:#x}"


def render_dossier(dossier: dict[str, Any], out: TextIO = sys.stdout) -> None:
    fn = dossier["function"]
    print("DREAMCAST REFERENCE — ANALYSIS OUTPUT, NOT RETAIL EVIDENCE", file=out)
    print(file=out)
    print(fn["name"], file=out)
    print(f"  module       {fn['module']} ({fn['kind']})", file=out)
    print(f"  source       {fn['source']}:{fn['declaration_line']}", file=out)
    print(f"  dc text      {_hex(fn['dc_offset'], 8)}..{_hex(fn['dc_end'], 8)} "
          f"({fn['dc_size']} B SH4)", file=out)
    print(f"  debug body   +{_hex(fn['debug_start'])}..+{_hex(fn['debug_end'])}",
          file=out)
    for signature in dossier["signatures"]:
        print(f"  signature    {signature}", file=out)

    print("\nRetail bridge(s) — correlation only:", file=out)
    if not dossier["retail_bridges"] and not dossier["retail_source_claims"]:
        print("  none", file=out)
    for bridge in dossier["retail_bridges"]:
        print(f"  VA {_hex(bridge['va'], 8)}  RVA {_hex(bridge['rva'], 8)}  "
              f"{bridge['size']} B  role={bridge['role']}", file=out)
    for claim in dossier["retail_source_claims"]:
        print(f"  source claim {_hex(claim['va'], 8)}  "
              f"{claim['path']}:{claim['line']}", file=out)

    for title, key in (("Parameters", "parameters"), ("Locals", "locals")):
        rows = dossier[key]
        print(f"\n{title} ({len(rows)}; CodeView lower bound):", file=out)
        if not rows:
            print("  none recorded", file=out)
        for item in rows:
            print(f"  {item['sp_offset']:>9}  {item['type']:<20}  {item['name']}",
                  file=out)

    print(f"\nLexical scopes ({len(dossier['scopes'])}):", file=out)
    if not dossier["scopes"]:
        print("  none recorded", file=out)
    for scope in dossier["scopes"]:
        print(f"  {_hex(scope['start'], 8)}..{_hex(scope['end'], 8)}  "
              f"{scope['size']} B", file=out)

    print(f"\nStatements ({len(dossier['statements'])} line/address rows):", file=out)
    for statement in dossier["statements"]:
        marks = " {" * statement["scope_opens"] + " }" * statement["scope_closes"]
        branch = f" br={statement['branches']}" if statement["branches"] else ""
        print(f"  {_basename(statement['source'])}:{statement['line']:<5} "
              f"dc {_hex(statement['address'], 8)} {statement['size']:>4} B"
              f"{branch}{marks}", file=out)
        for call in statement["calls"]:
            name = call["name"] or _hex(call["target_va"])
            details = []
            if call["kind"] and call["kind"] != "call":
                details.append(call["kind"])
            if call["dc_offset"] is not None:
                details.append(f"dc {_hex(call['dc_offset'])}")
            if call["dc_size"] is not None:
                details.append(f"{call['dc_size']} B")
            suffix = f"  [{', '.join(details)}]" if details else ""
            print(f"      -> {name}{suffix}", file=out)

    summary = dossier["summary"]
    print("\nSummary:", file=out)
    print(f"  source rows={summary['statement_rows']}  "
          f"branches={summary['conditional_branches']}  calls={summary['calls']}  "
          f"unique callees={summary['unique_named_callees']}", file=out)
    print(f"  ctors={summary['constructor_calls']}  dtors={summary['destructor_calls']}  "
          f"tiny-helper calls={summary['tiny_helper_calls']}", file=out)
    print("\nCaution: DC source is older and platform-specific. Promote a source fact "
          "only after retail x86 corroboration.", file=out)


def _mapped_vas(corpus: Corpus, row: dict[str, str]) -> list[int]:
    key = corpus.key(row)
    values = {common.IMAGE_BASE + _integer(item["rva"])
              for item in corpus.bridges_by_key.get(key, ())}
    values.update(claim.va for claim in corpus.claims_by_key.get(key, ()))
    return sorted(values)


def _find_payload(corpus: Corpus, rows: list[dict[str, str]]) -> list[dict[str, Any]]:
    return [{
        "name": row["name"], "module": row["module"],
        "dc_offset": _integer(row["offset"]), "dc_size": _integer(row["cb"]),
        "source": row["file"], "line": _integer(row["line"]),
        "retail_vas": _mapped_vas(corpus, row),
    } for row in rows]


def _render_find(rows: list[dict[str, Any]], out: TextIO = sys.stdout) -> None:
    print("Dreamcast CodeView corpus — analysis output, not retail evidence",
          file=out)
    if not rows:
        print("no Dreamcast procedure names matched", file=out)
        return
    for row in rows:
        retail = ",".join(_hex(va, 8) for va in row["retail_vas"]) or "-"
        print(f"{_hex(row['dc_offset'], 8)} {row['dc_size']:>6} B  "
              f"{row['module']:<30} retail={retail}", file=out)
        print(f"  {row['name']}  ({_basename(row['source'])}:{row['line']})", file=out)


def _stats(corpus: Corpus) -> dict[str, Any]:
    sizes = [_integer(row["cb"]) for row in corpus.functions]
    keys_with_bridge = set(corpus.bridges_by_key)
    keys_with_claim = set(corpus.claims_by_key)
    return {
        "authority": AUTHORITY,
        "functions": len(corpus.functions),
        "modules": len({row["module"] for row in corpus.functions}),
        "parameters": sum(row["kind"] == "param" for row in corpus.variables),
        "locals": sum(row["kind"] != "param" for row in corpus.variables),
        "functions_with_recorded_locals": len({(row["module"], row["proc"])
                                               for row in corpus.variables
                                               if row["kind"] != "param"}),
        "retail_bridge_rows": len(corpus.bridges),
        "functions_with_retail_bridge": len(keys_with_bridge),
        "functions_with_source_claim": len(keys_with_claim),
        "four_byte_functions": sum(size == 4 for size in sizes),
        "bridge_roles": dict(sorted(Counter(row["role"]
                                            for row in corpus.bridges).items())),
    }


def _render_stats(stats: dict[str, Any], out: TextIO = sys.stdout) -> None:
    print(stats["authority"], file=out)
    print(f"  functions                    {stats['functions']}", file=out)
    print(f"  modules                      {stats['modules']}", file=out)
    print(f"  parameters                   {stats['parameters']}", file=out)
    print(f"  locals                       {stats['locals']}", file=out)
    print(f"  functions with locals        {stats['functions_with_recorded_locals']}",
          file=out)
    print(f"  four-byte functions          {stats['four_byte_functions']}", file=out)
    print(f"  retail bridge rows           {stats['retail_bridge_rows']}", file=out)
    print(f"  functions with bridge        {stats['functions_with_retail_bridge']}",
          file=out)
    print(f"  functions with source claim  {stats['functions_with_source_claim']}",
          file=out)
    print("  bridge roles                 " + ", ".join(
        f"{name}={count}" for name, count in stats["bridge_roles"].items()), file=out)


def _build_parser() -> argparse.ArgumentParser:
    ap = argparse.ArgumentParser(
        prog="homm3 dreamcast", description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = ap.add_subparsers(dest="command", required=True)

    show = sub.add_parser("show", help="joined source-shape dossier")
    show.add_argument("selector", help="retail VA/RVA, dc:OFF, module:OFF, or name")
    show.add_argument("--json", action="store_true", help="machine-readable output")

    asm = sub.add_parser("asm", help="breakpoint-labelled SH4 assembly / CFG")
    asm.add_argument("selector", help="retail VA/RVA, dc:OFF, module:OFF, or name")
    asm.add_argument("--blocks", action="store_true",
                     help="show inferred CFG predecessor/successor headers")
    asm.add_argument("--no-breakpoints", action="store_true",
                     help="suppress CodeView source-line labels")
    asm.add_argument("--json", action="store_true", help="machine-readable output")

    find = sub.add_parser("find", help="search Dreamcast procedure names")
    find.add_argument("text", help="case-insensitive name substring")
    find.add_argument("--module", help="restrict to one module[.obj]")
    find.add_argument("--limit", type=int, default=50,
                      help="maximum rows (default 50; 0 = all)")
    find.add_argument("--json", action="store_true", help="machine-readable output")

    stats = sub.add_parser("stats", help="corpus and retail-bridge coverage")
    stats.add_argument("--json", action="store_true", help="machine-readable output")
    return ap


def _log(rc: int, argv: list[str]) -> None:
    try:
        import datetime
        now = datetime.datetime.now()
        LOG.parent.mkdir(parents=True, exist_ok=True)
        with LOG.open("a") as fh:
            fh.write(f"[{now.date()}][{now.strftime('%H:%M:%S')}][{rc}]: "
                     f"{shlex.join(['homm3', 'dreamcast', *argv])}\n")
    except Exception:
        pass


def main(argv: list[str] | None = None) -> int:
    argv = list(sys.argv[1:] if argv is None else argv)
    parser = _build_parser()
    rc = 0
    try:
        args = parser.parse_args(argv)
        corpus = Corpus()
        if args.command == "show":
            row = corpus.resolve(args.selector)
            dossier = build_dossier(corpus, row)
            if args.json:
                json.dump(dossier, sys.stdout, indent=2, sort_keys=True)
                print()
            else:
                render_dossier(dossier)
        elif args.command == "asm":
            row = corpus.resolve(args.selector)
            if not dc_lines.DUMP.is_file():
                raise DreamcastError(
                    f"Dreamcast CodeView dump not found: {dc_lines.DUMP}")
            dump = dc_lines._dump_lines()
            data = _gate_dc_exe(dc_lines.EXE)
            try:
                view = dc_asm.build_view(row, dump, data)
            except dc_asm.AsmError as exc:
                raise DreamcastError(str(exc)) from exc
            if args.json:
                json.dump(view, sys.stdout, indent=2, sort_keys=True)
                print()
            else:
                dc_asm.render(view, data, dc_lines.symbol_map(dump),
                              blocks=args.blocks,
                              breakpoints=not args.no_breakpoints,
                              out=sys.stdout)
        elif args.command == "find":
            rows = corpus.find(args.text, args.module)
            if args.limit < 0:
                raise DreamcastError("--limit must be >= 0")
            if args.limit:
                rows = rows[:args.limit]
            payload = _find_payload(corpus, rows)
            if args.json:
                json.dump({"authority": AUTHORITY, "matches": payload},
                          sys.stdout, indent=2, sort_keys=True)
                print()
            else:
                _render_find(payload)
        else:
            payload = _stats(corpus)
            if args.json:
                json.dump(payload, sys.stdout, indent=2, sort_keys=True)
                print()
            else:
                _render_stats(payload)
    except DreamcastError as exc:
        print(f"[homm3 dreamcast] ERROR: {exc}", file=sys.stderr)
        rc = 2
    except SystemExit as exc:
        rc = exc.code if isinstance(exc.code, int) else 2
        _log(rc, argv)
        raise
    _log(rc, argv)
    return rc


if __name__ == "__main__":
    sys.exit(main())
