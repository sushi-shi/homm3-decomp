"""Inspect emitted functions and rank pre-claim hypotheses by mnemonic similarity."""
from __future__ import annotations

import contextlib
import csv
import difflib
import json
from pathlib import Path
import sys
import subprocess

from homm3 import manifest
from homm3.build.canonicalize_data_symbols import CoffObject, FUNCTION_TYPE, MEM_EXECUTE
from homm3.core import common, undname
from homm3.sema import _asm, data
from homm3.sema._common import die
from homm3.sema.context import get_context


def functions(path):
    try:
        coff = CoffObject(path.read_bytes())
    except (OSError, ValueError) as exc:
        die(f"cannot read candidate object {path}: {exc}")
    symbols = [s for s in coff.symbols.values()
               if s.section > 0 and s.typ == FUNCTION_TYPE
               and coff.sections[s.section - 1].characteristics & MEM_EXECUTE]
    ordinals = {}
    for symbol in symbols:
        section = coff.sections[symbol.section - 1]
        end = min((s.value for s in symbols if s.section == symbol.section
                   and s.value > symbol.value), default=section.raw_size)
        raw = coff.section_bytes(section)[symbol.value:end]
        ordinal = ordinals.get(symbol.name, 0)
        ordinals[symbol.name] = ordinal + 1
        yield {"symbol": symbol.name, "ordinal": ordinal, "section": symbol.section,
               "offset": symbol.value, "size": len(raw), "raw": raw}


def mnemonics(raw):
    import capstone
    decoder = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
    rows = [ins.mnemonic for ins in decoder.disasm(raw, 0)]
    while rows and rows[-1] in ("nop", "int3"):
        rows.pop()
    return rows


def selected_text(path, selector, ordinal):
    inventory = list(functions(path))
    names = {fn["symbol"] for fn in inventory}
    name = selector
    if name not in names:
        demangled = undname.qualified_names(names)
        query = undname.strip_signature(name)
        matches = [m for m, q in demangled.items()
                   if q == query or undname.bare(q) == query]
        if len(matches) != 1:
            die(f"{selector!r} has {len(matches)} candidate symbols in {path}; "
                "supply an exact mangled name")
        name = matches[0]
    res = subprocess.run(["llvm-objdump", "-dr", "--x86-asm-syntax=intel", str(path)],
                         capture_output=True, text=True)
    if res.returncode:
        die(res.stderr.strip())
    text = _asm._slice_public_symbol(res.stdout, name, ordinal, names)
    if text is None:
        die(f"symbol {name!r} ordinal {ordinal} not found in {path}")
    return text, name


def object_path(args):
    if args.object:
        path = Path(args.object).resolve()
        if not path.is_file():
            die(f"object missing: {path}")
        return path
    if not args.unit:
        die("supply --unit TU or --object PATH")
    units = {row["unit"] for row in manifest.load()["unit"]}
    if args.unit not in units:
        die(f"unknown manifest unit: {args.unit}")
    if not args.no_build:
        note = _asm.refresh_unit(args.unit)
        if note:
            print(note, file=sys.stderr)
    path = _asm.BASE / f"{args.unit}.obj"
    if not path.is_file():
        die(f"object missing: {path}")
    return path


def run(args):
    if args.limit < 0:
        die("--limit must be >= 0")
    ctx = get_context()
    target = None
    with contextlib.redirect_stdout(sys.stderr):
        if args.target:
            target = ctx.symbols.resolve_fn(args.target)
        if args.object or args.unit:
            paths = [object_path(args)]
        else:
            paths = []
            for unit in manifest.load()["unit"]:
                args.unit = unit["unit"]
                paths.append(object_path(args))
            args.unit = None
    wanted = mnemonics(data.read(ctx.image, target[2], target[3])) if target else None
    rows = []
    claims = {}
    for rva, row in ctx.symbols.funcs.items():
        claims.setdefault((row[1], row[0]), []).append(rva + common.IMAGE_BASE)
    for path in paths:
        aliases = {}
        if path.parent == _asm.BASE:
            mapping = _asm.NORMAL_BASE / f"{path.stem}.symbols.tsv"
            normalized = _asm.NORMAL_BASE / f"{path.stem}.obj"
            from homm3.build.normalized_freshness import freshness_problems
            if mapping.is_file() and normalized.is_file() and not freshness_problems(normalized):
                with mapping.open() as stream:
                    aliases = {row["original_name"]: row["canonical_name"]
                               for row in csv.DictReader(stream, delimiter="\t")
                               if row["storage"] == "text"}
        inventory = list(functions(path))
        names = undname.demangle(fn["symbol"] for fn in inventory) if args.find else {}
        for fn in inventory:
            name = fn["symbol"]
            if args.find and args.find.casefold() not in (name + " " + names.get(name, "")).casefold():
                continue
            retail_name = aliases.get(name, name)
            claimed = claims.get((path.stem, retail_name), [])
            if args.unclaimed and claimed:
                continue
            seq = mnemonics(fn.pop("raw"))
            score = difflib.SequenceMatcher(None, wanted, seq, autojunk=False).ratio() if wanted else None
            rows.append(dict(fn, object=str(path), unit=path.stem, claimed_vas=claimed,
                             retail_symbol=retail_name,
                             instructions=len(seq), mnemonic_similarity=score))
    rows.sort(key=lambda row: (-(row["mnemonic_similarity"] or 0), row["unit"],
                               row["symbol"], row["ordinal"]))
    total = len(rows)
    rows = rows[:args.limit or None]
    payload = {"schema": "homm3.sema.candidates.v1", "exploratory": True,
               "metric": "mnemonic similarity; not a byte-match score or identity proof",
               "target": target, "total": total, "candidates": rows}
    if args.json:
        print(json.dumps(payload, indent=2))
    else:
        print(payload["metric"])
        for row in rows:
            score = f"{row['mnemonic_similarity']:.4f} " if wanted is not None else ""
            print(f"{score}{row['unit']}:{row['symbol']} ordinal={row['ordinal']} "
                  f"{row['size']} B  claimed={bool(row['claimed_vas'])}")
        print(f"{len(rows)} shown / {total} candidates")
    return 0 if rows else 1
