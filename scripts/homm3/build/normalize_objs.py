#!/usr/bin/env python3
"""homm3.build.normalize_objs - normalize comparison copies of the objs.

Thin driver over homm3.build.canonicalize_data_symbols (the gruntz/homm2
pattern): every object under build/objdiff/base/ and build/objdiff/target/
is canonicalized into build/objdiff/normalized/{base,target}/ with a
`.symbols.tsv` sidecar next to each copy. objdiff will point ONLY at the
normalized copies once the comparison graph lands (P2.3); the raw objects
are never touched. Absent roots are tolerated - the machinery predates its
first full user by design.

Each normalized copy gets a provenance stamp recording the raw object it
came from (homm3.build.normalized_freshness), so consumers of the
disposable copies (`homm3 sema diff`) can refuse a stale one instead of
silently comparing through it. The skip decision uses the SAME verifier
the consumers use (content identity, not mtimes), so a copy this driver
skips is by construction one `homm3 sema diff` accepts - a stale stamp
can never wedge between "build says fresh" and "sema says stale".

Stamps verify data inputs, the explicitly listed transform implementation files
in implementation_inputs(), and normalized output bytes. Extend that list when
adding behavior-affecting dependencies; bump STAMP_SCHEMA when changing the
stamp format or validation contract.
Unchanged comparisons can survive a full delink without trusting timestamps.

The first canonicalization pass strips trailing COMDAT NOP fill. A linked
target sometimes has the same logical function length but necessarily keeps
one to three of those NOPs before the next 4-byte-aligned function. The paired
pass below restores exactly that target-carried fill to the base comparison
copy. It only does so when stripping the target's NOP suffix makes both logical
lengths equal; a genuinely longer or shorter function is left alone.
"""
from __future__ import annotations

import csv
from collections import Counter
import hashlib
import os
import pickle
import re
import struct
import sys
from dataclasses import dataclass, replace
from pathlib import Path

from homm3.build import canonicalize_data_symbols as canon
from homm3.build.normalized_freshness import freshness_problems, write_stamp, ValidationContext
from homm3.core import common
from homm3.core.images import path as _image_path

OBJDIFF = common.HOMM3_DIR / _image_path("build/objdiff")
COMPGEN_MANIFEST = common.HOMM3_DIR / _image_path("build/gen/compgen_claims.tsv")
DATA_MANIFEST = common.HOMM3_DIR / _image_path('build/gen/delink_data_manifest.tsv')

CNT_CODE = 0x00000020
INITIALIZED_DATA = 0x00000040
UNINITIALIZED_DATA = 0x00000080
DIR32 = 0x0006
FUNCTION_TYPE = 0x0020
EXTERNAL_STORAGE = 2
STATIC_STORAGE = 3
LABEL_STORAGE = 6
REL32 = 0x0014
TEXT_PAD_TRIM_LIMIT = 15
ASSOCIATIVE_COMDAT = 5
UNWIND_OWNER = re.compile(r"(?:^|_)unwind[0-9]+$")
SYMBOL_NAMES = common.HOMM3_DIR / _image_path("build/gen/symbol_names.csv")
ADDRESS_IDENTITIES = common.HOMM3_DIR / _image_path("build/gen/address_identities.tsv")
FUNCLETS = common.HOMM3_DIR / _image_path("config/retail/funclets.tsv")
FUNCTIONS = common.HOMM3_DIR / _image_path("config/retail/functions.tsv")


@dataclass(frozen=True)
class EhHandlerOwnerRewrite:
    """One proved direct-handler -> last-funclet+size canonicalization."""

    function: str
    parent_section: int
    child_section: int
    relocation_offset: int
    relocation_site: int
    handler_symbol: int
    funclet_symbol: int
    handler_offset: int
    funclet_offset: int
    funclet_size: int
    canonical_name: str = ""
    prologue: int = 6


def _retail_symbol_rvas(path: Path = SYMBOL_NAMES) -> dict[str, tuple[int, str]]:
    """Load the generated retail name -> (RVA, kind) authority map."""
    result: dict[str, tuple[int, str]] = {}
    if not path.is_file():
        return result
    with path.open(newline="") as stream:
        rows = (line for line in stream if not line.startswith("#"))
        for row in csv.DictReader(rows):
            name = row.get("name", "")
            if not name:
                continue
            value = (int(row["rva"], 0), row.get("kind", ""))
            previous = result.setdefault(name, value)
            if previous != value:
                raise ValueError(
                    f"conflicting retail addresses for symbol {name}: "
                    f"{previous} vs {value}")
    return result


def _retail_funclet_owners() -> dict[int, tuple[int, int]]:
    """Admitted cleanup RVA -> (parent RVA, independently admitted size)."""
    if not FUNCLETS.is_file() or not FUNCTIONS.is_file():
        return {}

    def rows(path):
        with path.open(newline="") as stream:
            yield from csv.DictReader(
                (line for line in stream if not line.startswith("#")),
                delimiter="\t")

    sizes = {}
    for row in rows(FUNCTIONS):
        rva, size = int(row["rva"], 0), int(row["size"], 0)
        if sizes.setdefault(rva, size) != size:
            raise ValueError(f"conflicting retail function size at {rva:#x}")
    owners = {}
    for row in rows(FUNCLETS):
        rva, parent = int(row["rva"], 0), int(row["parent_rva"], 0)
        if rva not in sizes:
            continue
        value = (parent, sizes[rva])
        if owners.setdefault(rva, value) != value:
            raise ValueError(f"conflicting retail funclet owner at {rva:#x}")
    return owners


def _site_context_matches(base_bytes: bytes, base_site: int,
                          target_bytes: bytes, target_site: int) -> bool:
    """Require the instruction bytes around a proposed operand to agree."""
    before = min(1, base_site, target_site)
    after = min(1, len(base_bytes) - base_site - 4,
                len(target_bytes) - target_site - 4)
    return (before >= 1 and after >= 0 and
            base_bytes[base_site - before:base_site] ==
            target_bytes[target_site - before:target_site] and
            base_bytes[base_site + 4:base_site + 4 + after] ==
            target_bytes[target_site + 4:target_site + 4 + after])


def _except_list_operand(bytes_: bytes, site: int) -> bool:
    """Recognize an x86 ``fs:[imm32]`` operand used by VC6 EH setup."""
    if site >= 2 and bytes_[site - 2:site] == b"\x64\xa1":
        return True
    if site < 3 or bytes_[site - 3:site - 1] not in (
            b"\x64\x89", b"\x64\x8b"):
        return False
    return bytes_[site - 1] & 0xC7 == 0x05


def _canonicalize_except_list_literals(
        base_payload: bytes, target_payload: bytes,
        ) -> tuple[bytes, int]:
    """Remove proved candidate-only ``__except_list`` relocations.

    VC6 represents the absolute exception-chain head as an undefined external
    named ``__except_list`` whose linker value is zero. Vostok reconstructs
    the fixed-base retail operand as the literal zero and therefore emits no
    relocation. Remove the candidate row only when the same named function,
    function-relative site, x86 ``fs:[imm32]`` context, and zero operand all
    agree, and retail has no relocation at that site. Any semantic difference
    remains visible.
    """
    base = canon.CoffObject(base_payload)
    target = canon.CoffObject(target_payload)
    base_ranges = canon._function_ranges(base)
    target_ranges = canon._function_ranges(target)

    target_functions: dict[str, list[canon.Symbol]] = {}
    for symbol in target.symbols.values():
        if (symbol.section > 0 and symbol.typ == FUNCTION_TYPE and
                symbol.storage_class == EXTERNAL_STORAGE):
            target_functions.setdefault(symbol.name, []).append(symbol)

    target_relocation_sites: set[tuple[str, int]] = set()
    for relocation in target.relocations:
        owner = canon._function_owner(
            target_ranges, relocation.section, relocation.site)
        if owner is not None:
            target_relocation_sites.add(
                (owner.name, relocation.site - owner.value))

    admitted: set[int] = set()
    for relocation in base.relocations:
        if relocation.typ != DIR32:
            continue
        symbol = base.symbols[relocation.symbol_index]
        if (symbol.name != "__except_list" or symbol.value != 0 or
                symbol.section != 0 or symbol.typ != 0 or
                symbol.storage_class != EXTERNAL_STORAGE):
            continue
        owner = canon._function_owner(
            base_ranges, relocation.section, relocation.site)
        if owner is None:
            continue
        relative_site = relocation.site - owner.value
        key = (owner.name, relative_site)
        if key in target_relocation_sites:
            continue
        counterparts = target_functions.get(owner.name, ())
        if len(counterparts) != 1:
            continue
        target_owner = counterparts[0]
        target_site = target_owner.value + relative_site
        base_section = base.sections[relocation.section - 1]
        target_section = target.sections[target_owner.section - 1]
        base_bytes = base.section_bytes(base_section)
        target_bytes = target.section_bytes(target_section)
        if (relocation.site + 4 > len(base_bytes) or
                target_site + 4 > len(target_bytes)):
            continue
        base_operand, = struct.unpack_from(
            "<I", base_bytes, relocation.site)
        target_operand, = struct.unpack_from("<I", target_bytes, target_site)
        if (base_operand != 0 or target_operand != 0 or
                not _except_list_operand(base_bytes, relocation.site) or
                not _except_list_operand(target_bytes, target_site) or
                not _site_context_matches(
                    base_bytes, relocation.site, target_bytes, target_site)):
            continue
        admitted.add(relocation.offset)

    if not admitted:
        return base_payload, 0

    data = bytearray(base_payload)
    removed_by_section: dict[int, int] = {}
    for section in base.sections:
        rows = [row for row in base.relocations
                if row.section == section.index]
        removed = [row for row in rows if row.offset in admitted]
        if not removed:
            continue
        if section.characteristics & canon.LNK_NRELOC_OVFL:
            raise RuntimeError(
                "except-list normalization does not rewrite overflow "
                "relocation tables")
        kept = b"".join(bytes(data[row.offset:row.offset + 10])
                        for row in rows if row.offset not in admitted)
        allocated_end = section.reloc_offset + len(rows) * 10
        data[section.reloc_offset:section.reloc_offset + len(kept)] = kept
        data[section.reloc_offset + len(kept):allocated_end] = bytes(
            allocated_end - section.reloc_offset - len(kept))
        struct.pack_into(
            "<H", data, section.header_offset + 32,
            section.reloc_count - len(removed))
        removed_by_section[section.index] = len(removed)

    normalized = canon.CoffObject(bytes(data))
    if (base.section_count != normalized.section_count or
            base.symbol_count != normalized.symbol_count or
            len(base.relocations) - len(admitted) !=
            len(normalized.relocations)):
        raise RuntimeError("except-list normalization changed COFF topology")
    for original, changed in zip(base.sections, normalized.sections):
        removed = removed_by_section.get(original.index, 0)
        if ((original.name, original.raw_size, original.raw_offset,
             original.reloc_offset, original.reloc_count - removed,
             original.characteristics) !=
                (changed.name, changed.raw_size, changed.raw_offset,
                 changed.reloc_offset, changed.reloc_count,
                 changed.characteristics)):
            raise RuntimeError(
                "except-list normalization changed section metadata")

    expected = Counter(
        (row.section, row.site, row.symbol_index, row.typ)
        for row in base.relocations if row.offset not in admitted)
    actual = Counter(
        (row.section, row.site, row.symbol_index, row.typ)
        for row in normalized.relocations)
    if expected != actual:
        raise RuntimeError(
            "except-list normalization changed unrelated relocations")

    before = bytearray(base_payload[:base.string_offset])
    after = bytearray(data[:normalized.string_offset])
    for section_index in removed_by_section:
        original = base.sections[section_index - 1]
        changed = normalized.sections[section_index - 1]
        before[original.header_offset + 32:original.header_offset + 34] = bytes(2)
        after[changed.header_offset + 32:changed.header_offset + 34] = bytes(2)
        table_end = original.reloc_offset + original.reloc_count * 10
        before[original.reloc_offset:table_end] = bytes(
            table_end - original.reloc_offset)
        after[changed.reloc_offset:table_end] = bytes(
            table_end - changed.reloc_offset)
    if before != after:
        raise RuntimeError(
            "except-list normalization changed unexpected COFF bytes")
    for index, original in base.symbols.items():
        if original != normalized.symbols[index]:
            raise RuntimeError("except-list normalization changed a symbol")
    return bytes(data), len(admitted)


