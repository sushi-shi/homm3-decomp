#!/usr/bin/env python3
"""homm3.carve.audit - S7: fatal gates, negative control, oracle cross-checks.

Fatal gates (each can fail, and the negative control proves the partition
gate still detects the defect this package exists to fix):

  terminator  every extent either ends at ret/retf/jmp (buka's rule, over a
              flattened linear disassembly) or ends at its own attributed
              table; counted violations above budget are fatal.
  partition   extents + padding tile .text; non-pad bytes outside every
              extent are residue. Residue above RESIDUE_BUDGET is fatal.
              The budget deliberately sits between the measured genuine
              residue of this image (~26 KB of never-rooted dead code +
              embedded data; attempt-1 classified the same bytes as
              unowned-defined-instruction/embedded-defined-data) and the
              ~23 KB of table bytes a body-only carve would additionally
              leak - so clipped sizes CANNOT pass.
  negative    recompute the partition with deliberately body-only sizes;
              the residue gate MUST trip, else the gate is vacuous.
  eh-funclet  FuncInfo walk over .rdata (magics 0x1993052{0,1,2}): every
              non-null unwind action target is an entry (attempt-1: 5,125).
  pins        init-array >=1000 slots; switch-table count near attempt-1's
              334; zero uncovered vtable slots.

Report-only (oracles, never inputs): attempt-1 functions/vtables diff,
NH3API wrapper-address coverage. When every fatal gate is green this stage
renders the functions.tsv deliverable; `--emit-config` additionally renders
admission candidates under build/carve/config-candidate/.
"""
from __future__ import annotations

import struct
import sys
from collections import Counter

from homm3.carve import common
from homm3.carve.tables import PAD_BYTES

FUNCTIONS_TSV = common.CARVE_DIR / "functions.tsv"
CANDIDATE_DIR = common.CARVE_DIR / "config-candidate"
ATTEMPT1 = common.HOMM3_DIR / "../decomp-attempt-1/config"

TERMINATORS = {"ret", "retf", "jmp", "ljmp"}
RESIDUE_BUDGET = 36 * 1024
TERMINATOR_BUDGET = 40
EH_MAGICS = {0x19930520, 0x19930521, 0x19930522}


def load_inputs():
    image, _info = common.load_image()
    ext = common.read_tsv(common.need(
        common.CARVE_DIR / "functions_extended.tsv", "extents"))
    funcs = [(int(r["rva"], 16), int(r["size"], 16 if r["size"].startswith("0x")
              else 10), r) for r in ext]
    funcs.sort()
    tables = common.read_tsv(common.need(
        common.CARVE_DIR / "jump_tables.tsv", "tables"))
    return image, funcs, tables


def instruction_ends(image):
    """rva-after-instruction -> mnemonic, from one linear llvm-objdump pass."""
    from homm3.carve.find_relocs import ROW, disassemble
    ends = {}
    for section in image.sections:
        if not section.executable:
            continue
        for line in disassemble(image, section).splitlines():
            row = ROW.match(line)
            if not row:
                continue
            rva = int(row.group(1), 16) - image.image_base
            raw_len = len(row.group(2).replace(" ", "")) // 2
            text = row.group(3)
            mnemonic = text.split(None, 1)[0] if text else ""
            ends[rva + raw_len] = mnemonic
    return ends


def partition_residue(image, spans):
    """Non-pad bytes in .text outside every span. Returns (residue, gaps)."""
    text = next(s for s in image.sections if s.name == ".text")
    blob = image.blob(text)
    lo, hi = text.rva, text.rva + text.size
    residue = 0
    bad_gaps = []
    cursor = lo
    for start, end in spans:
        if start > cursor:
            gap = blob[cursor - lo:start - lo]
            bad = sum(1 for b in gap if b not in PAD_BYTES)
            residue += bad
            if bad:
                bad_gaps.append((cursor, start, bad))
        cursor = max(cursor, end)
    if hi > cursor:
        gap = blob[cursor - lo:hi - lo]
        bad = sum(1 for b in gap if b not in PAD_BYTES)
        residue += bad
        if bad:
            bad_gaps.append((cursor, hi, bad))
    return residue, bad_gaps


