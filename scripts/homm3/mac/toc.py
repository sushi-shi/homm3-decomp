"""Resolve MWOB TOC references through reviewed data and PEF loader pointers."""
from __future__ import annotations

import hashlib
import re
import tomllib
from pathlib import Path

from homm3.mac.loader import Loader
from homm3.mac import jump_tables, vtables
from homm3.mac.object import CodeHunk, DataHunk, ObjectError
from homm3.mac.pef import PEF
from homm3.mac.relocations import Address, TocBinding
from homm3.mac.source import data_rows, load_data


def _function_descriptors(root: Path, pef: PEF, loader: Loader, toc: Address,
                          *, unit: str | None, retail_va: int | None) -> dict[str, tuple[Address, str]]:
    """Pin compiler-generated function transition vectors used through the TOC."""
    result = {}
    for path in sorted((root / "config/mac/function_descriptors").glob("*.toml")):
        for row in tomllib.loads(path.read_text()).get("descriptors", []):
            owners = row.get("owner_vas")
            if (not isinstance(owners, list) or not owners
                    or any(not isinstance(va, int) or va <= 0 for va in owners)):
                raise ObjectError(f"invalid function-descriptor owners in {path}")
            if row.get("unit") != unit or retail_va not in owners:
                continue
            name, code_name = row["symbol"], row["code_symbol"]
            if (not isinstance(name, str) or not re.fullmatch(r"[A-Za-z_][\w$]*", name)
                    or code_name != "." + name or name in result
                    or not isinstance(row.get("evidence"), str) or not row["evidence"].strip()
                    or any(not isinstance(row.get(key), int) or row[key] < 0
                           for key in ("mac_section", "mac_offset", "code_section", "code_offset", "code_size"))
                    or row["code_size"] == 0 or row["mac_offset"] % 4 or row["code_offset"] % 4
                    or any(not isinstance(row.get(key), str) or not re.fullmatch(r"[0-9a-f]{64}", row[key])
                           for key in ("sha256", "code_sha256"))):
                raise ObjectError(f"invalid reviewed function descriptor in {path}")
            descriptor = Address(row["mac_section"], row["mac_offset"])
            code_target = Address(row["code_section"], row["code_offset"])
            if (pef.section(descriptor.section).kind not in (1, 2, 3)
                    or pef.section(code_target.section).kind != 0
                    or hashlib.sha256(pef.read(descriptor.section, descriptor.offset, 8)).hexdigest() != row["sha256"]
                    or hashlib.sha256(pef.read(code_target.section, code_target.offset,
                                               row["code_size"])).hexdigest() != row["code_sha256"]
                    or loader.pointers.get(descriptor) != code_target
                    or loader.pointers.get(Address(descriptor.section, descriptor.offset + 4)) != toc):
                raise ObjectError(f"reviewed function descriptor changed: {name!r}")
            result[name] = (descriptor, code_name)
    return result