def _canonicalize_equivalent_relocations(
        base_payload: bytes, target_payload: bytes,
        symbol_rvas: dict[str, tuple[int, str]], *, image_base: int,
        ) -> tuple[bytes, int, int]:
    """Normalize two stripped-image relocation representations.

    First, vostok can classify an honest 32-bit literal as DIR32 when its
    numeric value happens to be a retail VA.  When the paired candidate has
    no relocation and its operand is exactly the target symbol+addend resolved
    VA, the false target relocation is removed and the resolved literal is
    written back.

    Second, vostok can name an interior data field while VC6 names the owning
    aggregate plus an addend.  A reviewed retail name at the candidate
    aggregate's base, or otherwise one unambiguous same-addend paired
    relocation, anchors the candidate external symbol to a generated retail
    data RVA.  Other relocations using that candidate symbol are rewritten on
    the target side only when both forms resolve to the identical RVA.

    Both transforms are paired, function-relative, and context-checked.
    Symbols and section/file layout stay fixed; only proved false-literal rows
    reduce a section's relocation count. Different resolved addresses stay
    visible.
    """
    base = canon.CoffObject(base_payload)
    target = canon.CoffObject(target_payload)
    base_ranges = canon._function_ranges(base)
    target_ranges = canon._function_ranges(target)

    base_functions: dict[str, list[canon.Symbol]] = {}
    target_functions: dict[str, list[canon.Symbol]] = {}
    for coff, table in ((base, base_functions), (target, target_functions)):
        for symbol in coff.symbols.values():
            if (symbol.section > 0 and symbol.typ == FUNCTION_TYPE and
                    symbol.storage_class == EXTERNAL_STORAGE):
                table.setdefault(symbol.name, []).append(symbol)

    base_pairs: dict[tuple[str, int], canon.Relocation] = {}
    target_pairs: dict[tuple[str, int], canon.Relocation] = {}
    for coff, ranges, pairs in ((base, base_ranges, base_pairs),
                                (target, target_ranges, target_pairs)):
        for relocation in coff.relocations:
            if relocation.typ != DIR32:
                continue
            owner = canon._function_owner(
                ranges, relocation.section, relocation.site)
            if owner is None:
                continue
            pairs[(owner.name, relocation.site - owner.value)] = relocation

    base_relocation_sites: set[tuple[str, int]] = set()
    for relocation in base.relocations:
        owner = canon._function_owner(
            base_ranges, relocation.section, relocation.site)
        if owner is not None:
            base_relocation_sites.add(
                (owner.name, relocation.site - owner.value))

    # Infer a candidate external's retail base only from an equal-addend
    # paired data relocation. Conflicting observations make that symbol
    # ineligible rather than choosing one.
    observations: dict[int, set[tuple[int, int]]] = {}
    for key, base_relocation in base_pairs.items():
        target_relocation = target_pairs.get(key)
        if target_relocation is None:
            continue
        target_symbol = target.symbols[target_relocation.symbol_index]
        authority = symbol_rvas.get(target_symbol.name)
        if authority is None or authority[1] != "data":
            continue
        base_section = base.sections[base_relocation.section - 1]
        target_section = target.sections[target_relocation.section - 1]
        base_addend, = struct.unpack_from(
            "<I", base.section_bytes(base_section), base_relocation.site)
        target_addend, = struct.unpack_from(
            "<I", target.section_bytes(target_section), target_relocation.site)
        if base_addend != target_addend:
            continue
        observations.setdefault(base_relocation.symbol_index, set()).add(
            (authority[0], target_relocation.symbol_index))
    inferred_anchors = {}
    for symbol, rows in observations.items():
        # Delinked COFF can repeat the same undefined data symbol. Its
        # symbol-table indices do not establish distinct retail owners.
        identities = {(rva, target.symbols[index].name)
                      for rva, index in rows}
        if len(identities) == 1:
            inferred_anchors[symbol] = min(rows, key=lambda row: row[1])

    # A reviewed reloc-alias owner is stronger than the equal-addend
    # heuristic and remains usable when one stripped object contains several
    # synthesized zero-addend field names for the same candidate aggregate.
    # The target must still contain one unique data symbol at that exact owner
    # RVA; otherwise there is no symbol index to rewrite to and the mismatch
    # stays visible.
    known_data_rvas = {
        rva for rva, kind in symbol_rvas.values() if kind == "data"
    }
    target_data_symbols: dict[int, dict[str, set[int]]] = {}
    for symbol in target.symbols.values():
        authority = symbol_rvas.get(symbol.name)
        if authority is None:
            placeholder = re.fullmatch(
                r"(?:data|bss)_([0-9a-fA-F]+)", symbol.name)
            if placeholder is not None:
                placeholder_rva = int(placeholder.group(1), 16)
                if placeholder_rva in known_data_rvas:
                    authority = placeholder_rva, "data"
        if authority is not None and authority[1] == "data":
            target_data_symbols.setdefault(authority[0], {}).setdefault(
                symbol.name, set()).add(symbol.index)
    reviewed_anchors: dict[int, tuple[int, int]] = {}
    for symbol in base.symbols.values():
        authority = symbol_rvas.get(symbol.name)
        if authority is None or authority[1] != "data":
            continue
        targets = target_data_symbols.get(authority[0], {})
        if len(targets) == 1:
            indices = next(iter(targets.values()))
            reviewed_anchors[symbol.index] = (
                authority[0], min(indices))

    anchors = dict(inferred_anchors)
    for symbol, reviewed in reviewed_anchors.items():
        inferred = inferred_anchors.get(symbol)
        if inferred is not None and inferred[0] != reviewed[0]:
            anchors.pop(symbol, None)
            continue
        anchors[symbol] = reviewed

    data = bytearray(target_payload)
    admitted: dict[int, tuple[int, int, int, str]] = {}
    literal_count = aggregate_count = 0

    # Aggregate+addend versus synthesized field-symbol+zero.
    for key, base_relocation in base_pairs.items():
        target_relocation = target_pairs.get(key)
        anchor = anchors.get(base_relocation.symbol_index)
        if target_relocation is None or anchor is None:
            continue
        target_symbol = target.symbols[target_relocation.symbol_index]
        authority = symbol_rvas.get(target_symbol.name)
        if authority is None:
            # Vostok names stripped interior references by their resolved
            # retail address.  The owner base is the reviewed fact; an
            # interior placeholder need not (and normally does not) have a
            # second hand-admitted DATA row.  It is safe to use the parsed
            # address here because the equality below still requires the
            # candidate owner+addend and target placeholder+addend to resolve
            # to the identical RVA.  A wrong synthetic address therefore
            # remains visible.
            placeholder = re.fullmatch(
                r"(?:data|bss)_([0-9a-fA-F]+)", target_symbol.name)
            if placeholder is not None:
                authority = int(placeholder.group(1), 16), "data"
        if authority is None or authority[1] != "data":
            continue
        base_owner = base_functions.get(key[0], ())
        target_owner = target_functions.get(key[0], ())
        if len(base_owner) != 1 or len(target_owner) != 1:
            continue
        base_section = base.sections[base_relocation.section - 1]
        target_section = target.sections[target_relocation.section - 1]
        base_bytes = base.section_bytes(base_section)
        target_bytes = target.section_bytes(target_section)
        base_addend, = struct.unpack_from("<I", base_bytes,
                                          base_relocation.site)
        target_addend, = struct.unpack_from("<I", target_bytes,
                                            target_relocation.site)
        # This pass is specifically for aggregate+offset versus a synthesized
        # field symbol. Equal addends are already the same source form and do
        # not establish that a different retail name should be hidden.
        if base_addend == target_addend:
            continue
        anchor_rva, anchor_symbol = anchor
        if ((anchor_rva + base_addend) & 0xFFFFFFFF) != \
                ((authority[0] + target_addend) & 0xFFFFFFFF):
            continue
        if not _site_context_matches(base_bytes, base_relocation.site,
                                     target_bytes, target_relocation.site):
            continue
        if (target_addend == base_addend and
                target_relocation.symbol_index == anchor_symbol):
            continue
        operand = target_section.raw_offset + target_relocation.site
        struct.pack_into("<I", data, operand, base_addend)
        struct.pack_into("<I", data, target_relocation.offset + 4,
                         anchor_symbol)
        admitted[target_relocation.offset] = (
            anchor_symbol, DIR32, base_addend, "aggregate")
        aggregate_count += 1

    # Honest literal versus a false stripped-image DIR32 classification.
    for key, target_relocation in target_pairs.items():
        if key in base_relocation_sites:
            continue
        target_symbol = target.symbols[target_relocation.symbol_index]
        authority = symbol_rvas.get(target_symbol.name)
        if authority is None:
            continue
        base_owner = base_functions.get(key[0], ())
        target_owner = target_functions.get(key[0], ())
        if len(base_owner) != 1 or len(target_owner) != 1:
            continue
        base_function = base_owner[0]
        target_function = target_owner[0]
        base_site = base_function.value + key[1]
        target_site = target_function.value + key[1]
        base_section = base.sections[base_function.section - 1]
        target_section = target.sections[target_function.section - 1]
        base_bytes = base.section_bytes(base_section)
        target_bytes = target.section_bytes(target_section)
        if base_site + 4 > len(base_bytes) or target_site + 4 > len(target_bytes):
            continue
        target_addend, = struct.unpack_from("<I", target_bytes, target_site)
        resolved = (image_base + authority[0] + target_addend) & 0xFFFFFFFF
        base_operand, = struct.unpack_from("<I", base_bytes, base_site)
        if base_operand != resolved or not _site_context_matches(
                base_bytes, base_site, target_bytes, target_site):
            continue
        operand = target_section.raw_offset + target_site
        struct.pack_into("<I", data, operand, resolved)
        admitted[target_relocation.offset] = (
            target_relocation.symbol_index, DIR32, resolved, "literal")
        literal_count += 1

    # COFF has no objdiff-supported "ignored" x86 relocation type. Remove
    # admitted false-literal rows by compacting each relocation table inside
    # its existing allocation. This deliberately leaves every following file
    # offset fixed; the unused tail is zero fill.
    literal_offsets = {
        offset for offset, (_symbol, _typ, _addend, kind) in admitted.items()
        if kind == "literal"
    }
    removed_by_section: dict[int, int] = {}
    for section in target.sections:
        rows = [row for row in target.relocations
                if row.section == section.index]
        removed = [row for row in rows if row.offset in literal_offsets]
        if not removed:
            continue
        if section.characteristics & canon.LNK_NRELOC_OVFL:
            raise RuntimeError(
                "equivalent-relocation normalization does not rewrite "
                "overflow relocation tables")
        kept = b"".join(bytes(data[row.offset:row.offset + 10])
                        for row in rows if row.offset not in literal_offsets)
        allocated_end = section.reloc_offset + len(rows) * 10
        data[section.reloc_offset:section.reloc_offset + len(kept)] = kept
        data[section.reloc_offset + len(kept):allocated_end] = bytes(
            allocated_end - section.reloc_offset - len(kept))
        new_count = section.reloc_count - len(removed)
        struct.pack_into("<H", data, section.header_offset + 32, new_count)
        removed_by_section[section.index] = len(removed)

    normalized = canon.CoffObject(bytes(data))
    if (target.section_count != normalized.section_count or
            target.symbol_count != normalized.symbol_count or
            len(target.relocations) - literal_count !=
            len(normalized.relocations)):
        raise RuntimeError("equivalent-relocation normalization changed COFF topology")

    for original, changed in zip(target.sections, normalized.sections):
        removed = removed_by_section.get(original.index, 0)
        if ((original.name, original.raw_size, original.raw_offset,
             original.reloc_offset, original.reloc_count - removed,
             original.characteristics) !=
                (changed.name, changed.raw_size, changed.raw_offset,
                 changed.reloc_offset, changed.reloc_count,
                 changed.characteristics)):
            raise RuntimeError(
                "equivalent-relocation normalization changed section metadata")

    expected_relocations = Counter()
    for original in target.relocations:
        rewrite = admitted.get(original.offset)
        if rewrite is not None and rewrite[3] == "literal":
            continue
        symbol_index = rewrite[0] if rewrite is not None else original.symbol_index
        typ = rewrite[1] if rewrite is not None else original.typ
        expected_relocations[(original.section, original.site,
                              symbol_index, typ)] += 1
    actual_relocations = Counter(
        (row.section, row.site, row.symbol_index, row.typ)
        for row in normalized.relocations)
    if expected_relocations != actual_relocations:
        raise RuntimeError(
            "equivalent-relocation normalization changed unrelated relocations")

    normalized_by_site: dict[tuple[int, int], list[canon.Relocation]] = {}
    for row in normalized.relocations:
        normalized_by_site.setdefault((row.section, row.site), []).append(row)
    before = bytearray(target_payload[:target.string_offset])
    after = bytearray(data[:normalized.string_offset])
    for section_index in removed_by_section:
        original = target.sections[section_index - 1]
        changed = normalized.sections[section_index - 1]
        before[original.header_offset + 32:original.header_offset + 34] = bytes(2)
        after[changed.header_offset + 32:changed.header_offset + 34] = bytes(2)
        table_end = original.reloc_offset + original.reloc_count * 10
        before[original.reloc_offset:table_end] = bytes(
            table_end - original.reloc_offset)
        after[changed.reloc_offset:table_end] = bytes(
            table_end - changed.reloc_offset)
    for offset, (symbol_index, typ, addend, kind) in admitted.items():
        original = next(row for row in target.relocations if row.offset == offset)
        section = target.sections[original.section - 1]
        operand = section.raw_offset + original.site
        before[operand:operand + 4] = bytes(4)
        after[operand:operand + 4] = bytes(4)
        if original.section not in removed_by_section:
            before[offset:offset + 10] = bytes(10)
            after[offset:offset + 10] = bytes(10)
        changed_rows = normalized_by_site.get(
            (original.section, original.site), ())
        if kind == "literal":
            if changed_rows:
                raise RuntimeError(
                    "false-literal relocation was not removed")
            changed_addend, = struct.unpack_from(
                "<I", normalized.section_bytes(
                    normalized.sections[original.section - 1]), original.site)
        else:
            matching = [row for row in changed_rows
                        if row.symbol_index == symbol_index and row.typ == typ]
            if len(matching) != 1:
                raise RuntimeError(
                    "equivalent aggregate relocation was not rewritten")
            changed_addend, = struct.unpack_from(
                "<I", normalized.section_bytes(
                    normalized.sections[original.section - 1]), original.site)
        if changed_addend != addend:
            raise RuntimeError("equivalent relocation postcondition failed")
    if before != after:
        raise RuntimeError(
            "equivalent-relocation normalization changed unexpected COFF bytes")
    for index, original in target.symbols.items():
        if original != normalized.symbols[index]:
            raise RuntimeError(
                "equivalent-relocation normalization changed a symbol")
    return bytes(data), literal_count, aggregate_count


