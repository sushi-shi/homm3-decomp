#!/usr/bin/env python3
"""homm3.build.data_manifest - the delinker's data-side tsv inputs.

Schemas are byte-exact against the PINNED vostok (1393e24; they differ
from the sibling projects' older pins - notably the data manifest is
8-column and carries no name: symbol identities come from the synth PDB,
the manifest only assigns ownership/extent/topology):

  build/gen/delink_data_manifest.tsv     --data-manifest
      object rva size storage alignment section_ordinal section_offset scope
      (src/data_manifest.rs)  Initial rows: the retail vtables - the one
      data family with admitted extents (slots * 4) - owned by a synthetic
      `vtables.c` unit until per-class home TUs are assigned.
  build/gen/delink_data_sections.tsv     --data-section-manifest
      object ordinal name rva size alignment characteristics checksum
      comdat_selection associative_ordinal storage
      (src/data_section_manifest.rs)  Header-only until candidate data
      topology starts; packed sections are vostok's default.
  build/gen/delink_data_bindings.tsv     NOT a vostok input - the
      canonicalizer's DATA_COMPGEN binding table (homm2 10-column shape:
      name object rva size storage alignment section_ordinal
      section_offset scope provenance). Fresh source-to-COFF bindings
      include DATA, narrow literals and owner-proved byte guards. The
      ordinary comparison remains code-only pending strict enrollment;
      binding does not change data byte scores or target topology.

config/retail/reloc-aliases.tsv (--reloc-alias-manifest, header
`function_rva target_rva site_rva owner addend occurrences`,
src/reloc_alias_manifest.rs) is hand-owned/reviewed; this stage creates it
once if absent and never rewrites it.
"""
from __future__ import annotations

import csv
import sys

from pathlib import Path

from homm3.core import common
from homm3.core import tsv

DATA_OUT = common.HOMM3_DIR / "build/gen/delink_data_manifest.tsv"
SECTIONS_OUT = common.HOMM3_DIR / "build/gen/delink_data_sections.tsv"
BINDINGS_OUT = common.HOMM3_DIR / "build/gen/delink_data_bindings.tsv"
ALIASES = common.HOMM3_DIR / "config/retail/reloc-aliases.tsv"
VTABLES = common.HOMM3_DIR / "config/retail/vtables.tsv"

# byte-exact vostok 1393e24 headers
DATA_HEADER = ("object\trva\tsize\tstorage\talignment\t"
               "section_ordinal\tsection_offset\tscope")
SECTIONS_HEADER = ("object\tordinal\tname\trva\tsize\talignment\t"
                   "characteristics\tchecksum\tcomdat_selection\t"
                   "associative_ordinal\tstorage")
ALIASES_HEADER = ("function_rva\ttarget_rva\tsite_rva\towner\taddend\t"
                  "occurrences")
BINDINGS_HEADER = ("name\tobject\trva\tsize\tstorage\talignment\t"
                   "section_ordinal\tsection_offset\tscope\tprovenance")


def binding_rows(report, names):
    """Canonicalizer inputs use candidate topology, never guessed retail spans.

    One physical pool allocation may have many annotated uses. Give it the
    generated retail model's shared semantic identity, retaining every use in
    data-bindings.tsv. Ambiguities stay out of the canonicalizer input.
    """
    candidates = {r['id']: r for r in report['candidate_data']}
    rows, seen, issues = [], set(), []
    for binding in report['data_bindings']:
        if binding['status'] != 'bound':
            continue
        row = candidates[binding['candidate_ids'][0]]
        key = (row['id'], binding['rva'], binding['size'])
        if key in seen:
            continue
        seen.add(key)
        name = names.get(binding['rva']) if binding['macro'] != 'DATA' else (
            row['symbols'][0] if len(row['symbols']) == 1 else None)
        if (not name or row['section_ordinal'] == 0 or len(row['scopes']) != 1 or
                row['alignment'] is None):
            issues.append(dict(binding_id=binding['id'], candidate_id=row['id'],
                               reason='missing model identity, concrete section, unique symbol or alignment'))
            continue
        rows.append(dict(name=name, object=row['unit']+'.c', rva=hex(binding['rva']),
                         size=hex(binding['size']), storage=row['storage'], alignment=hex(row['alignment']),
                         section_ordinal=str(row['section_ordinal']), section_offset=hex(row['section_offset']),
                         scope=row['scopes'][0], provenance='source-'+binding['macro']+':'+binding['source']))
    return rows, issues


