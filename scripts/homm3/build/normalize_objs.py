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

Known trade-off: the stamp records data inputs only; a change to the
canonicalizer's own code is not detected. Bump STAMP_SCHEMA (which
invalidates every stamp) when the transform changes behavior.

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
import re
import struct
import sys
from dataclasses import dataclass, replace
from pathlib import Path

from homm3.build import canonicalize_data_symbols as canon
from homm3.build.normalized_freshness import freshness_problems, write_stamp
from homm3.core import common

OBJDIFF = common.HOMM3_DIR / "build/objdiff"
COMPGEN_MANIFEST = common.HOMM3_DIR / "build/gen/compgen_claims.tsv"

CNT_CODE = 0x00000020
DIR32 = 0x0006
FUNCTION_TYPE = 0x0020
EXTERNAL_STORAGE = 2
STATIC_STORAGE = 3
LABEL_STORAGE = 6
REL32 = 0x0014
TEXT_PAD_TRIM_LIMIT = 15
ASSOCIATIVE_COMDAT = 5
UNWIND_OWNER = re.compile(r"(?:^|_)unwind[0-9]+$")
SYMBOL_NAMES = common.HOMM3_DIR / "build/gen/symbol_names.csv"
IMAGE_BASE = 0x00400000


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
        symbol_rvas: dict[str, tuple[int, str]],
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
    inferred_anchors = {
        symbol: next(iter(rows)) for symbol, rows in observations.items()
        if len(rows) == 1
    }

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
        resolved = (IMAGE_BASE + authority[0] + target_addend) & 0xFFFFFFFF
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
        if owner is None or relocation.site != owner.value + 6:
            continue
        parent = coff.sections[relocation.section - 1]
        parent_bytes = coff.section_bytes(parent)
        if parent_bytes[owner.value:owner.value + 6] != b"\x55\x8b\xec\x6a\xff\x68":
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
        if not prior:
            continue
        funclet = prior[-1]
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
        ))
    return tuple(candidates)