@dataclass(frozen=True)
class FunctionBody:
    """One external function's bytes and function-relative relocation sites."""

    payload: bytes
    sites: tuple[int, ...]


_ICF_PADDING = frozenset(b"\xcc\x90")


def _function_bodies(coff: canon.CoffObject) -> dict[str, FunctionBody]:
    """External function bodies of one object; ambiguous repeats dropped."""
    by_section: dict[int, list[canon.Relocation]] = {}
    for relocation in coff.relocations:
        by_section.setdefault(relocation.section, []).append(relocation)
    bodies: dict[str, FunctionBody | None] = {}
    for section_index, ranges in canon._function_ranges(coff).items():
        section = coff.sections[section_index - 1]
        data = coff.section_bytes(section)
        relocations = by_section.get(section_index, ())
        for start, end, symbol in ranges:
            if symbol.storage_class != EXTERNAL_STORAGE:
                continue
            body = FunctionBody(data[start:end], tuple(sorted(
                r.site - start for r in relocations if start <= r.site < end)))
            if symbol.name in bodies and bodies[symbol.name] != body:
                bodies[symbol.name] = None
            else:
                bodies[symbol.name] = body
    return {name: body for name, body in bodies.items() if body is not None}


def _icf_identical(left: FunctionBody, right: FunctionBody) -> bool:
    """Bytes equal outside relocation fields, the same relocation sites, and
    at most linker fill (int3/nop) beyond the shorter body."""
    if left.sites != right.sites:
        return False
    short, long_ = sorted((left.payload, right.payload), key=len)
    if not short or any(byte not in _ICF_PADDING for byte in long_[len(short):]):
        return False
    if left.sites and left.sites[-1] + 4 > len(short):
        return False
    masked = [bytearray(short), bytearray(long_[:len(short)])]
    for site in left.sites:
        for payload in masked:
            payload[site:site + 4] = b"\0\0\0\0"
    return masked[0] == masked[1]


def _content_key(*parts: bytes | str | Path) -> str:
    """Digest of literal parts and of file contents (for Path parts)."""
    digest = hashlib.sha256()
    for part in parts:
        if isinstance(part, Path):
            part = hashlib.sha256(part.read_bytes()).digest()
        digest.update(part.encode() if isinstance(part, str) else part)
        digest.update(b"\0")
    return digest.hexdigest()


def _cache_dir() -> Path:
    """Content-keyed derived indexes beside the comparison tree (build/gen/cache);
    disposable, never a freshness authority."""
    return OBJDIFF.parent / "gen/cache"


def _load_cache(name: str) -> dict:
    try:
        with open(_cache_dir() / name, "rb") as stream:
            payload = pickle.load(stream)
        return payload if isinstance(payload, dict) else {}
    except (OSError, EOFError, pickle.UnpicklingError, AttributeError, ImportError, ValueError):
        return {}


def _store_cache(name: str, payload: dict) -> None:
    """Atomically replace one cache file; a failed write only costs speed."""
    try:
        cache = _cache_dir()
        cache.mkdir(parents=True, exist_ok=True)
        temporary = cache / f".{name}.{os.getpid()}.tmp"
        with open(temporary, "wb") as stream:
            pickle.dump(payload, stream, protocol=pickle.HIGHEST_PROTOCOL)
        os.replace(temporary, cache / name)
    except OSError:
        pass


def _icf_code_key() -> str:
    # Bodies depend only on the object bytes and these parsers.
    return _content_key(Path(__file__), Path(canon.__file__))


_ICF_INDEX: tuple[dict[str, FunctionBody], dict[str, FunctionBody]] | None = None


def _icf_index() -> tuple[dict[str, FunctionBody], dict[str, FunctionBody]]:
    """(candidate bodies, retail bodies) across every normalized object.

    A body defined differently by two objects is ambiguous and omitted.
    Each object's bodies are cached by its content hash, so an unchanged
    object is never reparsed; the merge itself always reads every object.
    """
    global _ICF_INDEX
    if _ICF_INDEX is None:
        code = _icf_code_key()
        cached = _load_cache("icf-bodies.pickle")
        previous = cached.get("objects", {}) if cached.get("code") == code else {}
        current: dict[str, tuple[str, dict | None]] = {}
        sides = []
        for side, pattern in (("base", "*.obj"), ("target", "*.c.obj")):
            merged: dict[str, FunctionBody | None] = {}
            root = OBJDIFF / "normalized" / side
            for obj in sorted(root.rglob(pattern)) if root.is_dir() else ():
                data = obj.read_bytes()
                digest = hashlib.sha256(data).hexdigest()
                key = f"{side}/{obj.relative_to(root).as_posix()}"
                hit = previous.get(key)
                if hit is not None and hit[0] == digest:
                    rows = hit[1]
                else:
                    try:
                        rows = {name: (body.payload, body.sites) for name, body in
                                _function_bodies(canon.CoffObject(data)).items()}
                    except ValueError:
                        rows = None
                current[key] = (digest, rows)
                if rows is None:
                    continue
                bodies = {name: FunctionBody(*row) for name, row in rows.items()}
                for name, body in bodies.items():
                    if name in merged and merged[name] != body:
                        merged[name] = None
                    else:
                        merged[name] = body
            sides.append({n: b for n, b in merged.items() if b is not None})
        if current != previous:
            _store_cache("icf-bodies.pickle", {"code": code, "objects": current})
        _ICF_INDEX = (sides[0], sides[1])
    return _ICF_INDEX


