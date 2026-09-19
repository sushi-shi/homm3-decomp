#!/usr/bin/env python3
"""Audit one map-object pointer/reference claim alias in comparison objects.

The raw ABI argument remains one address. Only the exact addObject nominal name
is changed for comparison; raw objects, section payloads, relocation records,
other symbol identities and claims remain untouched. This is an experiment,
not a declaration that pointer and reference C++ contracts are interchangeable.
"""
import json
import csv
import hashlib
import sys
from pathlib import Path

from homm3.build.canonicalize_data_symbols import CoffObject, _rewrite_names
from homm3.vc6 import source_families

REFERENCE = '?addObject@type_random_map@@QAEXAAVtype_object@@UTRmgMapPosition@@@Z'
VALUE = '?addObject@type_random_map@@QAEXPAVtype_object@@UTRmgMapPosition@@@Z'


def nominal_object(payload, target_name=VALUE):
    original = CoffObject(payload)
    renames = {}
    rows = []
    destinations = {}
    for symbol in original.symbols.values():
        name = target_name if symbol.name in (REFERENCE, VALUE) else symbol.name
        if name in destinations and destinations[name] != symbol.name:
            raise ValueError("nominal alias collision: " + name)
        destinations[name] = symbol.name
        if name != symbol.name:
            renames[symbol.index] = name
            rows.append({"index": symbol.index, "from": symbol.name, "to": name,
                         "section": symbol.section, "value": symbol.value,
                         "type": symbol.typ, "storage": symbol.storage_class})
    if not renames:
        return payload, rows
    result = _rewrite_names(original, renames)
    rewritten = CoffObject(result)
    assert len(original.sections) == len(rewritten.sections)
    for before, after in zip(original.sections, rewritten.sections):
        assert before == after
        assert original.section_bytes(before) == rewritten.section_bytes(after)
    assert original.relocations == rewritten.relocations
    assert original.symbols.keys() == rewritten.symbols.keys()
    for index, before in original.symbols.items():
        after = rewritten.symbols[index]
        assert (before.index, before.offset, before.value, before.section,
                before.typ, before.storage_class, before.aux_count) == (
                    after.index, after.offset, after.value, after.section,
                    after.typ, after.storage_class, after.aux_count)
        assert after.name == renames.get(index, before.name)
        aux_start = before.offset + 18
        aux_end = aux_start + before.aux_count * 18
        assert original.data[aux_start:aux_end] == rewritten.data[aux_start:aux_end]
    return result, rows


def main():
    manifest = json.loads(Path(sys.argv[1]).read_text())
    assert manifest['diagnostic_runner_sha256'] == hashlib.sha256(Path(__file__).read_bytes()).hexdigest()
    compile_original = source_families.compile_candidate
    root = source_families.common.HOMM3_DIR
    baseline = CoffObject((root / 'build/objdiff/base/rmg.obj').read_bytes())
    query_names = {s.name for s in baseline.symbols.values()
                   if s.section > 0 and s.typ == 32 and s.name in (REFERENCE, VALUE)}
    assert len(query_names) == 1, query_names
    target_name = next(iter(query_names))
    retail_rvas = {}
    with (root / "build/gen/symbol_names.csv").open() as stream:
        for row in csv.reader(stream):
            if len(row) >= 3:
                retail_rvas.setdefault((row[2], row[1]), []).append(row[0])

    def compile_with_aliases(candidate_root, unit, output):
        raw_path = compile_original(candidate_root, unit, output)
        payload, aliases = nominal_object(raw_path.read_bytes(), target_name)
        for alias in aliases:
            alias["retail_rvas"] = retail_rvas.get((unit, alias["to"]), [])
        original_unit = CoffObject((root / "build/objdiff/base" / (unit + ".obj")).read_bytes())
        baseline_names = {s.name for s in original_unit.symbols.values()
                          if s.section > 0 and s.typ == 32}
        view = CoffObject(payload)
        new_definitions = sorted({s.name for s in view.symbols.values()
                                  if s.section > 0 and s.typ == 32} - baseline_names)
        path = raw_path.with_name("candidate.nominal.obj")
        path.write_bytes(payload)
        (Path(output) / "nominal-aliases.json").write_text(json.dumps({
            "raw_object": raw_path.name, "comparison_object": path.name,
            "section_bytes_unchanged": True, "relocations_unchanged": True,
            "symbol_identity_unchanged_except_names": True,
            "alias_count": len(aliases), "alias_collisions": [],
            "unmapped_new_definitions": new_definitions,
            "aliases": aliases,
        }, indent=2) + "\n")
        return path

    source_families.compile_candidate = compile_with_aliases
    return source_families.main()


if __name__ == "__main__":
    raise SystemExit(main())
