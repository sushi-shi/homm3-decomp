"""Project a canonical header VA onto its comparison object.

The header owns the declaration/name; the chosen object is only the comparison
carrier. If its body stops emitting and no unique replacement is established,
a banked RVA keeps its existing carrier so missing code remains measurable.
New claims require a unique emitter or a unique neighbouring retail
contribution. No independent header-symbol ledger exists.
"""
from pathlib import Path

from homm3.core import common


def claim_files(root: Path = common.HOMM3_DIR) -> list[Path]:
    from homm3.retail_labels import source
    head, _arity, _prototype = source.MACRO_HEADS['VA']
    return [path for path in sorted((root / 'include').rglob('*'))
            if path.is_file() and path.suffix.lower() in {'.h', '.hpp', '.inl'}
            and head.search(source.mask_lexical_noise(path.read_text(errors='replace')))]


def choose_carrier(rva: int, emitters: set[str], banked: dict[int, str],
                   anchors: list[tuple[int, str]]) -> str | None:
    if banked.get(rva) in emitters:
        return banked[rva]
    if len(emitters) == 1:
        return next(iter(emitters))
    if not emitters:
        return banked.get(rva)
    lower = [a for a in anchors if a[0] < rva]
    upper = [a for a in anchors if a[0] > rva]
    neighbours = set()
    if lower:
        neighbours.add(max(lower)[1])
    if upper:
        neighbours.add(min(upper)[1])
    plausible = neighbours & emitters
    return next(iter(plausible)) if len(plausible) == 1 else banked.get(rva)


def project(paths: list[Path], functions: set[int], ir_maps: dict,
            rows_by_unit: dict, problems: list[str]) -> None:
    from homm3.retail_labels import source
    from homm3.match.status import load_baseline
    from homm3.match.source_ownership import collect, claim_definitions
    if not paths:
        return
    definitions, errors, _reached = collect()
    problems.extend(f"{error} (FATAL)" for error in errors)
    # Clang omits annotate metadata for some linkonce_odr inline definitions
    # in LLVM IR. Its AST still attaches the annotation to the real body.
    # Prefer an object confirming the AST's exact mangled name. An existing
    # comparison binding can also measure a body which has stopped emitting;
    # it supplies only the carrier, never the source name.
    header_names = {d.va - common.IMAGE_BASE: d.mangled for d in claim_definitions(definitions)
                    if d.file.startswith('include/') and d.va is not None}
    banked = {row.rva: unit for (unit, _name), row in load_baseline().items()
              if row.rva is not None and unit in ir_maps}
    anchors = [(row['rva'], unit) for unit, rows in rows_by_unit.items()
               for row in rows if row['kind'] == 'func']
    authorities = {unit: {name for group in source._base_authority_names(unit).values()
                          for name, _size in group} for unit in ir_maps}
    for path in paths:
        for row in source.scan_file(path, functions, problems):
            if row['channel'] != 'src-VA':
                continue
            rva = row['rva']
            mangled = header_names.get(rva)
            if not mangled:
                problems.append(
                    f"header VA(0x{rva + common.IMAGE_BASE:08x}) in {path.name}: "
                    f"no annotated active definition supplies its identity (FATAL)")
                continue
            emitters = {unit: mangled for unit in ir_maps
                        if mangled is not None and mangled in authorities[unit]}
            carrier = choose_carrier(rva, set(emitters), banked, anchors)
            if carrier is None:
                problems.append(
                    f"header VA(0x{rva + common.IMAGE_BASE:08x}) in {path.name}: "
                    f"no unique VC6 comparison carrier among {sorted(emitters)} (FATAL)")
                continue
            row['unit'] = carrier
            row['joined'] = mangled
            row['channel'] = 'src-VA+base' if carrier in emitters else 'src-VA+ir'
            if carrier not in emitters:
                problems.append(
                    f"header VA(0x{rva + common.IMAGE_BASE:08x}) in {path.name}: "
                    f"{carrier}.obj emits no {mangled!r}; retaining it as the "
                    f"banked comparison carrier to report the missing body")
            content = {name: size for group in source._base_authority_names(carrier).values()
                       for name, size in group}
            source._report_size_mismatch(carrier, row, row['joined'], content, problems)
            rows_by_unit[carrier].append(row)