_RETAIL_TWINS = None


def _retail_twins():
    """reloc_pairing's RetailTwins over the census: retail function RVA ->
    other census functions with the same bytes and call targets.

    Retail keeps some byte-identical bodies unfolded (the vector::insert
    copies, _Ufill<int> and _Ufill<widget*>, the `ret` bodies). A call that
    reaches one of them does not prove which instantiation the source named;
    the one rule is reloc_pairing's (`prove_folds`)."""
    global _RETAIL_TWINS
    if _RETAIL_TWINS is None:
        from homm3.core import inputs
        from homm3.delink import reloc_pairing
        from homm3.delink.image import retail
        with FUNCTIONS.open(newline="") as stream:
            rows = csv.DictReader((line for line in stream if not line.startswith("#")),
                                  delimiter="\t")
            sizes = {int(row["rva"], 0): int(row["size"], 0) for row in rows}
        # The index disassembles every retail function (seconds); it depends
        # only on the pinned image, the census and the key implementation.
        key = _content_key(inputs.RETAIL.sha256, FUNCTIONS, Path(reloc_pairing.__file__))
        cached = _load_cache("retail-twins.pickle")
        index = cached.get("index") if cached.get("key") == key else None
        _RETAIL_TWINS = reloc_pairing.RetailTwins(retail(), sizes, index)
        if index is None:
            _store_cache("retail-twins.pickle",
                         {"key": key, "index": _RETAIL_TWINS.index()})
    return _RETAIL_TWINS


def _canonicalize_icf_aliases(
        base_payload: bytes, target_payload: bytes,
        symbol_rvas: dict[str, tuple[int, str]],
        index: tuple[dict[str, FunctionBody], dict[str, FunctionBody]],
        twins_of=lambda rva: (),
        folded_at=lambda name, rva: False,
        ) -> tuple[bytes, bytes, int]:
    """Name a retail /OPT:ICF body by the candidate's folded twin.

    Retail labels one address for byte-identical functions that /OPT:ICF
    folded, so a call to the unlabelled twin cl emitted (a class's inline
    destructor, an empty virtual, vector<T*>::size for another T) pairs
    with the surviving twin's label. At a paired call/data site the two
    references are made to agree only when the retail name is a labelled
    function, the two bodies are ICF-identical, and the candidate name has
    no retail address of its own, and retail keeps no identical copy of the
    surviving body elsewhere (`twins_of`) unless reloc_pairing's twin rule
    admitted the fold of that candidate twin at that address (`folded_at`:
    each identical retail twin is independently another function). Each
    candidate twin paired with a label is judged alone, so a differing
    overload at one site leaves the verified twins' sites canonical. When
    the label's only twin verifies, its undefined target reference is
    renamed to the twin; otherwise the verified twins' candidate
    relocations point at the surviving label, appended to the candidate
    object as an undefined external when absent. Every other difference
    stays visible. Returns (base, target, rewritten references).
    """
    base = canon.CoffObject(base_payload)
    target = canon.CoffObject(target_payload)
    base_ranges = canon._function_ranges(base)
    target_ranges = canon._function_ranges(target)

    def paired(coff, ranges):
        rows = {}
        for relocation in coff.relocations:
            if relocation.typ not in (REL32, DIR32):
                continue
            owner = canon._function_owner(ranges, relocation.section, relocation.site)
            if owner is not None:
                rows[(owner.name, relocation.site - owner.value)] = relocation
        return rows

    base_rows = paired(base, base_ranges)
    target_rows = paired(target, target_ranges)
    target_names = {symbol.name for symbol in target.symbols.values()}
    base_by_name: dict[str, int] = {}
    for symbol in sorted(base.symbols.values(), key=lambda row: row.index):
        if symbol.storage_class == EXTERNAL_STORAGE:
            base_by_name.setdefault(symbol.name, symbol.index)
    candidates, retail = index
    sites: dict[tuple[str, str], list[canon.Relocation]] = {}
    aliases: dict[str, set[str]] = {}
    for key, target_relocation in target_rows.items():
        base_relocation = base_rows.get(key)
        if base_relocation is None or base_relocation.typ != target_relocation.typ:
            continue
        twin = base.symbols[base_relocation.symbol_index].name
        label = target.symbols[target_relocation.symbol_index].name
        if twin == label:
            continue
        aliases.setdefault(label, set()).add(twin)
        sites.setdefault((label, twin), []).append(base_relocation)
    renames: dict[int, str] = {}
    retargets: list[tuple[canon.Relocation, int]] = []
    pending: list[tuple[str, set[str]]] = []
    for label, twins in aliases.items():
        authority = symbol_rvas.get(label)
        surviving = retail.get(label)
        if authority is None or authority[1] != "func" or surviving is None:
            continue
        retail_copies = bool(twins_of(authority[0]))
        verified = set()
        for twin in twins:
            if retail_copies and not folded_at(twin, authority[0]):
                # Retail keeps an identical copy elsewhere: unless the pairing
                # rule identified this twin as another function, it could
                # equally be folded onto either, so its sites stay visible.
                continue
            candidate = candidates.get(twin)
            if candidate is None or twin in symbol_rvas:
                # Retail keeps some byte-identical bodies unfolded (_Ufill<int>
                # and _Ufill<widget*>, several vector::insert copies). A twin
                # with its own retail address is a different instantiation
                # than the one this site reached, so the fold is not proven.
                continue
            if not _icf_identical(candidate, surviving):
                continue
            verified.add(twin)
        if not verified:
            continue
        undefined = [symbol.index for symbol in target.symbols.values()
                     if symbol.name == label and symbol.section == 0]
        # Renaming the target symbol renames every site; when the candidate
        # also references the surviving name itself, retarget the twin's
        # sites instead.
        if (verified == twins and len(twins) == 1 and undefined
                and next(iter(twins)) not in target_names
                and label not in base_by_name):
            renames.update((symbol, next(iter(twins))) for symbol in undefined)
        else:
            # Each verified twin's sites name the surviving label; a twin
            # that differs from it (another overload paired at a shifted
            # site) keeps its own sites visible.
            pending.append((label, verified))
    if pending:
        missing = [label for label, _twins in pending if label not in base_by_name]
        if missing:
            base_payload, appended = _append_undefined_symbols(base_payload, missing)
            base_by_name.update(appended)
        for label, verified in pending:
            retargets.extend((relocation, base_by_name[label])
                             for twin in sorted(verified)
                             for relocation in sites[(label, twin)])
    if retargets:
        data = bytearray(base_payload)
        for relocation, symbol in retargets:
            struct.pack_into("<I", data, relocation.offset + 4, symbol)
        base_payload = bytes(data)
    if renames:
        target_payload = canon._rewrite_names(target, renames)
    return base_payload, target_payload, len(renames) + len(retargets)


def _append_undefined_symbols(payload: bytes, names: list[str]) -> tuple[bytes, dict[str, int]]:
    """Append undefined external symbols; existing indices and offsets stay."""
    coff = canon.CoffObject(payload)
    strings = bytearray(payload[coff.string_offset:])
    records = bytearray()
    indices = {}
    index = coff.symbol_count
    for name in names:
        raw = name.encode("latin-1")
        if len(raw) <= 8:
            field = raw.ljust(8, b"\0")
        else:
            field = struct.pack("<II", 0, len(strings))
            strings.extend(raw + b"\0")
        records.extend(field + struct.pack("<IhHBB", 0, 0, 0, EXTERNAL_STORAGE, 0))
        indices[name] = index
        index += 1
    struct.pack_into("<I", strings, 0, len(strings))
    data = bytearray(payload[:coff.string_offset]) + records + strings
    struct.pack_into("<I", data, 12, index)
    return bytes(data), indices


def _canonicalize_equivalent_data_addresses(
        base_payload: bytes, target_payload: bytes,
        symbol_rvas: dict[str, tuple[int, str]]) -> tuple[bytes, int]:
    """Compare two data references that resolve to one retail address.

    VC6 spells an element address as its array symbol plus an addend, which
    may lie before the array (`g_statNames[i - 23]`); the stripped image
    names the same address after whichever labelled object contains it. When
    a paired DIR32 site's candidate symbol has a labelled retail address and
    both symbol+addend forms resolve to the same retail RVA, the target
    relocation takes the candidate's symbol and addend (appending that
    undefined external to the target object when it has none). Different
    resolved addresses stay visible.
    """
    base = canon.CoffObject(base_payload)
    target = canon.CoffObject(target_payload)
    base_ranges = canon._function_ranges(base)
    target_ranges = canon._function_ranges(target)

    def paired(coff, ranges):
        rows = {}
        for relocation in coff.relocations:
            if relocation.typ != DIR32:
                continue
            owner = canon._function_owner(ranges, relocation.section, relocation.site)
            if owner is not None:
                rows[(owner.name, relocation.site - owner.value)] = relocation
        return rows

    def authority(name):
        found = symbol_rvas.get(name)
        if found is None:
            placeholder = re.fullmatch(r"(?:data|bss|const)_([0-9a-fA-F]+)", name)
            if placeholder is not None:
                found = int(placeholder.group(1), 16), "data"
        return found if found is not None and found[1] == "data" else None

    base_rows = paired(base, base_ranges)
    target_rows = paired(target, target_ranges)
    rewrites = []
    for key, target_relocation in target_rows.items():
        base_relocation = base_rows.get(key)
        if base_relocation is None:
            continue
        base_symbol = base.symbols[base_relocation.symbol_index]
        target_symbol = target.symbols[target_relocation.symbol_index]
        base_rva, target_rva = authority(base_symbol.name), authority(target_symbol.name)
        if base_rva is None or target_rva is None:
            continue
        base_bytes = base.section_bytes(base.sections[base_relocation.section - 1])
        target_section = target.sections[target_relocation.section - 1]
        target_bytes = target.section_bytes(target_section)
        base_addend, = struct.unpack_from("<i", base_bytes, base_relocation.site)
        target_addend, = struct.unpack_from("<i", target_bytes, target_relocation.site)
        if base_symbol.name == target_symbol.name and base_addend == target_addend:
            continue
        if base_rva[0] + base_addend != target_rva[0] + target_addend:
            continue
        if not _site_context_matches(base_bytes, base_relocation.site,
                                     target_bytes, target_relocation.site):
            continue
        rewrites.append((target_relocation, target_section, base_symbol.name, base_addend))
    if not rewrites:
        return target_payload, 0
    existing = {}
    for symbol in sorted(target.symbols.values(), key=lambda row: row.index):
        existing.setdefault(symbol.name, symbol.index)
    missing = sorted({name for _r, _s, name, _a in rewrites if name not in existing})
    payload, appended = _append_undefined_symbols(target_payload, missing)
    existing.update(appended)
    data = bytearray(payload)
    for relocation, section, name, addend in rewrites:
        struct.pack_into("<i", data, section.raw_offset + relocation.site, addend)
        struct.pack_into("<I", data, relocation.offset + 4, existing[name])
    result = bytes(data)
    normalized = canon.CoffObject(result)
    if len(normalized.relocations) != len(target.relocations):
        raise RuntimeError("equivalent data-address normalization changed relocation count")
    return result, len(rewrites)