def bindings(root: Path, pef: PEF, code: CodeHunk,
             hunks: tuple[DataHunk, ...], *, unit: str | None = None,
             retail_va: int | None = None, target_origin: Address | None = None,
             target_size: int | None = None) -> dict[str, TocBinding]:
    references = [(kind, name) for _, kind, name in code.xrefs
                  if kind in ("HUNK_XREF_16BIT_IL", "HUNK_XREF_16BIT")]
    if not references:
        if any(row.get("owner_va") == retail_va and row.get("unit") == unit
               for row in data_rows(root, "jump_tables")):
            raise ObjectError("reviewed jump table has no candidate TOC reference")
        return {}
    loader = Loader(pef)
    toc = loader.toc()
    descriptors = _function_descriptors(root, pef, loader, toc,
                                        unit=unit, retail_va=retail_va)
    tables = jump_tables.bindings(root, pef, loader, code, hunks,
                                 unit=unit, retail_va=retail_va)
    external_vtables = vtables.external_bindings(root, pef, loader, code, hunks,
                                                 unit=unit, retail_va=retail_va)

    def verified(section, offset, size, digest, *, declaration_only=False, literal=False):
        # PEF code sections can contain literal pools. Anonymous literals and
        # source-proven const objects use that route; mutable storage cannot.
        if size <= 0 or pef.section(section).kind not in ((0, 1, 2, 3) if literal else (1, 2, 3)):
            raise ObjectError("invalid reviewed Mac data span")
        payload = pef.read(section, offset, size)
        if hashlib.sha256(payload).hexdigest() != digest:
            raise ObjectError(f"reviewed Mac data changed at {section}+{offset:#x}")
        if not declaration_only and any(at.section == section and offset < at.offset + 4 and at.offset < offset + size
               for at in loader.pointers):
            raise ObjectError("relocatable data payloads require additional matching support")
        return payload

    named = {}
    for pair in load_data(root):
        if pair.local_owner_va is not None and pair.local_owner_va != retail_va:
            continue
        if pair.mac_symbol in named:
            raise ObjectError(f"ambiguous source-owned Mac data symbol {pair.mac_symbol}")
        target = Address(pair.mac_section, pair.mac_offset)
        payload = verified(pair.mac_section, pair.mac_offset, pair.mac_size, pair.sha256,
                           declaration_only=pair.declaration_only,
                           literal=pair.local_owner_va is not None or pair.read_only)
        if pair.same_tu_definition and (target.section != toc.section
                                        or payload != bytes(pair.mac_size)):
            raise ObjectError("reviewed same-TU UDATA target must be zero-filled TOC storage")
        if pair.local_owner_va is not None:
            # CodeWarrior appends a TU-local counter to function statics. The
            # source DATA and owner VA establish identity; the emitted payload
            # must still match exactly. The counter itself proves nothing.
            names = {name for _, name in references
                     if re.fullmatch(re.escape(pair.mac_symbol) + r'\$[0-9]+', name)}
        else:
            names = {pair.mac_symbol}
        for name in names:
            if name in named:
                raise ObjectError(f"ambiguous source-owned Mac data symbol {name}")
            named[name] = (target, payload, pair.declaration_only,
                           pair.same_tu_definition, pair.owner_unit,
                           pair.same_tu_array or pair.same_tu_external)
    # MSL header data can be referenced by a source call without a game-owned
    # DATA annotation. Keep that inventory separate from authored globals and
    # require a pinned payload plus the loader-proven TOC destination.
    runtime_path = root / "config/mac/runtime.toml"
    runtime_rows = tomllib.loads(runtime_path.read_text()).get("data", []) if runtime_path.is_file() else []
    for row in runtime_rows:
        name = row.get("symbol")
        if (not isinstance(name, str) or not re.fullmatch(r'\S+', name)
                or not isinstance(row.get("evidence"), str) or not row["evidence"].strip()):
            raise ObjectError("unproven or invalid Mac runtime data symbol")
        if name in named:
            raise ObjectError(f"ambiguous Mac runtime data symbol {name}")
        target = Address(row["mac_section"], row["mac_offset"])
        payload = verified(row["mac_section"], row["mac_offset"], row["mac_size"],
                           row["sha256"], declaration_only=True)
        named[name] = (target, payload, True, False, None, False)
    constants = data_rows(root, "constants")
    literals = {}
    site_literals = {}
    site_uses = {}
    site_indirect = set()
    site_kinds = {}
    for row in constants:
        if "units" in row:
            scope = row["units"]
            if not isinstance(scope, list) or not scope or any(not isinstance(name, str) or not name for name in scope):
                raise ObjectError("Mac literal units must be a nonempty unit-name list")
            if unit not in scope:
                continue
        if not row["evidence"].strip():
            raise ObjectError("Mac literal has no pairing evidence")
        site_keys = ("owner_va", "candidate_offset", "target_offset")
        if any(key in row for key in site_keys):
            if not all(key in row for key in site_keys) or not isinstance(row["owner_va"], int):
                raise ObjectError("incomplete owner/site-bounded Mac literal")
            if row["owner_va"] != retail_va:
                continue
        payload = verified(row["mac_section"], row["mac_offset"], row["mac_size"], row["sha256"], literal=True)
        target = Address(row["mac_section"], row["mac_offset"])
        if any(key in row for key in site_keys):
            if (target_origin is None or target_size is None
                    or not isinstance(row["candidate_offset"], int)
                    or not isinstance(row["target_offset"], int)
                    or row["candidate_offset"] % 4 or row["target_offset"] % 4
                    or not 0 <= row["candidate_offset"] < len(code.data)
                    or not 0 <= row["target_offset"] <= target_size - 4):
                raise ObjectError("invalid owner/site-bounded Mac literal")
            candidate_site = row["candidate_offset"]
            target_site = row["target_offset"]
            refs = [(kind, name) for at, kind, name in code.xrefs if at == candidate_site]
            if (len(refs) != 1 or refs[0][0] not in ("HUNK_XREF_16BIT", "HUNK_XREF_16BIT_IL")
                    or not refs[0][1] or not refs[0][1].startswith("@")):
                raise ObjectError("site-bounded Mac literal lacks one anonymous TOC reference")
            kind, name = refs[0]
            emitted = [hunk for hunk in hunks if hunk.name == name and hunk.storage_class in ("RW", "RO", "TD")]
            if len(emitted) != 1 or emitted[0].xrefs or emitted[0].data != payload:
                raise ObjectError(f"site-bounded Mac literal {name!r} differs from target payload")
            word = int.from_bytes(pef.code(target_origin.section,
                                           target_origin.offset + target_site, 4), "big")
            displacement = (word & 0xffff) - (0x10000 if word & 0x8000 else 0)
            target_cell = Address(toc.section, toc.offset + displacement)
            if (pef.section(target_origin.section).kind != 0
                    or ((word >> 16) & 31) != 2
                    or (kind == "HUNK_XREF_16BIT"
                        and (target.section != toc.section
                             or (word >> 26) not in (14, 32, 34, 48, 50)
                             or target_cell != target))
                    or (kind == "HUNK_XREF_16BIT_IL"
                        and ((word >> 26) != 32
                             or loader.pointers.get(target_cell) != target))):
                raise ObjectError("site-bounded Mac literal target is not the reviewed TOC load")
            if name in site_literals and site_literals[name] != target:
                raise ObjectError(f"conflicting site-bounded Mac literal {name!r}")
            if name in site_kinds and site_kinds[name] != kind:
                raise ObjectError(f"mixed direct/indirect site-bounded Mac literal {name!r}")
            site_kinds[name] = kind
            if kind == "HUNK_XREF_16BIT_IL":
                site_indirect.add(name)
            if candidate_site in site_uses.setdefault(name, set()):
                raise ObjectError(f"duplicate site-bounded Mac literal use {name!r}")
            site_literals[name] = target
            site_uses[name].add(candidate_site)
        else:
            literals.setdefault(payload, set()).add(target)
    for name, reviewed_sites in site_uses.items():
        emitted_sites = {at for at, _, symbol in code.xrefs if symbol == name}
        if reviewed_sites != emitted_sites:
            raise ObjectError(f"site-bounded Mac literal {name!r} leaves unreviewed use sites")

    result = {}
    for kind, name in references:
        if name in tables:
            if kind != "HUNK_XREF_16BIT_IL":
                raise ObjectError("jump table requires an indirect TOC reference")
            result[name] = tables[name]
            continue
        indirect = kind == "HUNK_XREF_16BIT_IL"
        values = [hunk for hunk in hunks if hunk.name == name and hunk.storage_class in ("RW", "RO", "TD")]
        if any(value.data is None for value in values):
            raise ObjectError(f"TOC symbol {name!r} has a truncated MWLink data listing; payload unavailable")
        external = (name in named and named[name][2]) or name in external_vtables
        if (name in named and not named[name][2] and not named[name][3]
                and named[name][4] is not None and unit != named[name][4]
                and not values and indirect):
            # A reviewed initializer can live in its original owning TU while
            # another candidate TU refers to it through the indirect TOC.
            # If this object emits the datum, its payload is checked below.
            external = True
        if name in descriptors:
            target, code_name = descriptors[name]
            descriptor_values = [hunk for hunk in hunks if hunk.name == name and hunk.storage_class == "DS"]
            if (not indirect or external or values or len(descriptor_values) != 1
                    or descriptor_values[0].data != bytes(8)
                    or descriptor_values[0].xrefs != ((0, "HUNK_XREF_32BIT", code_name),
                                                     (4, "HUNK_XREF_32BIT", "TOC"))):
                raise ObjectError(f"emitted function descriptor differs: {name!r}")
            value = descriptor_values[0]
        elif external:
            # A reviewed vtable may be defined in this TU; its slot layout was checked.
            if not indirect or (values and name not in external_vtables):
                raise ObjectError(f"external TOC symbol {name!r} must have an IL reference and no emitted initializer")
            value = None
        elif len(values) != 1 or values[0].xrefs:
            raise ObjectError(f"TOC symbol {name!r} needs one nonrelocatable emitted data payload")
        else:
            value = values[0]
        if name in named and named[name][5]:
            # CodeWarrior gives source-owned uninitialized arrays and globals
            # with external linkage RW storage and an indirect TOC load.
            # Require exact zero payload and the original owning TU.
            expected = named[name][1]
            if (unit != named[name][4] or value is None or value.initialized
                    or value.storage_class != "RW" or not indirect
                    or len(value.data) != len(expected) or value.data != expected):
                raise ObjectError(f"same-TU indirect UDATA {name!r} lacks its reviewed zero storage")
        elif name in named and named[name][3]:
            # The authored source owns uninitialized same-TU storage. A
            # CodeWarrior UDATA hunk and a direct TOC reference are required;
            # accepting IDATA here would invent an initializer.
            if (value is None or value.initialized or value.storage_class != "TD"
                    or indirect or value.data != bytes(len(value.data))):
                raise ObjectError(f"same-TU UDATA symbol {name!r} lacks direct zero-filled storage")
        elif value is not None and not value.initialized:
            # A site-bounded anonymous zero template can be emitted as RW
            # UDATA. Its one owner, use site, indirect loader pointer, size,
            # and exact zero payload were checked above. No other UDATA is
            # admitted through the literal path.
            if (name not in site_literals or name not in site_indirect
                    or value.storage_class != "RW"
                    or value.data != bytes(len(value.data))):
                raise ObjectError(f"unreviewed UDATA symbol {name!r}")
        if name in descriptors:
            pass  # The emitted relocations and both retail loader pointers were checked above.
        elif name in named:
            target, expected, _, _, _, _ = named[name]
            if value is not None and value.data != expected:
                raise ObjectError(f"TOC data {name!r} differs from its reviewed target; data scoring is not yet supported")
        elif name in external_vtables:
            target = external_vtables[name]
        elif name and name.startswith("@"):
            if name in site_literals:
                target = site_literals[name]
            else:
                targets = literals.get(value.data, set())
                if len(targets) != 1:
                    raise ObjectError(f"anonymous TOC literal {name!r} has {len(targets)} reviewed payload matches")
                target = next(iter(targets))
        else:
            raise ObjectError(f"unreviewed named TOC data {name!r}")
        address_load = (indirect and name not in site_indirect
                        and (external or value.storage_class in ("RW", "RO"))
                        and target.section == toc.section
                        and -0x8000 <= target.offset - toc.offset < 0x8000)
        if indirect:
            cells = [hunk for hunk in hunks if hunk.name == name and hunk.storage_class == "TC"]
            if (len(cells) != 1 or cells[0].data != bytes(4)
                    or cells[0].xrefs != ((0, "HUNK_XREF_32BIT", name),)):
                raise ObjectError(f"TOC symbol {name!r} lacks its MWOB pointer cell")
        if indirect and not address_load:
            sites = [at for at, destination in loader.pointers.items()
                     if destination == target and at.section == toc.section
                     and -0x8000 <= at.offset - toc.offset < 0x8000]
            if len(sites) != 1:
                raise ObjectError(f"TOC symbol {name!r} has {len(sites)} relocated pointer cells")
            at = sites[0]
        else:
            at = target
            if at.section != toc.section:
                raise ObjectError(f"TOC literal {name!r} is outside the TOC section")
        binding = TocBinding(at.offset - toc.offset, target, indirect and not address_load,
                             address_load, external)
        if name in result and result[name] != binding:
            raise ObjectError(f"incompatible TOC uses for {name!r}")
        result[name] = binding
    return result
