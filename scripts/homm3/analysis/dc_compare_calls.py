"""Dreamcast versus Mac retail callees per paired function.

`homm3 dreamcast compare-calls` joins each retail function that has both a
Dreamcast source claim and a Mac `MAC_ADDRESS` claim, then compares the game
helpers the two native builds call, by normalised name (`dc_callees`):
CodeWarrior mangling dropped, case and underscores ignored, last component
compared. The standard library, C runtime, operator new/delete, import glue
and compiler/runtime helpers are excluded. A callee one platform calls and
the other does not is usually an inlining decision (or an older source); a
Dreamcast-missing callee shows the Dreamcast inline-clue trace when one
exists.

Mac call sites come from the pinned PEF bytes; targets are named by the
source claims (no CodeWarrior compile). Targets without a claim stay counted
as unnamed. Neither side is retail Windows evidence.
"""
from __future__ import annotations

from collections import Counter, defaultdict
import json
import sys
from typing import Any, TextIO

from homm3.analysis import dc_callees, dc_diff_locals, dreamcast

SCHEMA = "homm3.dreamcast-compare-calls.v1"
CAUTION = (
    "Dreamcast (SH4, older RoE) and Mac (CodeWarrior PowerPC) are independent "
    "native builds. A callee present on one side only is a helper-boundary or "
    "inlining lead for the Windows source, never retail evidence by itself."
)


class MacCalls:
    """Retail Mac call sites of claimed functions, named by source claims."""

    def __init__(self, root=None):
        from homm3.core import common, inputs
        from homm3.mac import addresses, glue, tables
        from homm3.mac.pef import PEF
        self.root = root or common.HOMM3_DIR
        _claims, windows, problems = addresses.scan(self.root)
        if problems:
            raise dreamcast.DreamcastError("Mac claims: " + "; ".join(problems[:3]))
        self.pef = PEF(inputs.read_verified(inputs.MAC, inputs.stage_executable(inputs.MAC)))
        self.section = tables.CODE_SECTION
        names: dict[int, str] = {}
        self.pairs: dict[int, Any] = {}
        for claim in windows:
            if claim.mac is None:
                continue
            names.setdefault(claim.mac.offset, claim.label)
            if claim.va is not None:
                self.pairs[claim.va] = claim
        for claim in _claims:
            if claim.label:
                names.setdefault(claim.offset, claim.label)
        plumbing = {row.offset: row.name for row in tables.read_runtime(self.root)}
        plumbing.update({row.offset: row.name for row in tables.read_glue(self.root)})
        plumbing.update({row.offset: row.name for row in tables.read_zlib(self.root)})
        from homm3.mac.relocations import Address, CallTarget
        self.symbols: dict[str, Any] = {}
        self.labels: dict[str, str] = {}
        for offset, name in names.items():
            key = f"game:{offset:x}"
            self.symbols[key] = Address(self.section, offset)
            self.labels[key] = name
        for offset, name in plumbing.items():
            key = f"plumbing:{offset:x}"
            if offset not in names:
                self.symbols[key] = Address(self.section, offset)
                self.labels[key] = name
        for name, target in glue.imports(self.pef).items():
            self.symbols[f"plumbing:import:{name}"] = target

    def calls(self, va: int) -> dict[str, Any] | None:
        from homm3.mac import calls
        from homm3.mac.relocations import Address
        claim = self.pairs.get(va)
        if claim is None:
            return None
        mac = claim.mac
        view = calls.analyze(self.pef.code(self.section, mac.offset, mac.size),
                             Address(self.section, mac.offset), self.symbols, labels=self.labels)
        named, unnamed, plumbing = [], 0, 0
        for site in view["sites"]:
            symbol = site.get("symbol") or ""
            if site["kind"] != "direct":
                unnamed += 1
            elif symbol.startswith("game:"):
                named.append(site["label"])
            elif symbol.startswith("plumbing:") or site["target"] and not site["target"].startswith("mac:"):
                plumbing += 1
            else:
                unnamed += 1
        return {"label": claim.label, "mac_offset": mac.offset, "mac_size": mac.size,
                "named": named, "unnamed_sites": unnamed, "plumbing_sites": plumbing}