def generate(candidate_report=None) -> Path:
    image, _info = common.load_image()
    if candidate_report is None:
        from homm3.analysis import candidate_data, data_declarations
        declarations = data_declarations.extract(common.HOMM3_DIR, image.image_base)
        candidate_report = candidate_data.extract(common.HOMM3_DIR, declarations)
        candidate_data.export(candidate_report, common.HOMM3_DIR/'build/gen/data')
    with (common.HOMM3_DIR/'build/gen/symbol_names.csv').open() as stream:
        names = {int(r['rva'], 0): r['name'] for r in csv.DictReader(
            line for line in stream if not line.startswith('#')) if r['kind'] == 'data'}
    bindings, binding_issues = binding_rows(candidate_report, names)
    secmap = {s.name: s for s in image.sections}
    rdata, dat = secmap[".rdata"], secmap[".data"]

    rows = []
    for line in VTABLES.open():
        if line.startswith("#") or line.startswith("rva"):
            continue
        rva_text, count = line.split("\t")[:2]
        rva, size = int(rva_text, 16), int(count) * 4
        if rdata.rva <= rva < rdata.rva + rdata.mapped:
            storage = "rdata"
        elif dat.rva <= rva < dat.rva + dat.size:
            storage = "data"
        else:
            common.die(f"vtable 0x{rva:x} outside .rdata/.data")
        rows.append(f"vtables.c\t0x{rva:x}\t0x{size:x}\t{storage}\t0x4"
                    f"\t-\t-\texternal")

    provenance = "\n".join(common.provenance("homm3.build.data_manifest"))
    DATA_OUT.parent.mkdir(parents=True, exist_ok=True)
    DATA_OUT.write_text(
        "# GENERATED - vostok --data-manifest (schema: src/data_manifest.rs"
        " @ 1393e24)\n" + provenance + "\n"
        + DATA_HEADER + "\n" + "\n".join(rows) + "\n")
    SECTIONS_OUT.write_text(
        "# GENERATED - vostok --data-section-manifest (schema: "
        "src/data_section_manifest.rs @ 1393e24); header-only until\n"
        "# candidate data topology starts - packed sections are the "
        "default.\n" + provenance + "\n" + SECTIONS_HEADER + "\n")
    tsv.write(BINDINGS_OUT, [
        '# GENERATED - fresh candidate topology for source-bound data; NOT a vostok input.',
        '# Binding does not prove retail extent or byte equality. Rejections: build/gen/data/data-bindings.tsv.',
        *provenance.splitlines()], BINDINGS_HEADER.split('\t'), bindings)
    tsv.write(BINDINGS_OUT.with_name('delink_data_binding_issues.tsv'),
              ['# Bound source sites withheld from canonicalization.'],
              ['binding_id', 'candidate_id', 'reason'], binding_issues)
    created = not ALIASES.is_file()
    if created:
        ALIASES.write_text(
            "# MANUALLY MANAGED - vostok --reloc-alias-manifest (schema: "
            "src/reloc_alias_manifest.rs @ 1393e24).\n"
            "# Reviewed rows only; site_rva may be `*`; exact rows must "
            "declare occurrences=1.\n"
            + ALIASES_HEADER + "\n")

    print(f"[build data_manifest] {len(rows)} data rows -> {DATA_OUT.name};"
          f" {len(bindings)} candidate bindings, {len(binding_issues)} withheld; aliases "
          f"{'created' if created else 'kept'}: {ALIASES.name}")
    return DATA_OUT


def main(argv=None) -> int:
    generate()
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
