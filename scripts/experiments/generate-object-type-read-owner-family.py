"""Test native ownership of the full-width object-type read, not a narrowing cast.

NewfullMap::readObjectType (retail 0x503780) reads four bytes, checks the
read result, and only then commits the type field. DC rows 3610..3614 use
the generic int_buffer; its existence and its other length/extra uses are
preserved. A separate enum local is a Complete I/O-boundary hypothesis,
not a claimed Dreamcast local. Unlike the neighbouring 16-bit readers,
this field's wire and native enum widths already agree. Never read directly
into the destination field: a short read must not partially commit it.

DC row 3619 obtains the diagnostic type from the already committed record.
Cross that canonical field query with the old union and three native-value
lifetimes. Do not introduce conversion helpers, casts, memcpy substitutes,
new directives, or changes to the file format. Score all mapcell functions.
"""

import argparse
import itertools
import json
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]


def make_manifest():
    source = (ROOT / "src/mapcell.cpp").read_text()
    start = source.index("int NewfullMap::readObjectType(TAbstractFile* infile,")
    original = source[start:source.index("\n}\n", start) + 3]
    union_start = original.index("    union {\n")
    union_end = original.index("    tempObjectType->m_objectType = convertedType.m_typed;", union_start)
    union_end += len("    tempObjectType->m_objectType = convertedType.m_typed;")
    union = original[union_start:union_end]
    read = "    if (infile->read(&value, sizeof(value)) < sizeof(value))\n        return -1;\n"
    read_start = original.rfind(read, 0, union_start)
    if read_start < 0 or original[read_start + len(read):union_start]:
        raise ValueError("Review the full-width type read")
    tail = original.index("\n    if (infile->read(&value, sizeof(value)) < sizeof(value))", union_end)
    typed_read = """    if (infile->read(&objectTypeRead, sizeof(objectTypeRead)) < sizeof(objectTypeRead))
        return -1;
    tempObjectType->m_objectType = objectTypeRead;"""
    declaration = "    TAdventureObjectType objectTypeRead;\n"
    options = []
    for lifetime, field_query in itertools.product(range(4), range(2)):
        candidate = original
        if lifetime:
            body = original[read_start:tail]
            body = body.replace(read + union, typed_read)
            body = body.replace("g_adventureObjectNames[value]", "g_adventureObjectNames[objectTypeRead]")
            if lifetime == 1:
                body = declaration + body
            elif lifetime == 3:
                body = "    {\n" + "".join("    " + line if line.strip() else line
                                             for line in (declaration + body).splitlines(keepends=True)) + "    }\n"
            candidate = original[:read_start] + body + original[tail:]
            if lifetime == 2:
                candidate = candidate.replace("    int value;\n", "    int value;\n" + declaration, 1)
        if field_query:
            old = "g_adventureObjectNames[objectTypeRead]" if lifetime else "g_adventureObjectNames[value]"
            if candidate.count(old) != 1:
                raise ValueError("Review the committed-type diagnostic")
            candidate = candidate.replace(old, "g_adventureObjectNames[tempObjectType->m_objectType]")
        option = dict(name=("union-control", "native-local", "native-top-local", "native-scoped-local")[lifetime]
                      + ("-record-diagnostic" if field_query else "-buffer-diagnostic"))
        if candidate != original:
            option["replace"] = candidate
        options.append(option)
    return dict(schema=1, source="src/mapcell.cpp", units=["mapcell"], evidence=__doc__,
                axes=[dict(name="full-width-type-owner", find=original, options=options)])


def make_phase_scope_manifest():
    """Give each independently committed wire field its own real value lifetime.

    The first family differs only in stack-slot assignments (18 displacement
    bytes for the unscoped native local); instructions, layout and relocations
    otherwise agree. Test the filename length and extra field as separate int
    values, with optional scopes ending immediately after their commits. This
    remains a source-lifetime hypothesis, not a recovered DC declaration list.
    """
    payload = make_manifest()
    axis = payload["axes"][0]
    original = axis["find"]
    length = """    if (infile->read(&value, sizeof(value)) < sizeof(value))
        return -1;
    infile->read(imageName, value);
    tempObjectType->m_imageName = imageName;"""
    extra = """    if (infile->read(&value, sizeof(value)) < sizeof(value))
        return -1;
    tempObjectType->m_extra = value;"""

    def scoped(body):
        return "    {\n" + "\n".join("    " + line for line in body.splitlines()) + "\n    }"

    options = [dict(name="unchanged-union-control")]
    for length_scope, type_scope, extra_scope in itertools.product(range(2), repeat=3):
        # Native parent always queries the already committed type for diagnostics.
        candidate = axis["options"][7 if type_scope else 3]["replace"]
        if candidate.count("    int value;\n") != 1:
            raise ValueError("Review the generic integer buffer declaration")
        candidate = candidate.replace("    int value;\n", "")
        for old, name, scope in [(length, "imageNameLength", length_scope),
                                 (extra, "objectExtra", extra_scope)]:
            if candidate.count(old) != 1:
                raise ValueError("Review the integer field's read/commit phase")
            new = "    int " + name + ";\n" + old.replace("value", name)
            candidate = candidate.replace(old, scoped(new) if scope else new)
        options.append(dict(name="native-phases-%d%d%d" % (length_scope, type_scope, extra_scope),
                            replace=candidate))
    payload["evidence"] += "\n" + make_phase_scope_manifest.__doc__
    payload["axes"] = [dict(name="field-value-lifetimes", find=original, options=options)]
    return payload


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    parser.add_argument("--phase-scopes", action="store_true")
    args = parser.parse_args()
    args.output.parent.mkdir(parents=True, exist_ok=True)
    payload = make_phase_scope_manifest() if args.phase_scopes else make_manifest()
    args.output.write_text(json.dumps(payload, indent=2) + "\n")