def _vendor(corpus, offset: int) -> bool:
    """A Dreamcast procedure compiled from the vendored zlib sources."""
    return any("\\zlib" in dreamcast._source_key(row["file"])
               for row in corpus.by_offset.get(offset, ()))


def _keyed(names: list[str]) -> tuple[dict[str, int], dict[str, str]]:
    counts, display = Counter(), {}
    for name in names:
        identity = dc_callees.callee(name)
        if identity is None:
            continue
        counts[identity.key] += 1
        display.setdefault(identity.key, dc_callees.qualified_name(name) or name)
    return counts, display


def compare(dc_names: list[str], mac_names: list[str],
            traces: dict[str, list[dict]]) -> dict[str, list[dict]]:
    dc, dc_display = _keyed(dc_names)
    mac, mac_display = _keyed(mac_names)
    mac_only, dc_only = [], []
    for key in sorted(set(dc) | set(mac)):
        if dc[key] == mac[key]:
            continue
        item = {"key": key, "callee": mac_display.get(key) or dc_display[key],
                "mac": mac[key], "dreamcast": dc[key]}
        if mac[key] > dc[key]:
            item["dc_inline"] = traces.get(key, [])
            mac_only.append(item)
        else:
            dc_only.append(item)
    absent_first = lambda item: (min(item["mac"], item["dreamcast"]) > 0, item["key"])
    return {"mac_only": sorted(mac_only, key=absent_first),
            "dreamcast_only": sorted(dc_only, key=absent_first)}


def aggregate(functions: list[dict[str, Any]]) -> dict[str, list[dict[str, Any]]]:
    out = {}
    for side in ("mac_only", "dreamcast_only"):
        table: dict[str, dict[str, Any]] = {}
        for function in functions:
            for item in function.get(side, []):
                row = table.setdefault(item["key"], {
                    "key": item["key"], "callee": item["callee"], "functions": 0,
                    "absent_functions": 0, "mac_sites": 0, "dreamcast_sites": 0,
                    "dc_inline_functions": 0})
                row["functions"] += 1
                row["absent_functions"] += min(item["mac"], item["dreamcast"]) == 0
                row["mac_sites"] += item["mac"]
                row["dreamcast_sites"] += item["dreamcast"]
                row["dc_inline_functions"] += bool(item.get("dc_inline"))
        out[side] = sorted(table.values(), key=lambda row: (
            -row["functions"], -row["absent_functions"], row["key"]))
    return out


def run(corpus, args) -> int:
    from homm3.analysis import dc_lines
    from homm3.core import inputs
    from homm3.core.nb11_types import Types
    selectors = [args.selector] if args.selector else []
    pairs = dc_diff_locals.paired(corpus, selectors=selectors, units=args.unit or (),
                                  modules=args.module or (), all_functions=args.all)
    mac = MacCalls()
    pairs = [(row, claim) for row, claim in pairs if claim.va in mac.pairs]
    if not pairs:
        raise dreamcast.NoMatch("no selected function has both Dreamcast and Mac claims")
    pairs.sort(key=lambda pair: pair[1].va)
    dump, data = dc_lines.load_symbols(), inputs.read_dreamcast_exe()
    types = Types.from_symbols(dump)
    functions = []
    for row, claim in pairs:
        dossier = dreamcast.build_dossier(corpus, row, dump=dump, data=data, type_table=types)
        clue_rows = dreamcast._inline_clue_rows(corpus, row, dump.source_lines.get(row["module"], ()))
        traces = dc_callees.inline_traces(dreamcast._inline_clue_groups(clue_rows))
        # Calls into DC debug procedures only (no import thunks); the vendored
        # zlib is plumbing on both sides (the Mac zlib map is excluded too).
        dc_names = [call.name for statement in dossier.shape.statements
                    for call in statement.calls
                    if call.name and call.target_function_address is not None
                    and not _vendor(corpus, call.target_function_address)]
        sites = mac.calls(claim.va)
        item = {"va": claim.va, "name": row["name"], "module": row["module"],
                "dc_offset": dreamcast._integer(row["offset"]), "source": claim.path,
                "mac_label": sites["label"], "mac_offset": sites["mac_offset"],
                "mac_unnamed_sites": sites["unnamed_sites"],
                **compare(dc_names, sites["named"], traces)}
        functions.append(item)
    totals = aggregate(functions)
    summary = {"functions": len(functions),
               "identical": sum(not f["mac_only"] and not f["dreamcast_only"] for f in functions),
               "mac_only_callees": sum(len(f["mac_only"]) for f in functions),
               "dreamcast_only_callees": sum(len(f["dreamcast_only"]) for f in functions),
               "mac_unnamed_sites": sum(f["mac_unnamed_sites"] for f in functions)}
    payload = {"schema": SCHEMA, "authority": dreamcast.AUTHORITY, "caution": CAUTION,
               "summary": summary, "aggregate": totals, "functions": functions}
    if args.json:
        json.dump(payload, sys.stdout, indent=2, sort_keys=True)
        print()
    else:
        render(payload, limit=args.limit)
    return 0