def gate_terminators(image, funcs, tables, ends):
    table_end_of = {}
    for r in tables:
        owner = int(r["owner_rva"], 16)
        end = int(r["table_rva"], 16) + int(r["size"])
        table_end_of[owner] = max(table_end_of.get(owner, 0), end)

    text = next(s for s in image.sections if s.name == ".text")
    blob = image.blob(text)
    lo = text.rva
    starts = sorted(rva for rva, _s, _r in funcs)

    def clean_follower(end):
        """Padding (or nothing) up to the next entry."""
        import bisect
        i = bisect.bisect_right(starts, end - 1)
        nxt = starts[i] if i < len(starts) else text.rva + text.size
        return all(b in PAD_BYTES for b in blob[end - lo:nxt - lo])

    cats = Counter()
    violations = []
    for rva, size, row in funcs:
        extent_end = rva + size
        if table_end_of.get(rva) == extent_end:
            cats["table-ended"] += 1
            continue
        mnemonic = ends.get(extent_end)
        if mnemonic in TERMINATORS:
            cats["terminator"] += 1
        elif mnemonic is None:
            cats["desync"] += 1  # linear sweep disagrees with Ghidra here
        elif mnemonic == "call" and clean_follower(extent_end):
            # a trailing call to a noreturn callee (throw helpers, _amsg_exit)
            # is legitimate VC6 codegen; the clean follower proves the extent
            # still tiles - only unpadded mid-stream ends stay violations
            cats["call-noreturn"] += 1
        else:
            cats["violation"] += 1
            violations.append((rva, extent_end, mnemonic))
    return cats, violations


def gate_eh_funclets(image, entries):
    """Every FuncInfo unwind action must target a known function entry."""
    rdata = next(s for s in image.sections if s.name == ".rdata")
    blob = image.blob(rdata)
    text = next(s for s in image.sections if s.name == ".text")
    text_lo, text_hi = text.rva, text.rva + text.size
    base = image.image_base

    func_infos = 0
    actions = set()
    missing = []
    for offset in range(0, len(blob) - 28 + 1, 4):
        magic, max_state, unwind_va, ntry, _try_va, nip, _ip_va = \
            struct.unpack_from("<7I", blob, offset)
        if magic not in EH_MAGICS:
            continue
        if max_state > 0x1000 or ntry > 0x1000 or nip > 0x10000:
            continue
        if max_state == 0:
            func_infos += unwind_va == 0
            continue
        unwind_rva = unwind_va - base
        start = unwind_rva - rdata.rva
        if not 0 <= start <= len(blob) - max_state * 8:
            continue
        parsed = []
        valid = True
        for state in range(max_state):
            to_state, action_va = struct.unpack_from(
                "<iI", blob, start + state * 8)
            if not -1 <= to_state < max_state:
                valid = False
                break
            if action_va:
                action_rva = action_va - base
                if not text_lo <= action_rva < text_hi:
                    valid = False
                    break
                parsed.append(action_rva)
        if not valid:
            continue
        func_infos += 1
        for action_rva in parsed:
            actions.add(action_rva)
            if action_rva not in entries:
                missing.append(action_rva)
    return func_infos, actions, sorted(set(missing))


PROLOGUES = (b"\x55\x8b\xec",)  # push ebp; mov ebp, esp
PROLOGUE_FIRST = {0x51, 0x53, 0x55, 0x56, 0x57,   # push reg
                  0x8b, 0x8a, 0xa0, 0xa1,          # mov forms
                  0x83, 0x81, 0x6a, 0x68, 0x33,    # sub/push imm/xor
                  0xb8, 0xb9, 0xba, 0xd9, 0xe9}    # mov r,imm / fld / jmp


def gap_candidates(image, spans, a1_entries):
    """Report-only: functions are packed in .text, so a residue gap start
    (after stripping padding) is usually a function Ghidra missed. Not
    necessarily correct - a gap can also hold a desync tail or embedded data
    - hence candidates, never roots.
    """
    text = next(s for s in image.sections if s.name == ".text")
    blob = image.blob(text)
    lo, hi = text.rva, text.rva + text.size
    rows = []
    cursor = lo
    for start, end in spans + [(hi, hi)]:
        if start > cursor:
            gap_lo, gap_hi = cursor, start
            # packed layout: candidates are the first non-pad byte, plus every
            # 16-aligned non-pad byte right after padding (a new slot)
            cands = []
            for off in range(gap_lo, gap_hi):
                if blob[off - lo] in PAD_BYTES:
                    continue
                prev_pad = off == gap_lo or blob[off - 1 - lo] in PAD_BYTES
                if prev_pad and (not cands or off % 16 == 0):
                    cands.append(off)
            for i, cand in enumerate(cands):
                nxt = cands[i + 1] if i + 1 < len(cands) else gap_hi
                first = blob[cand - lo:cand - lo + 16]
                prologue = (any(first.startswith(p) for p in PROLOGUES)
                            or first[0] in PROLOGUE_FIRST)
                rows.append((f"0x{cand:x}", nxt - cand,
                             int(cand % 16 == 0), int(prologue),
                             int(cand in a1_entries), first.hex()))
        cursor = max(cursor, end)
    return rows