def _move_symbols_to_zero_sections(payload: bytes,
                                   moves: list[tuple[canon.Symbol, str, int]]) -> bytes:
    """Give each symbol its own initialized section of zero bytes.

    New section headers follow the existing ones (every stored file offset
    shifts by the inserted headers); their raw data goes directly before the
    symbol table. Existing section numbers, symbol indices and relocations
    are unchanged; only the moved symbols' section and value fields change.
    """
    coff = canon.CoffObject(payload)
    optional_size = struct.unpack_from("<H", payload, 16)[0]
    insert_at = 20 + optional_size + coff.section_count * 40
    shift = 40 * len(moves)
    data = bytearray(payload[:insert_at] + bytes(shift) + payload[insert_at:])
    for section in coff.sections:
        for field_offset, value in ((20, section.raw_offset), (24, section.reloc_offset)):
            if value:
                struct.pack_into("<I", data, section.header_offset + field_offset,
                                 value + shift)
        lineno = struct.unpack_from("<I", payload, section.header_offset + 28)[0]
        if lineno:
            struct.pack_into("<I", data, section.header_offset + 28, lineno + shift)
    symbol_offset = coff.symbol_offset + shift
    raw = bytearray()
    for position, (symbol, name, size) in enumerate(moves):
        number = coff.section_count + 1 + position
        header = struct.pack("<8sIIIIIIHHI", name.encode("latin-1").ljust(8, b"\0"),
                             0, 0, size, symbol_offset + len(raw), 0, 0, 0, 0,
                             INITIALIZED_DATA | 0xC0000000 | 0x00300000)
        data[insert_at + 40 * position:insert_at + 40 * (position + 1)] = header
        raw.extend(bytes(size))
        struct.pack_into("<I", data, symbol.offset + shift + 8, 0)
        struct.pack_into("<h", data, symbol.offset + shift + 12, number)
    data[symbol_offset:symbol_offset] = raw
    struct.pack_into("<H", data, 2, coff.section_count + len(moves))
    struct.pack_into("<I", data, 8, symbol_offset + len(raw))
    result = bytes(data)
    canon.CoffObject(result)
    return result


def _canonicalize_zero_literal_sections(
        base_payload: bytes, target_payload: bytes) -> tuple[bytes, bytes, int]:
    """Compare a zero-filled string literal in its retail section.

    cl emits an all-zero literal such as `""` (`??_C@_00A@?$AA@`) as
    uninitialized data (its own .bss COMDAT, or a COMMON that normalization
    materializes into .bss), while retail pools the same bytes in initialized
    data. The comparison matches relocation targets by section name, and a
    .bss-named section cannot carry bytes. Each such candidate literal moves
    to its own initialized section of the same zero bytes, named after the
    section in which the target defines it. No byte value or relocation
    moves. Returns (base, target, literals moved).
    """
    base = canon.CoffObject(base_payload)
    target = canon.CoffObject(target_payload)
    target_home = {}
    for symbol in target.symbols.values():
        if symbol.section > 0 and symbol.name.startswith("??_C@"):
            target_home[symbol.name] = target.sections[symbol.section - 1]
    starts: dict[int, list[int]] = {}
    for symbol in base.symbols.values():
        if symbol.section > 0:
            starts.setdefault(symbol.section, []).append(symbol.value)
    moves = []
    for symbol in sorted(base.symbols.values(), key=lambda row: row.index):
        if symbol.section <= 0 or not symbol.name.startswith("??_C@"):
            continue
        section = base.sections[symbol.section - 1]
        home = target_home.get(symbol.name)
        if (section.name != ".bss" or section.raw_offset
                or not section.characteristics & UNINITIALIZED_DATA
                or home is None or home.name == section.name
                or len(home.name) > 8):
            continue
        following = [value for value in starts[symbol.section] if value > symbol.value]
        end = min(following) if following else section.raw_size
        size = end - symbol.value
        if home.raw_size:
            size = min(size, home.raw_size)
        if size <= 0:
            continue
        moves.append((symbol, home.name, size))
    if not moves:
        return base_payload, target_payload, 0
    return _move_symbols_to_zero_sections(base_payload, moves), target_payload, len(moves)


def _associative_parents(coff: canon.CoffObject) -> dict[int, int]:
    """Read IMAGE_COMDAT_SELECT_ASSOCIATIVE parents from section aux rows."""
    result = {}
    for symbol in coff.symbols.values():
        if (symbol.section <= 0 or symbol.storage_class != STATIC_STORAGE or
                not symbol.aux_count):
            continue
        section = coff.sections[symbol.section - 1]
        if symbol.name != section.name:
            continue
        aux = symbol.offset + canon.SYMBOL_SIZE
        parent, = struct.unpack_from("<H", coff.data, aux + 12)
        selection = coff.data[aux + 14]
        if selection != ASSOCIATIVE_COMDAT:
            continue
        if not 1 <= parent <= coff.section_count or parent == symbol.section:
            raise ValueError(
                f"invalid associative COMDAT parent {parent} for section "
                f"{symbol.section}")
        previous = result.setdefault(symbol.section, parent)
        if previous != parent:
            raise ValueError(
                f"conflicting associative COMDAT parents for section "
                f"{symbol.section}")
    return result


#: The VC6 frame-handler registration prologues, up to the `push offset
#: handler` operand: `push ebp; mov ebp, esp; push -1; push offset`, and the
#: same with `mov eax, fs:[0]` scheduled before `push -1` (its `__except_list`
#: displacement is the absolute 0).
EH_PROLOGUES = (b"\x55\x8b\xec\x6a\xff\x68",
                b"\x55\x8b\xec\x64\xa1\x00\x00\x00\x00\x6a\xff\x68",
                # /O2 without a frame pointer (the campaign editor)
                b"\x6a\xff\x68",
                b"\x64\xa1\x00\x00\x00\x00\x6a\xff\x68")


#: The `/O1` form (the map editor): `mov eax, offset handler; call
#: __EH_prolog`, whose helper builds the same registration.
EH_PROLOG_CALL = (b"\xb8", b"\xe8")


def _eh_prologue_site(code: bytes, start: int) -> int | None:
    """Offset of the handler operand after a recognized EH prologue."""
    for prologue in EH_PROLOGUES:
        if code[start:start + len(prologue)] == prologue:
            return len(prologue)
    mov, call = EH_PROLOG_CALL
    if code[start:start + 1] == mov and code[start + 5:start + 6] == call:
        return len(mov)
    return None


def _eh_handler_candidates(coff: canon.CoffObject) -> tuple[EhHandlerOwnerRewrite, ...]:
    """Find canonical VC6 EH prologues whose operand names the handler thunk.

    VC6 puts cleanup funclets and the ten-byte CxxFrameHandler thunk in an
    associative ``.text$x`` COMDAT. The compiler object relocates the second
    prologue push directly to that final thunk. Vostok instead expresses the
    same byte as ``last cleanup funclet + cleanup size``. This recognizer is
    intentionally structural: parent association, exact EH prologue, final
    local label, exact handler-thunk shape, and both thunk relocations must all
    agree before a candidate is returned.
    """
    parents = _associative_parents(coff)
    ranges = canon._function_ranges(coff)
    relocations_by_section: dict[int, list[canon.Relocation]] = {}
    for relocation in coff.relocations:
        relocations_by_section.setdefault(relocation.section, []).append(relocation)
    local_labels: dict[int, list[canon.Symbol]] = {}
    for symbol in coff.symbols.values():
        if (symbol.section > 0 and symbol.typ == 0 and
                symbol.storage_class == LABEL_STORAGE):
            local_labels.setdefault(symbol.section, []).append(symbol)

    candidates = []
    for relocation in coff.relocations:
        if relocation.typ != DIR32:
            continue
        owner = canon._function_owner(ranges, relocation.section, relocation.site)
        if owner is None:
            continue
        parent = coff.sections[relocation.section - 1]
        parent_bytes = coff.section_bytes(parent)
        prologue = _eh_prologue_site(parent_bytes, owner.value)
        if prologue is None or relocation.site != owner.value + prologue:
            continue
        handler = coff.symbols[relocation.symbol_index]
        if (handler.section <= 0 or
                parents.get(handler.section) != relocation.section or
                handler.typ != 0 or handler.storage_class != LABEL_STORAGE):
            continue
        child = coff.sections[handler.section - 1]
        if (child.name != ".text$x" or
                not child.characteristics & CNT_CODE):
            continue
        labels = sorted(local_labels.get(handler.section, ()),
                        key=lambda row: row.value)
        if not labels or labels[-1].index != handler.index:
            continue
        prior = [row for row in labels if row.value < handler.value]
        if prior:
            funclet = prior[-1]
        else:
            # a lone cleanup at the section start carries no label of its
            # own; the section symbol names that offset
            funclet = next((row for row in coff.symbols.values()
                            if row.section == handler.section and row.value == 0
                            and row.storage_class == 3 and row.name == child.name),
                           None)
            if funclet is None:
                continue
        child_bytes = coff.section_bytes(child)
        if (handler.value + 10 != child.raw_size or
                child_bytes[handler.value] != 0xB8 or
                child_bytes[handler.value + 5] != 0xE9):
            continue
        thunk_relocations = [
            row for row in relocations_by_section.get(handler.section, ())
            if handler.value <= row.site < handler.value + 10
        ]
        if (len(thunk_relocations) != 2 or
                sorted((row.site - handler.value, row.typ)
                       for row in thunk_relocations) != [(1, DIR32), (6, REL32)]):
            continue
        operand = struct.unpack_from(
            "<I", parent_bytes, relocation.site)[0]
        if operand != 0:
            continue
        candidates.append(EhHandlerOwnerRewrite(
            owner.name, relocation.section, handler.section,
            relocation.offset, relocation.site, handler.index, funclet.index,
            handler.value, funclet.value, handler.value - funclet.value,
            prologue=prologue,
        ))
    return tuple(candidates)


