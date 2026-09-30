"""Exception graph identities with reviewed anonymous-namespace spellings.

VC6 embeds the build path in RTTI. A reviewed namespace spelling can establish
the identity of that type and its exception graph, but cannot establish the
raw candidate descriptor's bytes or extent. Projections below are disposable;
only address identities leave this module, never data-manifest/accounting rows.
"""
from collections import defaultdict
from dataclasses import replace
from hashlib import sha256

from homm3.compare.canonicalize import anon_ns_stamp_inputs, reviewed_anon_ns_name
from homm3.core import compile_receipt
from homm3.delink import exception_data


def project_records(records):
    """Project supported raw records for identity proofs, never byte credit."""
    result = []
    for record in records:
        if not exception_data.supported(record):
            continue
        name = reviewed_anon_ns_name(record.name, record.unit)
        payload = record.payload
        if name != record.name and name.startswith('??_R0'):
            payload = payload[:8] + b'.' + name[5:-2].encode('ascii') + b'\0'
        result.append(replace(record, name=name, payload=payload,
            relocations=tuple((off, reviewed_anon_ns_name(target, record.unit), kind)
                              for off, target, kind in record.relocations)))
    return result


class CandidateNames:
    """Reviewed names over a raw code witness; no instruction bytes change."""
    def __init__(self, candidate, unit):
        self.candidate, self.unit = candidate, unit
        self.symbols = defaultdict(list)
        for name in candidate.symbols:
            self.symbols[reviewed_anon_ns_name(name, unit)].append(name)

    def body(self, name):
        names = self.symbols.get(name, ())
        if len(names) != 1:
            return None
        body = self.candidate.body(names[0])
        if body is None:
            return None
        return replace(body, name=name,
            relocations=tuple((off, reviewed_anon_ns_name(target, self.unit), kind)
                              for off, target, kind in body.relocations))


def policy_snapshot():
    return compile_receipt.snapshot(anon_ns_stamp_inputs().values())


def witnesses_current(witnesses, base_dir, policy):
    """Recheck source, object and namespace policy after the complete proof."""
    inputs = {}
    for obj, expected in witnesses.values():
        for path, digest in expected.items():
            if path in inputs and inputs[path] != digest:
                return False
            inputs[path] = digest
    try:
        return (policy_snapshot() == policy
                and compile_receipt.snapshot(inputs) == inputs
                and all(compile_receipt.digest(base_dir / f'{unit}.obj')
                        == sha256(obj.buf).hexdigest()
                        for unit, (obj, _) in witnesses.items()))
    except OSError:
        return False


def address_identities(model, project, base_dir, defined):
    """Return uniquely anchored graph names, without claiming storage."""
    from homm3.delink.image import retail
    if not base_dir.is_dir() or not (project.toolchain / 'bin').is_dir():
        return []
    policy = policy_snapshot()
    witnesses = {}
    records, _ = exception_data.candidates(project, base_dir, witnesses=witnesses)
    known = defaultdict(set)
    for b in model.functions + model.data:
        for entry in (b, *b.aliases):
            if entry.name and entry.channel:
                known[entry.name].add(b.rva)
    for rva, names in defined.items():
        for name in names:
            known[name].add(rva)
    matched, _ = exception_data.resolve(project_records(records), retail(), known)
    if not witnesses_current(witnesses, base_dir, policy):
        return []
    return [(rva, record.name, 'exception-identity',
             'type-anchored exception graph; identity only', record.unit)
            for record, rva in matched]