def _canonicalize_matching_eh_handler_owners(
        base_payload: bytes, target_payload: bytes,
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
        target_site = counterpart.value + 6
        if (target_bytes[counterpart.value:counterpart.value + 6] !=
                b"\x55\x8b\xec\x6a\xff\x68"):
            continue
        target_relocation = target_relocations.get(
            (counterpart.section, target_site))
        if target_relocation is None:
            continue
        target_owner = target.symbols[target_relocation.symbol_index]
        if (target_owner.section != 0 or target_owner.typ != FUNCTION_TYPE or
                target_owner.storage_class != EXTERNAL_STORAGE or
                not UNWIND_OWNER.search(target_owner.name)):
            continue
        target_addend, = struct.unpack_from("<I", target_bytes, target_site)
        if target_addend != rewrite.funclet_size:
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


def _drop_data_sections(payload: bytes) -> bytes:
    """Truncate every non-code section in a comparison copy to zero.

    The matching scope is FUNCTIONS ONLY for now (user decision
    2026-08-06): data comparison returns later as its own phase. The
    raw base/delinked objects keep their data sections untouched -
    only the disposable objdiff copies are scoped, so flipping this
    call back re-admits data wholesale. Section headers stay in place
    (no renumbering); raw size and relocation count drop to zero."""
    data = bytearray(payload)
    nsec, = struct.unpack_from("<H", data, 2)
    dropped = set()
    for index in range(nsec):
        offset = 20 + index * 40
        characteristics, = struct.unpack_from("<I", data, offset + 36)
        if characteristics & CNT_CODE:
            continue
        dropped.add(index + 1)
        struct.pack_into("<I", data, offset + 16, 0)   # SizeOfRawData
        struct.pack_into("<H", data, offset + 32, 0)   # NumberOfRelocations
    # Symbols defined in a dropped section become undefined externs in
    # the copy - .text relocations keep resolving them by name, and the
    # differ no longer sees extents pointing past the emptied section.
    symoff, nsyms = struct.unpack_from("<II", data, 8)
    o, i = symoff, 0
    while i < nsyms:
        section, = struct.unpack_from("<h", data, o + 12)
        if section in dropped:
            struct.pack_into("<I", data, o + 8, 0)     # Value
            struct.pack_into("<h", data, o + 12, 0)    # SectionNumber
        aux = data[o + 17]
        o += 18 * (1 + aux)
        i += 1 + aux
    return bytes(data)


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


def main(argv=None) -> int:
    argv = list(argv or [])
    wrote = skipped = 0
    for side in ("base", "target"):
        root = OBJDIFF / side
        out_root = OBJDIFF / "normalized" / side
        if not root.is_dir():
            continue
        for obj in sorted(root.rglob("*.obj")):
            rel = obj.relative_to(root)
            unit = rel.name[:-6] if rel.name.endswith(".c.obj") else rel.stem
            claims = ()
            if COMPGEN_MANIFEST.is_file():
                claims = canon.load_compgen_claims(COMPGEN_MANIFEST, unit)
            out = out_root / rel
            sidecar = out.with_suffix(".symbols.tsv")
            out.parent.mkdir(parents=True, exist_ok=True)
            if out.exists() and sidecar.is_file() \
                    and not freshness_problems(out):
                skipped += 1
                continue
            result = canon.canonicalize_coff(obj.read_bytes(), claims)
            out.write_bytes(_drop_data_sections(result.data))
            sidecar.write_bytes(canon.sidecar_bytes(result.rows))
            stamp_inputs = {"raw": obj}
            if COMPGEN_MANIFEST.is_file():
                stamp_inputs["compgen_manifest"] = COMPGEN_MANIFEST
            write_stamp(out, stamp_inputs)
            wrote += 1
    retained = 0
    eh_rewritten = 0
    literal_rewritten = 0
    aggregate_rewritten = 0
    symbol_rvas = _retail_symbol_rvas()
    base_root = OBJDIFF / "base"
    for base_obj in sorted(base_root.rglob("*.obj")):
        rel = base_obj.relative_to(base_root)
        target_rel = rel.with_name(rel.stem + ".c.obj")
        target_obj = OBJDIFF / "target" / target_rel
        normalized_base = OBJDIFF / "normalized/base" / rel
        normalized_target = OBJDIFF / "normalized/target" / target_rel
        if not (target_obj.is_file() and normalized_base.is_file()
                and normalized_target.is_file()):
            continue
        padded, count = _retain_matching_target_padding(
            normalized_base.read_bytes(), normalized_target.read_bytes())
        if count:
            retained += count
        paired_base, base_literal_count = \
            _canonicalize_except_list_literals(
                padded, normalized_target.read_bytes())
        literal_rewritten += base_literal_count
        paired_target, literal_count, aggregate_count = \
            _canonicalize_equivalent_relocations(
                paired_base, normalized_target.read_bytes(), symbol_rvas)
        literal_rewritten += literal_count
        aggregate_rewritten += aggregate_count
        normalized, rewrites = _canonicalize_matching_eh_handler_owners(
            paired_base, paired_target)
        if rewrites:
            eh_rewritten += len(rewrites)
        if count or base_literal_count or rewrites:
            normalized_base.write_bytes(normalized)
        if literal_count or aggregate_count:
            normalized_target.write_bytes(paired_target)
        # Padding is a paired normalization decision, so the base copy is
        # stale whenever either raw input changes, even when this run found no
        # suffix to retain.
        stamp_inputs = {
            "raw": base_obj,
            "target": target_obj,
            "symbol_names": SYMBOL_NAMES,
        }
        if COMPGEN_MANIFEST.is_file():
            stamp_inputs["compgen_manifest"] = COMPGEN_MANIFEST
        write_stamp(normalized_base, stamp_inputs)
        target_stamp_inputs = {
            "raw": target_obj,
            "base": base_obj,
            "symbol_names": SYMBOL_NAMES,
        }
        if COMPGEN_MANIFEST.is_file():
            target_stamp_inputs["compgen_manifest"] = COMPGEN_MANIFEST
        write_stamp(normalized_target, target_stamp_inputs)

    print(f"[build normalize_objs] {wrote} normalized, {skipped} fresh, "
          f"{retained} target-padding span(s) retained "
          f"{eh_rewritten} EH handler-owner relocation(s) canonicalized "
          f"{literal_rewritten} false-literal relocation(s) removed "
          f"{aggregate_rewritten} aggregate/field relocation(s) canonicalized "
          f"-> {OBJDIFF / 'normalized'}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