def _canonicalize_matching_eh_handler_owners(
        base_payload: bytes, target_payload: bytes, *,
        symbol_rvas=None, funclet_owners=None,
        ) -> tuple[bytes, tuple[EhHandlerOwnerRewrite, ...]]:
    """Mirror retail's proved ``last funclet + size`` EH relocation form.

    This is deliberately paired. A structurally valid VC6 handler is changed
    only when the unique retail counterpart has the same EH prologue and its
    relocation addend equals the candidate's measured final-funclet size.
    Different cleanup topology therefore remains visible to objdiff.
    """
    base = canon.CoffObject(base_payload)
    target = canon.CoffObject(target_payload)
    target_functions: dict[str, list[canon.Symbol]] = {}
    for symbol in target.symbols.values():
        if (symbol.section > 0 and symbol.typ == FUNCTION_TYPE and
                symbol.storage_class == EXTERNAL_STORAGE):
            target_functions.setdefault(symbol.name, []).append(symbol)
    target_relocations = {
        (row.section, row.site): row for row in target.relocations
        if row.typ == DIR32
    }

    data = bytearray(base_payload)
    admitted = []
    for rewrite in _eh_handler_candidates(base):
        counterparts = target_functions.get(rewrite.function, ())
        if len(counterparts) != 1:
            continue
        counterpart = counterparts[0]
        target_section = target.sections[counterpart.section - 1]
        target_bytes = target.section_bytes(target_section)
        target_prologue = _eh_prologue_site(target_bytes, counterpart.value)
        if target_prologue != rewrite.prologue:
            continue
        target_site = counterpart.value + target_prologue
        target_relocation = target_relocations.get(
            (counterpart.section, target_site))
        if target_relocation is None:
            continue
        target_owner = target.symbols[target_relocation.symbol_index]
        if (target_owner.section != 0 or target_owner.typ != FUNCTION_TYPE or
                target_owner.storage_class != EXTERNAL_STORAGE):
            continue
        target_addend, = struct.unpack_from("<I", target_bytes, target_site)
        if target_addend != rewrite.funclet_size:
            continue
        if not UNWIND_OWNER.search(target_owner.name):
            # Generic names carry no ownership evidence. Require the existing
            # retail inventories to identify both functions and the cleanup's
            # parent and full extent before accepting this relocation form.
            owner_address = (symbol_rvas or {}).get(target_owner.name)
            parent_address = (symbol_rvas or {}).get(rewrite.function)
            if (owner_address is None or parent_address is None or
                    owner_address[1] != "func" or parent_address[1] != "func" or
                    (funclet_owners or {}).get(owner_address[0]) !=
                    (parent_address[0], target_addend)):
                continue

        base_section = base.sections[rewrite.parent_section - 1]
        operand_offset = base_section.raw_offset + rewrite.relocation_site
        before_addend, = struct.unpack_from("<I", base.data, operand_offset)
        handler = base.symbols[rewrite.handler_symbol]
        funclet = base.symbols[rewrite.funclet_symbol]
        before_resolved = (handler.section,
                           (handler.value + before_addend) & 0xFFFFFFFF)
        after_resolved = (funclet.section,
                          (funclet.value + rewrite.funclet_size) & 0xFFFFFFFF)
        if before_resolved != after_resolved or before_resolved != (
                rewrite.child_section, rewrite.handler_offset):
            raise RuntimeError(
                "EH handler-owner normalization changed the resolved target")
        collision = next((
            symbol for symbol in base.symbols.values()
            if symbol.name == target_owner.name and
            symbol.index != rewrite.funclet_symbol
        ), None)
        if collision is not None:
            continue
        struct.pack_into("<I", data, operand_offset, rewrite.funclet_size)
        struct.pack_into("<I", data, rewrite.relocation_offset + 4,
                         rewrite.funclet_symbol)
        admitted.append(replace(
            rewrite, canonical_name=target_owner.name))

    relocation_normalized = canon.CoffObject(bytes(data))
    renames = {
        row.funclet_symbol: row.canonical_name for row in admitted
    }
    if len(renames) != len(admitted):
        raise RuntimeError("duplicate EH handler-owner funclet rewrite")
    data = bytearray(canon._rewrite_names(relocation_normalized, renames))
    normalized = canon.CoffObject(bytes(data))
    admitted_by_offset = {row.relocation_offset: row for row in admitted}
    if len(admitted_by_offset) != len(admitted):
        raise RuntimeError("duplicate EH handler-owner relocation rewrite")
    if (base.section_count != normalized.section_count or
            base.symbol_count != normalized.symbol_count or
            len(base.relocations) != len(normalized.relocations)):
        raise RuntimeError("EH handler-owner normalization changed COFF topology")
    before = bytearray(base_payload[:base.string_offset])
    after = bytearray(data[:normalized.string_offset])
    for original, changed in zip(base.sections, normalized.sections):
        before[original.header_offset:original.header_offset + 8] = bytes(8)
        after[changed.header_offset:changed.header_offset + 8] = bytes(8)
        if ((original.name, original.raw_size, original.raw_offset,
             original.reloc_offset, original.reloc_count,
             original.characteristics) !=
                (changed.name, changed.raw_size, changed.raw_offset,
                 changed.reloc_offset, changed.reloc_count,
                 changed.characteristics)):
            raise RuntimeError(
                "EH handler-owner normalization changed section metadata")
    for index, original in base.symbols.items():
        changed = normalized.symbols[index]
        before[original.offset:original.offset + 8] = bytes(8)
        after[changed.offset:changed.offset + 8] = bytes(8)
    normalized_relocations = {row.offset: row for row in normalized.relocations}
    for rewrite in admitted:
        section = base.sections[rewrite.parent_section - 1]
        operand = section.raw_offset + rewrite.relocation_site
        before[operand:operand + 4] = bytes(4)
        after[operand:operand + 4] = bytes(4)
        before[rewrite.relocation_offset + 4:
               rewrite.relocation_offset + 8] = bytes(4)
        after[rewrite.relocation_offset + 4:
              rewrite.relocation_offset + 8] = bytes(4)
        row = normalized_relocations[rewrite.relocation_offset]
        if row.symbol_index != rewrite.funclet_symbol:
            raise RuntimeError("EH handler-owner relocation target was not rewritten")
        addend, = struct.unpack_from("<I", data, operand)
        if addend != rewrite.funclet_size:
            raise RuntimeError("EH handler-owner addend was not rewritten")
    if before != after:
        raise RuntimeError(
            "EH handler-owner normalization changed unexpected COFF bytes")
    for original, changed in zip(base.relocations, normalized.relocations):
        rewrite = admitted_by_offset.get(original.offset)
        expected_symbol = (rewrite.funclet_symbol if rewrite else
                           original.symbol_index)
        if ((original.section, original.site, original.typ) !=
                (changed.section, changed.site, changed.typ) or
                changed.symbol_index != expected_symbol):
            raise RuntimeError(
                "EH handler-owner normalization changed an unexpected relocation")
    for index, original in base.symbols.items():
        changed = normalized.symbols[index]
        expected_name = renames.get(index, original.name)
        if (changed.name != expected_name or
                (original.value, original.section, original.typ,
                 original.storage_class, original.aux_count) !=
                (changed.value, changed.section, changed.typ,
                 changed.storage_class, changed.aux_count)):
            raise RuntimeError(
                "EH handler-owner normalization changed an unexpected symbol")
    return bytes(data), tuple(admitted)


def _retain_matching_target_padding(base_payload: bytes,
                                    target_payload: bytes) -> tuple[bytes, int]:
    """Retain linked-target NOP fill when the logical function sizes agree.

    VC6 emits each base function in its own padded /Gy section, whereas the
    delinked retail object packs every function into one .text section. The
    canonicalizer initially removes all trailing base fill. If retail's next
    function is 4-byte aligned, objdiff still assigns the intervening NOPs to
    the previous function. Restore only that proven suffix, without changing
    either function's logical code extent.
    """
    data = bytearray(base_payload)
    base = canon.CoffObject(base_payload)
    target = canon.CoffObject(target_payload)

    base_functions: dict[int, list] = {}
    for symbol in base.symbols.values():
        if symbol.typ == FUNCTION_TYPE and symbol.section > 0:
            base_functions.setdefault(symbol.section, []).append(symbol)

    target_functions: dict[int, list] = {}
    target_by_name: dict[str, list] = {}
    for symbol in target.symbols.values():
        if symbol.typ != FUNCTION_TYPE or symbol.section <= 0:
            continue
        target_functions.setdefault(symbol.section, []).append(symbol)
        target_by_name.setdefault(symbol.name, []).append(symbol)

    retained = 0
    for section_index, functions in base_functions.items():
        # A normal /Gy contribution owns exactly one external function. Skip
        # unusual multi-function sections rather than guessing their extents.
        if len(functions) != 1:
            continue
        function = functions[0]
        section = base.sections[section_index - 1]
        if function.value != 0 or not section.characteristics & CNT_CODE:
            continue
        counterparts = target_by_name.get(function.name, ())
        if len(counterparts) != 1:
            continue
        counterpart = counterparts[0]
        target_section = target.sections[counterpart.section - 1]
        later = [
            row.value for row in target_functions[counterpart.section]
            if row.value > counterpart.value
        ]
        end = min(later) if later else target_section.raw_size
        if end <= counterpart.value:
            continue
        extent = end - counterpart.value
        target_bytes = target.section_bytes(target_section)[counterpart.value:end]
        pad = 0
        while (pad < min(TEXT_PAD_TRIM_LIMIT, len(target_bytes))
               and target_bytes[-1 - pad] == 0x90):
            pad += 1
        if not pad or section.raw_size != extent - pad:
            continue

        # Shrinking a section header leaves its original bytes in the file.
        # Require those hidden bytes to be the exact same NOP suffix before
        # making them visible again.
        fill_start = section.raw_offset + section.raw_size
        fill_end = section.raw_offset + extent
        if base_payload[fill_start:fill_end] != b"\x90" * pad:
            continue
        struct.pack_into("<I", data, section.header_offset + 16, extent)
        retained += 1

    return bytes(data), retained


def data_names_for_unit(unit: str) -> dict[str, str]:
    data_names = {}
    if DATA_MANIFEST.is_file():
        from homm3.core.tsv import read
        from homm3.core.msvc_names import mask
        for row in read(DATA_MANIFEST)[2]:
            if row['object'] == unit + '.c' and '$RVA' in row['name']:
                key = mask(row['name'])
                if key in data_names and data_names[key] != row['name']:
                    raise ValueError(f"ambiguous local data identity in {unit}: {key}")
                data_names[key] = row['name']
    return data_names


