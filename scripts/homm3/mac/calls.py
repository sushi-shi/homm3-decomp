"""Static PowerPC call-site comparisons for the two Mac code streams.

Counts describe emitted instructions, never dynamic invocation counts. Indirect
targets remain unknown; equal counts or branch forms do not prove an inline
decision. Candidate MWOB references can be reported before linking succeeds.
"""
from __future__ import annotations

from collections import Counter
from difflib import SequenceMatcher

from homm3.mac.object import ObjectError
from homm3.mac.relocations import Address, CallTarget, call_target


def _signed(value, bits):
    return value - (1 << bits) if value & (1 << (bits - 1)) else value


def address_key(address: Address) -> str:
    return f"mac:{address.section}:0x{address.offset:x}"


def analyze(data: bytes, origin: Address, symbols: dict[str, Address | CallTarget], *,
            xrefs=None, labels: dict[str, str] | None = None) -> dict:
    if len(data) % 4:
        raise ObjectError("call census requires complete PowerPC instructions")
    object_code = xrefs is not None
    refs = {}
    for offset, kind, symbol in xrefs or ():
        if offset in refs or offset % 4 or not 0 <= offset < len(data):
            raise ObjectError("invalid or duplicate call-census relocation")
        refs[offset] = (kind, symbol)
    destinations = {name: call_target(at) for name, at in symbols.items()}
    names = {address_key(at.address): name for name, at in destinations.items()}
    labels = labels or {}
    calls, local_links, external_branches = [], [], []
    for offset in range(0, len(data), 4):
        word = int.from_bytes(data[offset:offset + 4], "big")
        op, link = word >> 26, bool(word & 1)
        indirect = op == 19 and ((word >> 1) & 0x3ff) in (16, 528)
        if op not in (16, 18) and not indirect:
            continue
        # bclr without LK is a return/conditional return; it is not a call.
        if indirect and not link and ((word >> 1) & 0x3ff) == 16:
            continue
        bo, bi = (word >> 21) & 31, (word >> 16) & 31
        condition = "always" if op == 18 or bo & 20 == 20 else f"bo={bo},bi={bi}"
        kind = "direct"
        target = symbol = None
        unresolved = False
        local = False
        if indirect:
            kind = "indirect_ctr" if ((word >> 1) & 0x3ff) == 528 else "indirect_lr"
        elif offset in refs:
            ref_kind, symbol = refs[offset]
            if ref_kind != "HUNK_XREF_24BIT" or op != 18 or not symbol:
                raise ObjectError(f"unsupported call relocation at +{offset:#x}: {ref_kind}")
            resolved = destinations.get(symbol)
            target = address_key(resolved.address) if resolved else f"symbol:{symbol}"
            unresolved = resolved is None
        else:
            bits = 26 if op == 18 else 16
            delta = _signed(word & (((1 << bits) - 1) & ~3), bits)
            absolute = bool(word & 2)
            destination = delta if absolute else origin.offset + offset + delta
            local = not absolute and origin.offset <= destination < origin.offset + len(data)
            if object_code and not local:
                target = f"object:{destination - origin.offset:+#x}"
                unresolved = True
            elif absolute:
                target = f"absolute:0x{destination & 0xffffffff:x}"
                unresolved = True  # PEF sections have no fixed runtime VA.
            else:
                target = address_key(Address(origin.section, destination))
                symbol = names.get(target)
            if local and destination == origin.offset:
                local = False  # Recursive call to the function's own entry.
        physical_target = target
        callee = destinations.get(symbol)
        if callee and callee.kind == "indirect_tvector":
            kind, target = "indirect_tvector", None
        elif callee and callee.kind == "import":
            target = callee.import_identity
        label = labels.get(symbol, symbol) if symbol else target or kind
        if kind == "indirect_tvector":
            label = f"unknown function pointer via {symbol}"
        elif callee and callee.kind == "import":
            label = target
        site = {"offset": offset, "kind": kind, "target": target, "symbol": symbol,
                "label": label, "condition": condition, "unresolved": unresolved,
                "physical_target": physical_target}
        if link:
            # Local linked branches may recover PC or enter an internal stub;
            # list them separately instead of claiming another function call.
            (local_links if local else calls).append(site)
        elif not local:
            # Potential tail calls/dispatch branches; do not infer a callee.
            external_branches.append(site)
    counts = Counter(site["kind"] for site in calls)
    return {"offset_basis": "object" if object_code else "linked",
            "total": len(calls), "direct": counts["direct"],
            "indirect": counts["indirect_ctr"] + counts["indirect_lr"] + counts["indirect_tvector"],
            "unresolved_direct": sum(site["unresolved"] for site in calls),
            "unique_direct_targets": len({site["target"] for site in calls if site["kind"] == "direct"}),
            "sites": calls, "local_link_branches": local_links,
            "external_branches": external_branches}


