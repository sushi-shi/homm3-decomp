"""Recover serialization result locals before removing string-helper pins.

DC game::loadString (a7414), game::saveString (a750c), and
NewSMapHeader::readString (b1110) each record an int count local that receives
both IO results before their separate error tests. Their current Complete
counterparts fold those results into conditions. All three retain retail's
short/dword protocols and short-read behavior. The first two also have usable
leading gaps (2493..2495 and 2532), permitting a non-null stream verification
hypothesis. The third's boundary row is borrowed: no assertion is inferred
there. Do not introduce checks of optional or uninitialized values.

--object-save tests the same count-local recovery in NewfullMap::saveObject
(mapcell.obj:f1b1c, lines 3450/3451 through 3468/3469). Its leading boundary is
also borrowed; this family has no verification option.
"""
import argparse
import json
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
HELPERS = (
    ("loadString", "0x004bb990", "infile", "read", "short", True),
    ("saveAbstractString", "0x004bbb60", "outfile", "write", "short", True),
    ("readMapString", "0x004c6010", "infile", "read", "int", False),
)


def make_manifest():
    source = (ROOT / "src/game.cpp").read_text()
    axes = []
    for name, address, stream, operation, size_type, allow_verify in HELPERS:
        marker = "VA(" + address + ","
        if source.count(marker) != 1:
            raise ValueError("Review changed string-helper boundary: " + name)
        begin = source.index(marker)
        off = "#pragma auto_inline(off)\n"
        if source[max(0, begin - len(off)):begin] == off:
            begin -= len(off)
        closing = "    return length;\n}"
        end = source.index(closing, begin) + len(closing)
        on = "\n#pragma auto_inline(on)"
        if source[end:end + len(on)] == on:
            end += len(on)
        body = source[begin:end]
        if body.count("VA(") != 1:
            raise ValueError("Boundary contains another claim: " + name)
        unpinned = body.replace("#pragma auto_inline(off)\n", "").replace("\n#pragma auto_inline(on)", "")
        # Round-trip the adopted locals/precondition without ever synthesizing
        # a new fence. The unchanged option remains the live source control.
        unpinned = unpinned.replace("    HOMM3_RELEASE_VERIFY(" + stream + " != 0);\n", "")
        unpinned = unpinned.replace("    int count;\n", "")
        first = "    if (" + stream + "->" + operation + "(&length, sizeof(length)) < sizeof(length))"
        second = "        if (" + stream + "->" + operation + "(buffer, length) < length)"
        unpinned = unpinned.replace(
            "    count = " + stream + "->" + operation + "(&length, sizeof(length));\n"
            "    if (count < sizeof(length))", first)
        unpinned = unpinned.replace(
            "        count = " + stream + "->" + operation + "(buffer, length);\n"
            "        if (count < length)", second)
        if unpinned.count(first) != 1 or unpinned.count(second) != 1:
            raise ValueError("Review changed IO sequence: " + name)
        counted = unpinned.replace("    " + size_type + " length", "    int count;\n    " + size_type + " length", 1)
        counted = counted.replace(first,
            "    count = " + stream + "->" + operation + "(&length, sizeof(length));\n"
            "    if (count < sizeof(length))")
        counted = counted.replace(second,
            "        count = " + stream + "->" + operation + "(buffer, length);\n"
            "        if (count < length)")
        options = [dict(name="unchanged"), dict(name="remove-only", replace=unpinned),
                   dict(name="dc-count-local", replace=counted)]
        if allow_verify:
            caller = "loadSignPool/loadRumours" if operation == "read" else "saveSignPool/saveRumours"
            gap = "2493..2495" if operation == "read" else "2532"
            check = ("    // Probe: " + caller + " calls " + name + "; DC game.cpp:" + gap + "\n"
                     "    // permits a real stream precondition, not recovered ASSERT text.\n"
                     "    // Removal-only and count-only are the negative controls.\n"
                     "    HOMM3_RELEASE_VERIFY(" + stream + " != 0);\n")
            for label, variant in (("stream-precondition", unpinned),
                                    ("dc-count-and-stream-precondition", counted)):
                options.append(dict(name=label, replace=variant.replace("\n{\n", "\n{\n" + check, 1)))
        axes.append(dict(name=name + "-boundary", find=body, options=options))
    return dict(schema=1, source="src/game.cpp", units=["game"], evidence=__doc__, axes=axes)


def make_object_manifest():
    source = (ROOT / "src/mapcell.cpp").read_text()
    marker = "VA(0x00503640,"
    if source.count(marker) != 1:
        raise ValueError("Review changed saveObject boundary")
    begin = source.index(marker)
    off, on = "#pragma auto_inline(off)\n", "\n#pragma auto_inline(on)"
    if source[max(0, begin - len(off)):begin] == off:
        begin -= len(off)
    closing = "    return 0;\n}"
    end = source.index(closing, begin) + len(closing)
    if source[end:end + len(on)] == on:
        end += len(on)
    body = source[begin:end]
    plain = body.replace(off, "").replace(on, "").replace("    int count;\n", "")
    if "    char value" in plain:
        plain = plain.replace("    char value", "    unsigned char value")
    for buffer in ("value", "typeIndex"):
        statement = "outfile->write(&" + buffer + ", sizeof(" + buffer + "))"
        test = "sizeof(" + buffer + ")"
        plain = plain.replace("    count = " + statement + ";\n    if (count < " + test + ")",
                              "    if (" + statement + " < " + test + ")")
    counted = plain.replace("\n{\n", "\n{\n    int count;\n", 1)
    for buffer, expected in (("value", 3), ("typeIndex", 1)):
        statement = "outfile->write(&" + buffer + ", sizeof(" + buffer + "))"
        test = "sizeof(" + buffer + ")"
        old = "    if (" + statement + " < " + test + ")"
        if counted.count(old) != expected:
            raise ValueError("Review changed saveObject IO sequence")
        counted = counted.replace(old, "    count = " + statement + ";\n    if (count < " + test + ")")
    # DC's temporary is plain char, not unsigned char. Both encode the same
    # byte, but preserve the recorded conversion before judging C1's body.
    signed_plain = plain.replace("unsigned char value", "char value")
    signed_counted = counted.replace("unsigned char value", "char value")
    return dict(schema=1, source="src/mapcell.cpp", units=["mapcell"], evidence=__doc__,
                axes=[dict(name="save-object-boundary", find=body, options=[
                    dict(name="unchanged"), dict(name="remove-only", replace=plain),
                    dict(name="dc-count-local", replace=counted),
                    dict(name="dc-char-buffer", replace=signed_plain),
                    dict(name="dc-count-and-char-buffer", replace=signed_counted)])])


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    parser.add_argument("--selected-only", action="store_true",
                        help="Repeat all eight subsets of the selected 2/4/2 recovery")
    parser.add_argument("--object-save", action="store_true",
                        help="Test the object writer's recovered count local")
    args = parser.parse_args()
    args.output.parent.mkdir(parents=True, exist_ok=True)
    if args.object_save and args.selected_only:
        parser.error("--selected-only is for the string family")
    manifest = make_object_manifest() if args.object_save else make_manifest()
    if args.selected_only:
        for axis, choice in zip(manifest["axes"], (2, 4, 2)):
            axis["options"] = [axis["options"][0], axis["options"][choice]]
    args.output.write_text(json.dumps(manifest, indent=2) + "\n")
