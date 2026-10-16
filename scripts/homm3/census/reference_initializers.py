"""Evidence for short, compiler-generated global reference initializers."""
import struct

DIR32 = 6


def startup_bodies(functions, initializers, read):
    """Census bodies entered by a CRT slot, directly or through one jump."""
    bodies = {}
    for entry, slot in initializers:
        if slot == '-' or entry not in functions:
            continue
        bodies[entry] = functions[entry]
        if functions[entry] != 5:
            continue
        code = read(entry, 5)
        if code is None or len(code) != 5 or code[0] != 0xe9:
            continue
        target = (entry + 5 + struct.unpack_from('<i', code, 1)[0]) & 0xffffffff
        if target in functions:
            bodies[target] = functions[target]
    return bodies


def reference_initializer(name, body, relocs, *, starts, read, image_base,
                          data_names, data_definers, data_relocs,
                          absolute_sites, is_data):
    """Return one proved (body RVA, backing symbol, backing RVA), else None.

    The source-owned STATIC_CTOR must emit exactly a pointer store and return.
    Its destination is independently placed; its four-byte backing object has
    no relocations and must equal retail. Both native operands are admitted
    relocations, and the body must be entered by the CRT. No shape-only vote
    or ambiguous target is accepted.
    """
    if not name.startswith('__h3cg$') or '$static_ctor$' not in name:
        return None
    if body != b'\xc7\x05' + bytes(8) + b'\xc3':
        return None
    if set(relocs) != {2, 6} or any(kind != DIR32 for _ref, kind in relocs.values()):
        return None
    destination, backing = relocs[2][0], relocs[6][0]
    if destination == backing:
        return None
    destinations = data_names.get(destination, set())
    if len(destinations) != 1:
        return None
    destination_rva = next(iter(destinations))
    dest_definition = data_definers.get(destination)
    definition = data_definers.get(backing)
    if (dest_definition is None or dest_definition[1] != 4 or definition is None
            or definition[1] != 4 or definition[2] is None or len(definition[2]) != 4
            or data_relocs.get(backing) or not is_data(destination_rva, 4)):
        return None
    hits = []
    for rva, size in starts.items():
        if size != len(body):
            continue
        code = read(rva, size)
        if code is None or len(code) != size or code[:2] != body[:2] or code[10:] != body[10:]:
            continue
        if {offset for offset in range(size) if rva + offset in absolute_sites} != {2, 6}:
            continue
        dest, source = struct.unpack_from('<II', code, 2)
        if dest != destination_rva + image_base:
            continue
        source -= image_base
        if not is_data(source, 4) or read(source, 4) != definition[2]:
            continue
        known = data_names.get(backing, set())
        if known and known != {source}:
            continue
        if any(other != backing and source in places for other, places in data_names.items()):
            continue
        hits.append((rva, backing, source))
    return hits[0] if len(hits) == 1 else None