def cross_check_attempt1(funcs, tables):
    """Report-only oracle diff; never an input."""
    import csv
    path = ATTEMPT1 / "functions.csv"
    if not path.is_file():
        print("[carve audit] attempt-1 oracle not present; diff skipped")
        return
    theirs = {}
    with path.open(newline="") as fh:
        for r in csv.DictReader(fh):
            theirs[int(r["entry_rva"], 16)] = int(r["byte_size"])
    ours = {rva: size for rva, size, _r in funcs}
    only_theirs = sorted(set(theirs) - set(ours))
    only_ours = sorted(set(ours) - set(theirs))
    common_entries = set(theirs) & set(ours)
    table_owners = {int(r["owner_rva"], 16) for r in tables}
    shrunk = [e for e in common_entries & table_owners
              if ours[e] < theirs[e]]
    sum_delta = sum(ours[e] - theirs[e] for e in common_entries)
    print(f"[carve audit] attempt-1 diff (report-only): "
          f"{len(common_entries)} shared entries, "
          f"{len(only_theirs)} only-theirs, {len(only_ours)} only-ours")
    print(f"  sum(size delta over shared) = {sum_delta:+d} B "
          f"(their embedded tables: 12964 B kept out of byte_size)")
    if shrunk:
        print(f"  WARNING: {len(shrunk)} table-owning functions SMALLER than "
              f"attempt-1: {[hex(e) for e in shrunk[:8]]}")
    else:
        print("  all table-owning shared functions are >= attempt-1 sizes")

    nh = ATTEMPT1 / "nh3api-wrapper-addresses.csv"
    if nh.is_file():
        with nh.open(newline="") as fh:
            addrs = {int(r["wrapper_va"], 16) - 0x400000
                     for r in csv.DictReader(fh)}
        hit = sum(1 for a in addrs if a in ours)
        print(f"  NH3API wrapper addresses on our entries: {hit}/{len(addrs)} "
              "(external-unverified, coverage note only)")


def emit_config(funcs):
    for name in ("functions.tsv", "vtables.tsv"):
        src = common.need(common.CARVE_DIR / name, "audit")
        dst = CANDIDATE_DIR / f"retail-{name}"
        dst.parent.mkdir(parents=True, exist_ok=True)
        dst.write_text(common.MANUAL_BANNER + src.read_text())
    print(f"[carve audit] admission candidates -> {CANDIDATE_DIR} "
          "(supervised review required before anything reaches config/)")