def render(payload: dict[str, Any], *, limit: int = 25, out: TextIO = sys.stdout) -> None:
    print("DREAMCAST / MAC CALLEE COMPARISON — ANALYSIS OUTPUT, NOT RETAIL EVIDENCE", file=out)
    print(CAUTION, file=out)
    functions = payload["functions"]
    detailed = len(functions) <= limit or not limit
    for function in functions if detailed else ():
        print(f"\n0x{function['va']:08x} {function['name']}  [{function['module']} "
              f"dc:0x{function['dc_offset']:x}; mac:0x{function['mac_offset']:x}]", file=out)
        if function["mac_unnamed_sites"]:
            print(f"  {function['mac_unnamed_sites']} Mac site(s) to unclaimed or indirect targets",
                  file=out)
        if not function["mac_only"] and not function["dreamcast_only"]:
            print("  same named callees", file=out)
        for item in function["mac_only"]:
            print(f"  Mac only  {item['callee']} (Mac {item['mac']}, DC {item['dreamcast']})", file=out)
            for line in dc_callees.render_traces(item.get("dc_inline", [])):
                print(f"      {line}", file=out)
        for item in function["dreamcast_only"]:
            print(f"  DC only   {item['callee']} (DC {item['dreamcast']}, Mac {item['mac']})", file=out)
    summary = payload["summary"]
    print(f"\n{summary['functions']} paired function(s), {summary['identical']} with the same "
          f"named callees; {summary['mac_only_callees']} Mac-only and "
          f"{summary['dreamcast_only_callees']} Dreamcast-only callee difference(s); "
          f"{summary['mac_unnamed_sites']} Mac site(s) unnamed", file=out)
    for side, title in (("mac_only", "Mac calls, Dreamcast does not (DC inlined or source differs)"),
                        ("dreamcast_only", "Dreamcast calls, Mac does not (Mac inlined or source differs)")):
        rows = payload["aggregate"][side]
        if not rows:
            continue
        print(f"\n{title}:", file=out)
        print("  functions  absent  sites(Mac/DC)  callee", file=out)
        for row in rows[:limit or None]:
            inline = (f"  [DC inline trace in {row['dc_inline_functions']}]"
                      if row.get("dc_inline_functions") else "")
            print(f"  {row['functions']:>9}  {row['absent_functions']:>6}  "
                  f"{row['mac_sites']:>5}/{row['dreamcast_sites']:<6}  {row['callee']}{inline}", file=out)
        if limit and len(rows) > limit:
            print(f"  (+{len(rows) - limit} more; --limit 0 or --json for all)", file=out)


def add_parser(sub) -> None:
    parser = sub.add_parser(
        "compare-calls", help="Dreamcast versus Mac retail callees per paired function")
    parser.add_argument("selector", nargs="?", metavar="SELECTOR",
                        help="retail VA/RVA, dc:OFF, module:OFF, or name")
    scope = parser.add_mutually_exclusive_group()
    scope.add_argument("--unit", action="append", metavar="UNIT", help="build unit; repeatable")
    scope.add_argument("--module", action="append", metavar="MODULE",
                       help="Dreamcast module[.obj]; repeatable")
    scope.add_argument("--all", action="store_true", help="every Dreamcast+Mac paired function")
    parser.add_argument("--limit", type=int, default=25,
                        help="aggregate rows per side, and the most functions listed in "
                             "detail (default 25; 0 = all)")
    parser.add_argument("--json", action="store_true", help="machine-readable output")
