#!/usr/bin/env python3
"""RMG derived constructor: eligibility branches and table traversal ownership.

Retail 0x537b10 reads hero attribute bytes at +0x38/+0x39/+0x3a,
branches directly on eligibility, walks the disabled array with a pointer,
and walks override records backwards. Its string initialization expands
_Tidy; the candidate retains it. Test these interacting source forms without
changing canonical THeroTraits, string helpers, declarations or inline flags.
RMG has no Dreamcast counterpart. Byte views assume the pinned x86 target.
"""
import argparse
import itertools
import json
from pathlib import Path

from homm3.core.common import HOMM3_DIR
from homm3.vc6 import source_families
from homm3.vc6.test_rmg_families import generator

SOURCE = "src/rmg.cpp"
NAME = "type_random_map_generator::type_random_map_generator"


def definition(source):
    return generator("generate-rmg-position-family.py").definition(source, NAME)


def replace(body, old, new):
    if body.count(old) != 1:
        raise ValueError("review constructor anchor: " + old)
    return body.replace(old, new)


HERO = """        for (int hero = 0; hero < 156; ++hero) {
            if (static_cast<unsigned char>(g_heroTraits[hero].m_attributes >> 16)
                || (m_mapVersion >= 1
                    ? !static_cast<unsigned char>(g_heroTraits[hero].m_attributes >> 8)
                    : !static_cast<unsigned char>(g_heroTraits[hero].m_attributes)))
                m_disabledHeroes[hero] = 1;
        }"""


def hero_scan(policy, traversal):
    shifted = [f"static_cast<unsigned char>(g_heroTraits[hero].m_attributes{shift})"
               for shift in ("", " >> 8", " >> 16")]
    masked = [f"(g_heroTraits[hero].m_attributes & {mask})"
              for mask in ("0xff", "0xff00", "0xff0000")]
    viewed = [f"reinterpret_cast<const unsigned char*>(&g_heroTraits[hero].m_attributes)[{n}]"
              for n in range(3)]
    a, b, special = shifted if policy < 3 else (masked if policy == 3 else viewed)
    target = "m_disabledHeroes[hero]"
    if policy in (0, 3):
        predicate = (f"            if ({special}\n"
                     f"                || (m_mapVersion >= 1\n"
                     f"                    ? !{b}\n"
                     f"                    : !{a}))\n"
                     f"                {target} = 1;")
    elif policy == 2:
        predicate = (f"            if ({special})\n"
                     f"                {target} = 1;\n"
                     f"            else if (m_mapVersion >= 1 ? {b} : {a})\n"
                     f"                continue;\n"
                     f"            else\n"
                     f"                {target} = 1;")
    else:
        predicate = (f"            if ({special})\n"
                     f"                {target} = 1;\n"
                     f"            else if (m_mapVersion >= 1) {{\n"
                     f"                if (!{b})\n"
                     f"                    {target} = 1;\n"
                     f"            }} else if (!{a})\n"
                     f"                {target} = 1;")
    loop = "        for (int hero = 0; hero < 156; ++hero) {\n"
    if traversal == 1:
        loop = ("        unsigned char* disabled = m_disabledHeroes;\n"
                "        for (int hero = 0; hero < 156; ++hero, ++disabled) {\n")
        predicate = predicate.replace(target, "*disabled")
    elif traversal == 2:
        loop = ("        for (unsigned char* disabled = m_disabledHeroes;\n"
                "             disabled - m_disabledHeroes < 156; ++disabled) {\n"
                "            int hero = disabled - m_disabledHeroes;\n")
        predicate = predicate.replace(target, "*disabled")
    return loop + predicate + "\n        }"


def override_scan(kind, count, form):
    cap = kind.capitalize()
    index = kind + "Limit"
    array = "g_rmg" + cap + "ObjectLimitOverrides"
    target = "g_rmg" + cap + "ObjectLimits"
    if form <= 1:
        condition = index + "--" if form == 0 else "--" + index + " >= 0"
        return (f"        for (int {index} = {count}; {condition};)\n"
                f"            {target}[{array}[{index}].m_objectType]\n"
                f"                = {array}[{index}].m_limit;")
    pointer = kind + "Override"
    if form == 2:
        return (f"        const TRmgObjectLimit* {pointer} = {array} + {count};\n"
                f"        while ({pointer} != {array}) {{\n"
                f"            --{pointer};\n"
                f"            {target}[{pointer}->m_objectType] = {pointer}->m_limit;\n"
                f"        }}")
    return (f"        const TRmgObjectLimit* {pointer} = {array} + {count} - 1;\n"
            f"        for (;;) {{\n"
            f"            {target}[{pointer}->m_objectType] = {pointer}->m_limit;\n"
            f"            if ({pointer} == {array})\n"
            f"                break;\n"
            f"            --{pointer};\n"
            f"        }}")


