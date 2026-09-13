#!/usr/bin/env python3
"""Cross the real clamp, vector and loop-counter lifetimes in insetIslandZone.

Retail 0x53d1c0 keeps the reverse counter in its stack home across length(),
and uses separate clamp temporaries in the initial point and loop point.
The current source reuses one length/vector pair and names a mutable long
between max and min. Test nested versus sequential clamp evaluation, real
vector-result boundaries, and shared versus independent iteration locals.
All forms preserve positive-length gating, multiply-before-divide arithmetic,
reverse edge order, canonical operators and the final fillIslandInterior call.
No Dreamcast RMG counterpart supplies missing local or scope declarations.
"""
import argparse
import itertools
import json
from pathlib import Path

from homm3.core.common import HOMM3_DIR
from homm3.vc6.source_families import load_manifest


def transform(original, clamp, lifetime, counter):
    body = original
    if counter:
        body = body.replace("    int count = zone->m_boundary.size();", "    unsigned int count = zone->m_boundary.size();")
    if lifetime in ("loop_locals", "scoped_locals", "scoped_const"):
        body = body.replace("        delta = TRmgVector(center.m_x - point.m_x, center.m_y - point.m_y);",
                            "        TRmgVector delta(center.m_x - point.m_x, center.m_y - point.m_y);")
        body = body.replace("        length = delta.length();", "        int length = delta.length();")
    elif lifetime == "loop_length":
        body = body.replace("        length = delta.length();", "        int length = delta.length();")
    if lifetime in ("scoped_locals", "scoped_const"):
        first = body.index("    TRmgVector delta(")
        end = body.index("    while (count--)", first)
        body = body[:first] + "    {\n" + "".join("    " + line + "\n" for line in body[first:end].splitlines()) + "    }\n" + body[end:]
    if lifetime == "scoped_const":
        body = body.replace("int length = delta.length();", "const int length = delta.length();")
    # Each occurrence is a genuine first/loop clamp; handle its actual indent.
    for indent in ("        ", "            "):
        old = "\n".join(indent + line for line in (
            "long displacement = std::_cpp_max<long>(4, length / 4);",
            "displacement = std::_cpp_min<long>(displacement, length / 2);",
            "delta = delta * displacement / length;"))
        if clamp == "sequential_long":
            continue
        nested = "std::_cpp_min<long>(std::_cpp_max<long>(4, length / 4), length / 2)"
        forms = {
            "sequential_int": ["int displacement = std::_cpp_max<long>(4, length / 4);",
                               "displacement = std::_cpp_min<long>(displacement, length / 2);",
                               "delta = delta * displacement / length;"],
            "nested_long": ["long displacement = " + nested + ";", "delta = delta * displacement / length;"],
            "nested_int": ["int displacement = " + nested + ";", "delta = delta * displacement / length;"],
            "nested_expression": ["delta = delta * " + nested + " / length;"],
            "separate_vector_results": ["long displacement = std::_cpp_max<long>(4, length / 4);",
                                        "displacement = std::_cpp_min<long>(displacement, length / 2);",
                                        "delta = delta * displacement;", "delta = delta / length;"]}
        body = body.replace(old, "\n".join(indent + line for line in forms[clamp]))
    return body


def forms(original):
    clamps = ("sequential_long", "sequential_int", "nested_long", "nested_int",
              "nested_expression", "separate_vector_results")
    lifetimes = ("shared", "loop_locals", "scoped_locals", "loop_length", "scoped_const")
    for clamp, lifetime, counter in itertools.product(clamps, lifetimes, (False, True)):
        yield clamp + "+" + lifetime + ("+unsigned" if counter else "+signed"), transform(original, clamp, lifetime, counter)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    source = (HOMM3_DIR / "src/rmg.cpp").read_text()
    start = source.index("void type_random_map_generator::insetIslandZone(")
    original = source[start:source.index("\n}", start) + 2]
    options = [{"name": name, "replace": body} for name, body in forms(original)]
    assert options[0]["replace"] == original
    assert len({option["replace"] for option in options}) == 60
    payload = {"schema": 1, "source": "src/rmg.cpp", "units": ["rmg"],
               "evidence": __doc__, "axes": [{"name": "inset_lifetimes", "find": original, "options": options}]}
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(payload, indent=2) + "\n")
    load_manifest(args.output, HOMM3_DIR)
    print("60 island-inset lifetime states")


if __name__ == "__main__":
    main()