def main(argv=None) -> int:
    argv = list(argv or [])
    image, funcs, tables = load_inputs()
    entries = {rva for rva, _s, _r in funcs}
    failures = []

    # --- terminator + partition -----------------------------------------
    ends = instruction_ends(image)
    cats, violations = gate_terminators(image, funcs, tables, ends)
    print(f"[carve audit] terminators: {dict(cats)}")
    for rva, end, mnemonic in violations[:10]:
        print(f"    0x{rva:x} ends 0x{end:x} on `{mnemonic}`")
    if cats["violation"] > TERMINATOR_BUDGET:
        failures.append(f"{cats['violation']} terminator violations "
                        f"(budget {TERMINATOR_BUDGET})")

    spans = sorted((rva, rva + size) for rva, size, _r in funcs)
    residue, bad_gaps = partition_residue(image, spans)
    print(f"[carve audit] partition residue: {residue} non-pad bytes outside "
          f"extents in {len(bad_gaps)} gaps (budget {RESIDUE_BUDGET})")
    for lo, hi, bad in sorted(bad_gaps, key=lambda g: -g[2])[:5]:
        print(f"    gap 0x{lo:x}..0x{hi:x}: {bad} bytes")
    if residue > RESIDUE_BUDGET:
        failures.append(f"partition residue {residue} > {RESIDUE_BUDGET}")

    # residue gaps as MISSED-FUNCTION candidates (report-only): the packed
    # /Gy layout means a post-padding gap start is usually an uncarved entry
    import csv
    a1_entries = set()
    a1_path = ATTEMPT1 / "functions.csv"
    if a1_path.is_file():
        with a1_path.open(newline="") as fh:
            a1_entries = {int(r["entry_rva"], 16) for r in csv.DictReader(fh)}
    cand_rows = gap_candidates(image, spans, a1_entries)
    common.write_tsv(common.CARVE_DIR / "gap_candidates.tsv",
                     "homm3.carve.audit",
                     ["candidate_rva", "max_size", "aligned16", "prologue_like",
                      "attempt1_entry", "first_bytes"], cand_rows,
                     ["# report-only: likely uncarved functions in residue "
                      "gaps; NOT promoted, review before use"])
    strong = sum(1 for r in cand_rows if r[2] and r[3])
    print(f"[carve audit] gap candidates: {len(cand_rows)} residue starts "
          f"({strong} aligned+prologue-like, "
          f"{sum(1 for r in cand_rows if r[4])} corroborated by attempt-1) "
          "-> gap_candidates.tsv")

    # --- negative control: body-only sizes must trip the gate -----------
    ghidra_rows = common.read_tsv(common.CARVE_DIR / "ghidra_functions.tsv")
    body_end = {}
    for r in ghidra_rows:
        entry = int(r["entry_rva"], 16)
        body_end[entry] = max(int(c.split("-")[1], 16)
                              for c in r["body_ranges"].split(";"))
    neg_spans = sorted((rva, min(rva + size, body_end.get(rva, rva + size)))
                       for rva, size, _r in funcs)
    neg_residue, _ = partition_residue(image, neg_spans)
    if neg_residue <= RESIDUE_BUDGET:
        failures.append(
            f"NEGATIVE CONTROL FAILED: body-only residue {neg_residue} "
            f"passes the {RESIDUE_BUDGET} budget - the gate cannot fail")
    else:
        print(f"[carve audit] negative control: body-only sizes leak "
              f"{neg_residue} residue bytes > budget (gate trips) OK")

    # --- EH funclet completeness ----------------------------------------
    func_infos, actions, missing = gate_eh_funclets(image, entries)
    print(f"[carve audit] EH: {func_infos} FuncInfos, {len(actions)} distinct "
          f"unwind action targets, {len(missing)} not at entries")
    if missing:
        for rva in missing[:10]:
            print(f"    action target 0x{rva:x} not an entry")
        failures.append(f"{len(missing)} unwind actions lack entries")

    # --- sanity pins ------------------------------------------------------
    seed_rows = common.read_tsv(common.CARVE_DIR / "seed_log.tsv")
    init_slots = sum(1 for r in seed_rows
                     if r["iter"] == "1" and r["source"] == "init-array")
    dword_tables = sum(1 for r in tables if r["kind"] == "dword")
    print(f"[carve audit] pins: init-array {init_slots} slots, "
          f"{dword_tables} dword tables, "
          f"{sum(1 for r in tables if r['kind'] == 'byte')} byte tables")
    if init_slots < 1000:
        failures.append(f"init-array {init_slots} < 1000")
    if not 250 <= dword_tables <= 450:
        failures.append(f"dword table count {dword_tables} not near "
                        "attempt-1's 334")
    vt_detail = common.read_tsv(common.CARVE_DIR / "vtables_detail.tsv")
    uncovered = sum(int(r["uncovered_targets"]) for r in vt_detail
                    if r["classification"] == "vtable")
    if uncovered:
        failures.append(f"{uncovered} uncovered vtable slots")

    # --- report-only cross-checks ----------------------------------------
    cross_check_attempt1(funcs, tables)

    if failures:
        for failure in failures:
            print(f"[carve audit] FATAL: {failure}", file=sys.stderr)
        return 1

    rows = [(f"0x{rva:x}", size) for rva, size, _r in funcs]
    common.write_tsv(FUNCTIONS_TSV, "homm3.carve.audit", ["rva", "size"], rows,
                     [f"# {len(rows)} functions; size INCLUDES jump tables"])
    print(f"[carve audit] all gates green -> {FUNCTIONS_TSV.name} "
          f"({len(rows)} functions)")
    if "--emit-config" in argv:
        emit_config(funcs)
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
