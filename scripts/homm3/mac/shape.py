"""Compare CodeWarrior instructions while leaving relocations unresolved.

This is a source-shape diagnostic, never an exact Mac byte verdict. In
particular it cannot establish callee or TOC identities, or branch targets.
"""
from __future__ import annotations

from difflib import SequenceMatcher

from homm3.mac import build, pairing
from homm3.mac.object import parse_code_hunks, select_hunk
from homm3.mac.source import Pair


def _key(word: int) -> int:
    opcode = word >> 26
    if opcode == 18:  # b/bl: keep opcode, AA and LK; ignore displacement
        return word & 0xfc000003
    if opcode == 16:  # bc: keep BO, BI, AA and LK
        return word & 0xffff0003
    if opcode in (14, 32, 34, 40, 42, 48, 50) and (word >> 16) & 31 == 2:
        return word & 0xffff0000  # TOC displacement; keep registers/opcode
    return word


def _words(data: bytes) -> list[int]:
    if len(data) % 4:
        raise ValueError("unaligned PowerPC function body")
    return [int.from_bytes(data[at:at + 4], "big") for at in range(0, len(data), 4)]


def inspect(pair, pef, tools_dir) -> dict:
    if pair.retail_va is None:
        raise ValueError("Mac shape needs a Windows VA source claim")
    source_pair = (pair if isinstance(pair, Pair) else
                   pairing.candidate(build.ROOT, pair.retail_va, pair.unit))
    compiled = build.compile_pair(source_pair, tools_dir)
    hunk = compiled.hunk
    if hunk is None:
        listing = (build.object_directory(build.ROOT, source_pair) / "candidate.dis.txt").read_text()
        hunks = parse_code_hunks(listing)
        if pair.mac_symbol:
            hunk = select_hunk(listing, pair.mac_symbol)
        elif len(hunks) == 1:
            hunk = hunks[0]
        else:
            raise ValueError(f"compile probe emitted {len(hunks)} code hunks; "
                             "record the reviewed mac_symbol on this address reference")
    candidate = _words(hunk.data)
    target = _words(pef.code(pair.mac_section, pair.mac_offset, pair.mac_size))
    removed = []
    for at, kind, _ in hunk.xrefs:
        if kind != "HUNK_XREF_24BIT":
            continue
        index = at // 4
        if (index + 1 < len(candidate) and candidate[index] & 0xfc000003 == 0x48000001
                and candidate[index + 1] == 0x60000000):
            removed.append(index + 1)
    if len(set(removed)) != len(removed):
        raise ValueError("overlapping CodeWarrior reload slots")
    skipped = set(removed)
    source = [(at * 4, word, _key(word)) for at, word in enumerate(candidate)
              if at not in skipped]
    retail = [(at * 4, word, _key(word)) for at, word in enumerate(target)]
    matcher = SequenceMatcher(None, [row[2] for row in source],
                              [row[2] for row in retail], autojunk=False)
    changes = []
    equal = 0
    for tag, a0, a1, b0, b1 in matcher.get_opcodes():
        if tag == "equal":
            equal += a1 - a0
        else:
            changes.append({"kind": tag,
                            "candidate_offset": f"0x{source[a0][0]:x}" if a0 < len(source) else "end",
                            "retail_offset": f"0x{retail[b0][0]:x}" if b0 < len(retail) else "end",
                            "candidate": [f"{row[1]:08x}" for row in source[a0:a1]],
                            "retail": [f"{row[1]:08x}" for row in retail[b0:b1]]})
    candidate_calls = [(f"0x{at:x}", name) for at, kind, name in hunk.xrefs
                       if kind == "HUNK_XREF_24BIT"]
    retail_calls = []
    for at, word in enumerate(target):
        if word >> 26 != 18 or not word & 1:
            continue
        displacement = word & 0x03fffffc
        if displacement & 0x02000000:
            displacement -= 0x04000000
        destination = (displacement if word & 2 else
                       pair.mac_offset + at * 4 + displacement)
        retail_calls.append((f"0x{at * 4:x}", f"0x{destination:x}"))
    return {"scope": "relocation_masked_instruction_shape_only",
            "exact_verdict": None,
            "candidate_symbol": hunk.name,
            "limitations": ["call destinations and TOC identities are not compared",
                            "branch destinations are masked",
                            "direct-call NOP reload slots are assumed collapsed"],
            "candidate_bytes": len(hunk.data), "retail_bytes": pair.mac_size,
            "candidate_instructions_after_collapse": len(source),
            "retail_instructions": len(retail),
            "assumed_removed_reload_slots": [f"0x{at * 4:x}" for at in removed],
            "aligned_equal_instructions": equal,
            "candidate_calls": candidate_calls, "retail_calls": retail_calls,
            "changes": changes,
            "source_hash": compiled.source_hash, "object_hash": compiled.object_hash}


def render(report: dict) -> str:
    lines = ["[mac] relocation-masked instruction shape; no exact Mac verdict",
             f"  candidate {report['candidate_bytes']} bytes / "
             f"{report['candidate_instructions_after_collapse']} instructions after assumed reload collapse",
             f"  retail    {report['retail_bytes']} bytes / {report['retail_instructions']} instructions",
             f"  aligned identical instructions: {report['aligned_equal_instructions']}",
             f"  direct calls: candidate {len(report['candidate_calls'])}, "
             f"retail {len(report['retail_calls'])}",
             f"  unresolved regions: {len(report['changes'])}"]
    for row in report["changes"][:20]:
        left = " ".join(row["candidate"][:5]) or "∅"
        right = " ".join(row["retail"][:5]) or "∅"
        lines.append(f"    {row['kind']} candidate {row['candidate_offset']} [{left}] "
                     f"| retail {row['retail_offset']} [{right}]")
    if len(report["changes"]) > 20:
        lines.append(f"    … {len(report['changes']) - 20} more regions (use --json)")
    for index, (candidate, retail) in enumerate(zip(report["candidate_calls"],
                                                     report["retail_calls"])):
        lines.append(f"  call {index}: candidate {candidate[0]} {candidate[1]} "
                     f"| retail {retail[0]} -> {retail[1]}")
    return "\n".join(lines)