def _canonicalize_side(side: str, obj: Path, context=None) -> bool:
    """Write the normalized copy + sidecar + stamp of one raw object unless
    the existing copy is fresh; True when written."""
    root = OBJDIFF / side
    rel = obj.relative_to(root)
    unit = rel.name[:-6] if rel.name.endswith(".c.obj") else rel.stem
    out = OBJDIFF / "normalized" / side / rel
    sidecar = out.with_suffix(".symbols.tsv")
    out.parent.mkdir(parents=True, exist_ok=True)
    stamp_inputs = {"raw": obj}
    stamp_inputs.update(canon.anon_ns_stamp_inputs())
    if DATA_MANIFEST.is_file():
        stamp_inputs['data_manifest'] = DATA_MANIFEST
    if COMPGEN_MANIFEST.is_file():
        stamp_inputs["compgen_manifest"] = COMPGEN_MANIFEST
    if (out.exists() and sidecar.is_file()
            and not freshness_problems(out, required_inputs=stamp_inputs, context=context)):
        return False
    claims = ()
    accounted = frozenset()
    if COMPGEN_MANIFEST.is_file():
        claims = canon.load_compgen_claims(COMPGEN_MANIFEST, unit)
        accounted = canon.load_compgen_claim_names(COMPGEN_MANIFEST, unit)
    data_claims = (canon.load_compgen_data_claims(DATA_MANIFEST, unit)
                   if DATA_MANIFEST.is_file() else ())
    data_names = data_names_for_unit(unit)
    result = canon.canonicalize_coff(obj.read_bytes(), claims, data_claims,
                                     compgen_accounted=accounted, unit=unit, data_names=data_names)
    out.write_bytes(result.data)
    sidecar.write_bytes(canon.sidecar_bytes(result.rows))
    write_stamp(out, stamp_inputs, context=context)
    # The ICF index reads every normalized object and is not part of any
    # stamp: a later pair must never use one built before this rewrite (a
    # scoped pass followed by the complete fallback, or a header-only change
    # in another unit). Rebuilding it is cheap: bodies are content-cached.
    global _ICF_INDEX
    _ICF_INDEX = None
    return True


_GUARD_OWNERS = None
_CLAIMED_GUARD = re.compile(r"__h3cg\$(?P<unit>[^$]+)\$static_init_guard\$\w+")


def _guard_owners() -> dict:
    """{guard claim name: (owner static, address)} from DATA_COMPGEN_GUARD."""
    global _GUARD_OWNERS
    if _GUARD_OWNERS is None:
        from homm3.verify.byte_accounting import _guard_owners as scan
        _GUARD_OWNERS = scan(common.HOMM3_DIR)
    return _GUARD_OWNERS


def _canonicalize_claimed_guards(base_payload: bytes, target_payload: bytes,
                                 unit: str, guard_owners=None) -> tuple[bytes, int]:
    """Name the candidate's local-static guard by the target's guard claim.

    A function-local static's guard byte is cl's `$S<n>` symbol in the owner
    static's scope. When reloc_pairing does not pair it, the delinked target
    names the retail byte by its DATA_COMPGEN_GUARD claim instead, and the two
    references then compare by objdiff's fallback over the referenced bytes,
    whose extent follows cl's run-to-run .bss order. Where the target uses a
    claim of this unit and the candidate has exactly one guard in the claimed
    owner's scope (byte_accounting.bridge_data_name), the candidate symbol
    takes the claim name, so both sides name one guard identity. A target
    that already spells the guard `$S<n>` is left alone.
    """
    from homm3.verify.byte_accounting import bridge_data_name
    owners = _guard_owners() if guard_owners is None else guard_owners
    target = canon.CoffObject(target_payload)
    claims = sorted({symbol.name for symbol in target.symbols.values()
                     if (m := _CLAIMED_GUARD.fullmatch(symbol.name)) and m["unit"] == unit
                     and symbol.name in owners})
    if not claims:
        return base_payload, 0
    base = canon.CoffObject(base_payload)
    names = {symbol.name for symbol in base.symbols.values()}
    emitted = {name: True for name in names}
    renames: dict[int, str] = {}
    for claim in claims:
        if claim in names:
            continue
        spelling = bridge_data_name(claim, emitted, owners)
        if spelling is None:
            continue
        indices = [symbol.index for symbol in base.symbols.values()
                   if symbol.name == spelling]
        if len(indices) == 1 and indices[0] not in renames:
            renames[indices[0]] = claim
    if not renames:
        return base_payload, 0
    return canon._rewrite_names(base, renames), len(renames)


_MASKED_GUARD = re.compile(r"_\?\$S@.+@4EA")


def _canonicalize_masked_guards(base_payload: bytes, target_payload: bytes) -> tuple[bytes, int]:
    """Name the candidate's local-static guard by the target's masked spelling.

    Another image's placements name a shared unit's guard by the model's
    spelling, with cl's volatile `$S<n>` counter masked (`_?$S@?1??f@@...@4EA`,
    homm3.core.msvc_names.mask). The candidate keeps `$S<n>`, so the two
    references would compare by objdiff's fallback over the referenced bytes,
    whose extent follows cl's run-to-run .bss order (a guard runs 1 byte
    before another guard or the section end and 4 before an array). Where
    exactly one candidate symbol masks to a guard the target defines, it
    takes the target's name.
    """
    from homm3.core import msvc_names
    target = canon.CoffObject(target_payload)
    guards = {symbol.name for symbol in target.symbols.values()
              if symbol.section > 0 and _MASKED_GUARD.fullmatch(symbol.name)}
    if not guards:
        return base_payload, 0
    base = canon.CoffObject(base_payload)
    names = {symbol.name for symbol in base.symbols.values()}
    spellings: dict[str, list[int]] = {}
    for symbol in base.symbols.values():
        if symbol.name.startswith("_?$S") and symbol.name not in guards:
            spellings.setdefault(msvc_names.mask(symbol.name), []).append(symbol.index)
    renames = {indices[0]: guard for guard in sorted(guards)
               if guard not in names and len(indices := spellings.get(guard, ())) == 1}
    if not renames:
        return base_payload, 0
    return canon._rewrite_names(base, renames), len(renames)


_WEAK_EXTERNAL = 105
_OLDNAMES: dict[str, str] | None = None


def oldnames_aliases(library: Path | None = None) -> dict[str, str]:
    """{old name: CRT name} from the pinned toolchain's OLDNAMES.LIB.

    Each member defines one weak external (`_strcmpi`) whose default is the
    underscored CRT function (`__strcmpi`); LINK binds a call spelled with
    the old POSIX name to that function."""
    global _OLDNAMES
    if library is None and _OLDNAMES is not None:
        return _OLDNAMES
    from homm3.core.cc_wrap import find_ci, msvc_dir
    from homm3.verify.library_code import archive_members
    path = library or find_ci(msvc_dir() / "lib", "OLDNAMES.LIB")
    aliases: dict[str, str] = {}
    if path is not None and Path(path).is_file():
        for _member, body in archive_members(Path(path)):
            aliases.update(_weak_externals(body))
    if library is None:
        _OLDNAMES = aliases
    return aliases


def _weak_externals(body: bytes) -> dict[str, str]:
    """{weak external: its default} of one COFF object; OLDNAMES alias
    members are machine-independent (machine 0) COFF objects."""
    if len(body) < 20:
        return {}
    _machine, _sections, _stamp, table, count = struct.unpack_from("<HHIII", body)
    strings = table + 18 * count

    def name(at: int) -> str:
        if body[at:at + 4] == b"\0\0\0\0":
            start = strings + struct.unpack_from("<I", body, at + 4)[0]
            return body[start:body.index(b"\0", start)].decode("latin-1")
        return body[at:at + 8].split(b"\0", 1)[0].decode("latin-1")

    out = {}
    index = 0
    while index < count:
        at = table + 18 * index
        storage, aux = body[at + 16], body[at + 17]
        if storage == _WEAK_EXTERNAL and aux:
            tag = struct.unpack_from("<I", body, at + 18)[0]
            out[name(at)] = name(table + 18 * tag)
        index += 1 + aux
    return out


def _resolve_oldnames(payload: bytes, aliases: dict[str, str] | None = None) -> tuple[bytes, int]:
    """Bind undefined old POSIX names to their CRT functions, as LINK does
    through OLDNAMES.LIB: retail's code reaches `__strcmpi` whichever name
    the source called it by."""
    aliases = oldnames_aliases() if aliases is None else aliases
    coff = canon.CoffObject(payload)
    renames = {symbol.index: aliases[symbol.name] for symbol in coff.symbols.values()
               if symbol.section == 0 and symbol.storage_class == EXTERNAL_STORAGE
               and symbol.value == 0 and symbol.name in aliases}
    if not renames:
        return payload, 0
    return canon._rewrite_names(coff, renames), len(renames)


def _resolve_weak_defaults(base_payload: bytes, target_payload: bytes) -> tuple[bytes, int]:
    """Bind references to a weak external to its default, as LINK does when
    nothing defines the weak name strongly.

    VC6 spells a class's vector deleting destructor `??_E` as a weak external
    whose default is the scalar `??_G` it defines; a vtordisp thunk jumps to
    `??_E`, and the linked image reaches the `??_G` body. Only a reference
    whose default this object defines, and whose default the retail object
    names, is retargeted. Returns (base, retargeted references)."""
    base = canon.CoffObject(base_payload)
    target_names = {symbol.name for symbol in canon.CoffObject(target_payload).symbols.values()}
    defined: dict[str, int] = {}
    for symbol in sorted(base.symbols.values(), key=lambda row: row.index):
        if symbol.section > 0 and symbol.storage_class == EXTERNAL_STORAGE:
            defined.setdefault(symbol.name, symbol.index)
    defaults: dict[int, int] = {}
    for symbol in base.symbols.values():
        if symbol.storage_class != _WEAK_EXTERNAL or symbol.aux_count == 0:
            continue
        # The tag may be an undefined record of the name; the definition
        # comes further down the symbol table.
        tag = struct.unpack_from("<I", base_payload, symbol.offset + 18)[0]
        default = base.symbols.get(tag)
        if (default is not None and default.name in defined
                and default.name in target_names and symbol.name not in target_names):
            defaults[symbol.index] = defined[default.name]
    retargets = [relocation for relocation in base.relocations
                 if relocation.symbol_index in defaults]
    if not retargets:
        return base_payload, 0
    data = bytearray(base_payload)
    for relocation in retargets:
        struct.pack_into("<I", data, relocation.offset + 4, defaults[relocation.symbol_index])
    return bytes(data), len(retargets)