def variants(original):
    for policy, traversal, overrides in itertools.product(range(5), range(3), range(4)):
        body = replace(original, HERO, hero_scan(policy, traversal))
        for kind, count in (("map", 30), ("zone", 24)):
            body = replace(body, override_scan(kind, count, 0), override_scan(kind, count, overrides))
        yield dict(name=f"eligibility_{policy}+heroes_{traversal}+overrides_{overrides}", replace=body)


def initialization_forms(body):
    yield "unchanged_initialization", body
    arrays = (("m_usedQuestArtifacts", 144), ("m_disabledHeroes", 156),
              ("m_objectCountByType", 232), ("m_fixedHumanPlayers", 8))
    filled = body
    for name, count in arrays:
        filled = replace(filled, f"memset({name}, 0, sizeof({name}));",
                         f"std::fill_n({name}, {count}, 0);")
    yield "standard_fills", filled
    for kind in ("hero_flags", "object_counts", "player_flags"):
        changed = body
        for name, count in arrays:
            if ((kind == "hero_flags" and name in ("m_usedQuestArtifacts", "m_disabledHeroes"))
                    or (kind == "object_counts" and name == "m_objectCountByType")
                    or (kind == "player_flags" and name == "m_fixedHumanPlayers")):
                index = name[2:] + "Index"
                changed = replace(changed, f"memset({name}, 0, sizeof({name}));",
                    f"for (int {index} = 0; {index} < {count}; ++{index})\n"
                    f"            {name}[{index}] = 0;")
        yield kind, changed


def member_forms(body):
    yield "implicit_members", body
    anchor = "        width * height + 326900, mapVersion)"
    members = ("m_templates", "m_zones", "m_objectGenerators", "m_disabledKeyTents",
               "m_roadTargets", "m_monolithsOneWay", "m_monolithsTwoWay")
    for label, added in (
            ("empty_string", ', m_templateName("")'),
            ("counted_empty_string", ", m_templateName(0, '\\0')"),
            ("counted_empty_vectors", ",\n        " + ", ".join(name + "(0)" for name in members)),
            ("explicit_default_members", ", m_templateName(),\n        " + ", ".join(name + "()" for name in members))):
        yield label, replace(body, anchor, anchor + added)


def limit_fill_forms(body):
    yield "fill_n", body
    for form in ("zone_loop", "map_loop", "both_loops", "both_ranges"):
        changed = body
        for kind in ("Zone", "Map"):
            if form == "both_ranges":
                new = f"std::fill(g_rmg{kind}ObjectLimits, g_rmg{kind}ObjectLimits + 232, 32000);"
            elif form == "both_loops" or form == kind.lower() + "_loop":
                index = kind.lower() + "ObjectType"
                new = (f"for (int {index} = 0; {index} < 232; ++{index})\n"
                       f"            g_rmg{kind}ObjectLimits[{index}] = 32000;")
            else:
                continue
            changed = replace(changed, f"std::fill_n(g_rmg{kind}ObjectLimits, 232, 32000);", new)
        yield form, changed


def follow_options(original, source, parent, family="arrays"):
    if (parent / "snapshot" / SOURCE).read_text() != source:
        raise ValueError("parent source identity changed")
    manifest = json.loads((parent / "input.json").read_text())
    report = json.loads((parent / "generation-0001.json").read_text())
    expected = list(variants(original))
    if manifest["axes"][0]["options"] != expected:
        raise ValueError("parent manifest identity changed")
    options = [dict(name="unchanged", replace=original)]
    for elite in report["elites"]:
        repeated = json.loads((parent / "candidates" / elite["id"] / "repeat" / "result.json").read_text())
        if repeated["scores"] != elite["scores"] or repeated["object_hash"] != elite["object_hash"]:
            raise ValueError("parent reproduction failed")
        body = expected[elite["choices"][0]]["replace"]
        options += [dict(name=elite["id"] + "+" + name, replace=text)
                    for name, text in {"members": member_forms, "arrays": initialization_forms,
                                       "limits": limit_fill_forms}[family](body)]
    if len(options) != 51 or len({row["replace"] for row in options}) != 51:
        raise ValueError("expected 51 initialization states")
    return options


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    parser.add_argument("--parent", type=Path)
    parser.add_argument("--members", action="store_true")
    parser.add_argument("--limits", action="store_true")
    args = parser.parse_args()
    source = (HOMM3_DIR / SOURCE).read_text()
    original = definition(source)
    family = "limits" if args.limits else ("members" if args.members else "arrays")
    options = follow_options(original, source, args.parent, family) if args.parent else list(variants(original))
    assert options[0]["replace"] == original
    assert len({row["replace"] for row in options}) == len(options)
    payload = dict(schema=1, units=["rmg"], evidence=__doc__, axes=[dict(
        name="generator_construction", source=SOURCE, find=original, options=options)])
    args.output.write_text(json.dumps(payload, indent=2) + "\n")
    source_families.load_manifest(args.output, HOMM3_DIR)
    print(len(options), "constructor states")


if __name__ == "__main__":
    main()