def _identity(site):
    return site["kind"], site["target"], site["condition"]


def compare(retail: dict, candidate: dict | None, *, error: str | None = None) -> dict:
    if candidate is None:
        return {"retail": retail, "candidate": None, "delta": None,
                "state": "candidate_unavailable", "differences": [], "error": error}
    left = [_identity(site) for site in retail["sites"]]
    right = [_identity(site) for site in candidate["sites"]]
    differences = []
    for operation, a, b, c, d in SequenceMatcher(a=left, b=right, autojunk=False).get_opcodes():
        if operation != "equal":
            differences.append({"operation": operation, "retail": retail["sites"][a:b],
                                "candidate": candidate["sites"][c:d]})
    delta = candidate["total"] - retail["total"]
    if delta:
        state = "call_count_difference"
    elif retail["unresolved_direct"] or candidate["unresolved_direct"]:
        state = "unresolved_call_targets"
    elif differences:
        state = "call_target_difference"
    elif retail["indirect"] or candidate["indirect"]:
        state = "indirect_targets_unknown"
    else:
        state = "call_sequence_agrees"
    tails_differ = ([_identity(s) for s in retail["external_branches"]]
                    != [_identity(s) for s in candidate["external_branches"]])
    return {"retail": retail, "candidate": candidate, "delta": delta,
            "state": state, "differences": differences,
            "external_branches_differ": tails_differ, "error": error}


def totals(comparisons: list[dict]) -> dict:
    def side(name):
        views = [row[name] for row in comparisons if row[name] is not None]
        return {"functions": len(views), **{
            key: sum(row[key] for row in views)
            for key in ("total", "direct", "indirect", "unresolved_direct")}}
    return {"retail": side("retail"), "candidate": side("candidate"),
            "states": dict(Counter(row["state"] for row in comparisons))}


def summary(comparison: dict) -> str:
    retail, candidate = comparison["retail"], comparison["candidate"]
    def show(view):
        return "unavailable" if view is None else f"{view['total']} ({view['direct']} direct, {view['indirect']} indirect)"
    delta = "?" if comparison["delta"] is None else f"{comparison['delta']:+d}"
    return (f"calls retail {show(retail)} / candidate {show(candidate)}; "
            f"delta {delta}; {comparison['state']}")


def render(comparison: dict) -> list[str]:
    lines = [summary(comparison)]
    for name in ("retail", "candidate"):
        view = comparison[name]
        if view is None:
            continue
        lines.append(f"  {name} ({view['offset_basis']} offsets):")
        for site in view["sites"]:
            destination = site["label"]
            if site["symbol"] and site["target"]:
                destination += f" [{site['target']}]"
            lines.append(f"    +0x{site['offset']:x} {site['kind']}: {destination} ({site['condition']})")
        if not view["sites"]:
            lines.append("    no call instructions")
        if view["local_link_branches"] or view["external_branches"]:
            lines.append(f"    separately: {len(view['local_link_branches'])} local linked branches; "
                         f"{len(view['external_branches'])} external branches (possible tail calls)")
    for difference in comparison["differences"]:
        lhs = ", ".join(site["label"] for site in difference["retail"]) or "—"
        rhs = ", ".join(site["label"] for site in difference["candidate"]) or "—"
        lines.append(f"  {difference['operation']}: retail [{lhs}] -> candidate [{rhs}]")
    if comparison.get("error"):
        lines.append(f"  unavailable: {comparison['error']}")
    return lines