def canonicalize_pair(base_payload: bytes, target_payload: bytes, unit: str,
                      symbol_rvas, *, image_base=None, identities=None
                      ) -> tuple[bytes, bytes, Counter]:
    """Apply the shared full-build and candidate-search paired passes."""
    counts: Counter = Counter()
    base_payload, oldnames = _resolve_oldnames(base_payload)
    counts["oldnames"] += oldnames
    base_payload, weak = _resolve_weak_defaults(base_payload, target_payload)
    counts["weak"] += weak
    base_payload, guard_count = _canonicalize_claimed_guards(
        base_payload, target_payload, unit)
    counts["guard"] += guard_count
    base_payload, guard_count = _canonicalize_masked_guards(base_payload, target_payload)
    counts["guard"] += guard_count
    padded, count = _retain_matching_target_padding(
        base_payload, target_payload)
    counts["retained"] += count
    paired_base, base_literal_count = \
        _canonicalize_except_list_literals(
            padded, target_payload)
    counts["literal"] += base_literal_count
    paired_target, literal_count, aggregate_count = \
        _canonicalize_equivalent_relocations(
            paired_base, target_payload, symbol_rvas,
            image_base=retail_image_base() if image_base is None else image_base)
    counts["literal"] += literal_count
    counts["aggregate"] += aggregate_count
    from homm3.build import identity_relocations
    if identities is None:
        identities = (identity_relocations.load_identities(ADDRESS_IDENTITIES),
                      identity_relocations.load_library_names(ADDRESS_IDENTITIES))
    paired_base, paired_target, icf_count = _canonicalize_icf_aliases(
        paired_base, paired_target, symbol_rvas, _icf_index(), _retail_twins(),
        lambda name, rva: identity_relocations.resolve_name(
            name, symbol_rvas, identities[0], unit) == rva)
    counts["icf"] += icf_count
    paired_target, address_count = _canonicalize_equivalent_data_addresses(
        paired_base, paired_target, symbol_rvas)
    counts["address"] += address_count
    paired_base, paired_target, literal_sections = \
        _canonicalize_zero_literal_sections(paired_base, paired_target)
    counts["zero_literal"] += literal_sections
    normalized, rewrites = _canonicalize_matching_eh_handler_owners(
        paired_base, paired_target, symbol_rvas=symbol_rvas,
        funclet_owners=_retail_funclet_owners())
    counts["eh"] += len(rewrites)
    paired_target, identity_count = identity_relocations.canonicalize(
        normalized, paired_target, symbol_rvas, identities[0], unit=unit,
        library_names=identities[1])
    counts["identity"] += identity_count
    normalized, fold_count = _canonicalize_proven_fold_calls(
        normalized, paired_target, symbol_rvas, identities[0], unit)
    counts["fold"] += fold_count
    return normalized, paired_target, counts


def _canonicalize_proven_fold_calls(
        base_payload: bytes, target_payload: bytes,
        symbol_rvas: dict[str, tuple[int, str]],
        identities: dict[tuple[str, str], set[int]], unit: str = "",
        ) -> tuple[bytes, int]:
    """Name every candidate call to a proven fold by retail's label.

    `address_identities.tsv` proves some unclaimed names at a claimed retail
    function: an identical-body fold (`getArmy` and `getArmy const`,
    `sRandom` and `random`), a library alias or an import thunk. Every call
    to such a name reaches that one function, so the candidate's REL32
    references to it take the claimed label, at shifted sites too (the paired
    passes already cover the aligned ones). Only an undefined candidate
    symbol with no claim of its own is renamed, only to the single claimed
    function at its proven address, and only when the retail object
    references that label. Returns (base, rewritten references).
    """
    from homm3.build import identity_relocations
    base = canon.CoffObject(base_payload)
    target = canon.CoffObject(target_payload)
    target_names = {symbol.name for symbol in target.symbols.values()}
    labels: dict[int, set[str]] = {}
    for name, (rva, kind) in symbol_rvas.items():
        if kind == "func":
            labels.setdefault(rva, set()).add(name)
    retargets: list[tuple[canon.Relocation, str]] = []
    for relocation in base.relocations:
        if relocation.typ != REL32:
            continue
        symbol = base.symbols[relocation.symbol_index]
        name = symbol.name
        if (symbol.section != 0 or name in symbol_rvas or name in target_names):
            continue
        rva = identity_relocations.resolve_name(name, symbol_rvas, identities, unit)
        names = labels.get(rva) if rva is not None else None
        if not names or len(names) != 1:
            continue
        (label,) = names
        if label != name and label in target_names:
            retargets.append((relocation, label))
    if not retargets:
        return base_payload, 0
    existing: dict[str, int] = {}
    for symbol in sorted(base.symbols.values(), key=lambda row: row.index):
        if symbol.storage_class == EXTERNAL_STORAGE:
            existing.setdefault(symbol.name, symbol.index)
    missing = sorted({label for _r, label in retargets if label not in existing})
    if missing:
        base_payload, appended = _append_undefined_symbols(base_payload, missing)
        existing.update(appended)
    data = bytearray(base_payload)
    for relocation, label in retargets:
        struct.pack_into("<I", data, relocation.offset + 4, existing[label])
    return bytes(data), len(retargets)


def _pair_unit(rel: Path, symbol_rvas, context=None, *, image_base=None,
               identities=None) -> Counter:
    """The paired base/target passes for one unit (padding retention,
    __except_list literals, equivalent relocations, EH handler owners);
    a no-op unless both normalized copies exist."""
    counts: Counter = Counter()
    base_obj = OBJDIFF / "base" / rel
    target_rel = rel.with_name(rel.stem + ".c.obj")
    target_obj = OBJDIFF / "target" / target_rel
    normalized_base = OBJDIFF / "normalized/base" / rel
    normalized_target = OBJDIFF / "normalized/target" / target_rel
    if not (target_obj.is_file() and normalized_base.is_file()
            and normalized_target.is_file()):
        return counts
    stamp_inputs = {
        "project": common.HOMM3_DIR / "config/project.toml",
        "raw": base_obj, "target": target_obj, "symbol_names": SYMBOL_NAMES,
    }
    target_stamp_inputs = {
        "project": common.HOMM3_DIR / "config/project.toml",
        "raw": target_obj, "base": base_obj, "symbol_names": SYMBOL_NAMES,
    }
    for label, inventory in (("retail_funclets", FUNCLETS),
                             ("retail_functions", FUNCTIONS),
                             ("address_identities", ADDRESS_IDENTITIES)):
        if inventory.is_file():
            stamp_inputs[label] = inventory
            target_stamp_inputs[label] = inventory
    stamp_inputs.update(canon.anon_ns_stamp_inputs())
    target_stamp_inputs.update(canon.anon_ns_stamp_inputs())
    if DATA_MANIFEST.is_file():
        stamp_inputs['data_manifest'] = DATA_MANIFEST
        target_stamp_inputs['data_manifest'] = DATA_MANIFEST
    if COMPGEN_MANIFEST.is_file():
        stamp_inputs["compgen_manifest"] = COMPGEN_MANIFEST
        target_stamp_inputs["compgen_manifest"] = COMPGEN_MANIFEST
    # Verify both complete paired stamps, including content hashes. A fresh
    # raw-only stamp from _canonicalize_side is not proof of a paired result.
    # This avoids reparsing every COFF object on unchanged fast builds/diffs.
    if (not freshness_problems(normalized_base, required_inputs=stamp_inputs, context=context)
            and not freshness_problems(normalized_target,
                                       required_inputs=target_stamp_inputs, context=context)):
        return counts
    normalized, paired_target, counts = canonicalize_pair(
        normalized_base.read_bytes(), normalized_target.read_bytes(), rel.stem,
        symbol_rvas, image_base=image_base, identities=identities)
    normalized_base.write_bytes(normalized)
    normalized_target.write_bytes(paired_target)
    # Padding is a paired normalization decision, so the base copy is
    # stale whenever either raw input changes, even when this run found no
    # suffix to retain.
    write_stamp(normalized_base, stamp_inputs, context=context)
    write_stamp(normalized_target, target_stamp_inputs, context=context)
    return counts


def retail_image_base() -> int:
    from homm3.core.project import Project
    return Project(common.HOMM3_DIR).image.image_base


def normalize_unit(unit: str, symbol_rvas=None) -> Counter:
    """Refresh ONE unit's comparison copies: both raw sides canonicalized
    when stale, then the paired passes. The same steps `main` runs for
    every unit, so `homm3 sema diff` can refresh the unit it is about to
    compare instead of demanding a tree-wide build. Counters: wrote,
    retained, literal, aggregate, eh."""
    counts: Counter = Counter()
    base_obj = OBJDIFF / "base" / f"{unit}.obj"
    target_obj = OBJDIFF / "target" / f"{unit}.c.obj"
    context = ValidationContext()
    for side, obj in (("base", base_obj), ("target", target_obj)):
        if obj.is_file() and _canonicalize_side(side, obj, context):
            counts["wrote"] += 1
    if base_obj.is_file():
        if symbol_rvas is None:
            symbol_rvas = _retail_symbol_rvas()
        counts.update(_pair_unit(Path(f"{unit}.obj"), symbol_rvas, context))
    return counts


def _raw_objects(side: str, units: set[str] | None) -> list[Path]:
    root = OBJDIFF / side
    if not root.is_dir():
        return []
    if units is None:
        return sorted(root.rglob("*.obj"))
    suffix = ".obj" if side == "base" else ".c.obj"
    return [obj for obj in (root / f"{unit}{suffix}" for unit in sorted(units))
            if obj.is_file()]


def normalize_all(units: set[str] | None = None) -> Counter:
    """Refresh comparison copies of every unit, or only of `units`.

    Selected units get exactly the passes a full run gives them; unselected
    units' copies are left as they are (callers verify their freshness).
    """
    wrote = skipped = 0
    context = ValidationContext()
    for side in ("base", "target"):
        for obj in _raw_objects(side, units):
            if _canonicalize_side(side, obj, context):
                wrote += 1
            else:
                skipped += 1
    counts: Counter = Counter()
    symbol_rvas = _retail_symbol_rvas()
    image_base = retail_image_base()
    from homm3.build import identity_relocations
    identities = (identity_relocations.load_identities(ADDRESS_IDENTITIES),
                  identity_relocations.load_library_names(ADDRESS_IDENTITIES))
    base_root = OBJDIFF / "base"
    for base_obj in _raw_objects("base", units):
        counts.update(_pair_unit(base_obj.relative_to(base_root), symbol_rvas, context,
                                 image_base=image_base, identities=identities))
    print(f"[build normalize_objs] {wrote} normalized, {skipped} fresh, "
          f"{counts['retained']} target-padding span(s) retained "
          f"{counts['eh']} EH handler-owner relocation(s) canonicalized "
          f"{counts['literal']} false-literal relocation(s) removed "
          f"{counts['aggregate']} aggregate/field relocation(s) canonicalized "
          f"{counts['icf']} ICF twin reference(s) named "
          f"{counts['guard']} static guard(s) named "
          f"{counts['zero_literal']} zero literal(s) given retail sections "
          f"{counts['address']} equivalent data address(es) named "
          f"{counts['identity']} relocation(s) compared by proven address "
          f"{counts['fold']} proven-fold call(s) named "
          f"{counts['weak']} weak-external reference(s) bound to their default "
          f"-> {OBJDIFF / 'normalized'}")
    counts.update(wrote=wrote, skipped=skipped)
    return counts


def main(argv=None) -> int:
    normalize_all()
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
