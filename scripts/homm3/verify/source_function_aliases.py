"""Prove folded function names from ordinary C++ exception emissions.

Identical candidate bodies alone do not identify a retail function. An alias
also needs a complete exact retail body with named references, and a complete
type-identified exception graph that actually points to that body. Reviewed
namespace projections prove identities only, not descriptor bytes. The model
keeps the resulting name on its existing function; no extra body is claimed.
"""
from collections import defaultdict

from homm3.core import msvc_names
from homm3.delink import exception_data, exception_identities
from homm3.verify.startup_bodies import Candidate, bindings, match


def infer(model, records, candidates, image):
    known = defaultdict(set)
    functions = defaultdict(set)
    sizes = {b.rva: b.size for b in model.functions}
    for b in model.functions + model.data:
        for entry in (b, *b.aliases):
            if entry.name and entry.channel:
                known[entry.name].add(b.rva)
                if b.space == 'text':
                    functions[msvc_names.mask(entry.name)].add(b.rva)
    needed = defaultdict(set)
    for record in records:
        for _, name, _ in record.relocations:
            if name.startswith(('??0', '??1')) and name not in known:
                needed[record.unit].add(name)

    proposed, witnesses = defaultdict(set), defaultdict(list)
    for unit, names in sorted(needed.items()):
        candidate = candidates.get(unit)
        if candidate is None:
            continue
        pending = {name: candidate.body(name) for name in names}
        pending = {name: body for name, body in pending.items() if body}
        if not pending:
            continue
        wanted = {(b.payload, b.relocations) for b in pending.values()}
        lengths = {len(b.payload) for b in pending.values()}
        targets, _ = bindings(model, [], unit, list(candidate.symbols))
        exact = defaultdict(set)
        for name in list(candidate.symbols):
            addresses = {rva for rva in functions.get(msvc_names.mask(name), ())
                         if sizes[rva] in lengths}
            if not addresses:
                continue
            body = candidate.body(name)
            signature = (body.payload, body.relocations) if body else None
            if signature not in wanted:
                continue
            for rva in addresses:
                if match(candidate, name, rva, image, sizes, targets)[0] == 'exact':
                    exact[signature].add(rva)
        for name, body in pending.items():
            for rva in exact.get((body.payload, body.relocations), ()):
                proposed[name].add(rva)
                witnesses[name].append(unit)

    # Do not pick one address from multiple byte-identical representatives.
    proposed = {name: addresses for name, addresses in proposed.items()
                if len(addresses) == 1}
    matches, _ = exception_data.resolve(records, image, {**known, **proposed})
    anchored = defaultdict(list)
    for record, rva in matches:
        for _, name, _ in record.relocations:
            if name in proposed and record.unit in witnesses[name]:
                anchored[name].append((record.unit, record.name, rva))
    return [(name, next(iter(proposed[name])), *min(proofs))
            for name, proofs in sorted(anchored.items())]


def recover(model, project, base_dir):
    """Return model aliases only while all source/object witnesses stay fresh."""
    from homm3.delink.image import retail
    from homm3.retail_labels import Claim

    if not base_dir.is_dir() or not (project.toolchain / 'bin').is_dir():
        return []
    witnesses = {}
    policy = exception_identities.policy_snapshot()
    records, _ = exception_data.candidates(project, base_dir, witnesses=witnesses)
    candidates = {unit: exception_identities.CandidateNames(Candidate(obj), unit)
                  for unit, (obj, _) in witnesses.items()}
    proved = infer(model, exception_identities.project_records(records), candidates, retail())
    if not proved:
        return []
    # Recheck the complete witness set, with new input hashes. A change during
    # comparison cannot publish names from a mixture of old and new objects.
    if not exception_identities.witnesses_current(witnesses, base_dir, policy):
        return []
    sizes = {b.rva: b.size for b in model.functions}
    return [Claim(rva, name, 'func', 'source-folded-exact', sizes[rva], unit,
                  {'record': record, 'record_rva': hex(record_rva)})
            for name, rva, unit, record, record_rva in proved]
