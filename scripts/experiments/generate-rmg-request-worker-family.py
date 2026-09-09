#!/usr/bin/env python3
"""Request worker 0x54bf60: clamp, repair and seat-value lifetimes.

Retail retains one in EBX from the strength clamp through player repair and
human-seat setup. Both sides have fifteen blocks and the same generator
constructor/generate/write/destructor boundaries, but the current seat loop
uses a biased request pointer. Test actual clamp expressions, ordered repair
stores and seat bindings; no unrelated constant is reused as compiler padding.
There is no mapped Dreamcast counterpart. Score every rmg sibling.
"""
import argparse
import itertools
import json
from pathlib import Path

from homm3.core.common import HOMM3_DIR
from homm3.vc6.source_families import load_manifest
from homm3.vc6.test_rmg_families import generator

NAME = "TRandomMapRequest::generateToFile"


def definition(source):
    return generator("generate-rmg-position-family.py").definition(source, NAME)


def baseline(original):
    if "generator.setHumanPlayer(seat);" in original:
        original = original.replace("generator.setHumanPlayer(seat);", "generator.m_fixedHumanPlayers[seat] = 1;")
        original = original.replace("generator.setTownChoice(seat, m_townType[seat]);", "generator.m_townChoices[seat] = m_townType[seat];")
        original = original.replace("    if (strength > 5)", "    else if (strength > 5)")
    return original


def variants(original):
    current = original
    original = baseline(original)
    clamp = """    int strength = m_monsterStrength + 3;
    if (strength < 1)
        strength = 1;
    else if (strength > 5)
        strength = 5;"""
    repair = """        m_humanPlayerCount = 1;
        m_computerPlayerCount = 1;"""
    seats = """    for (int seat = 0; seat < 8; ++seat) {
        if (m_isHumanSeat[seat])
            generator.m_fixedHumanPlayers[seat] = 1;
        generator.m_townChoices[seat] = m_townType[seat];
    }"""
    for anchor in (clamp, repair, seats):
        if original.count(anchor) != 1:
            raise ValueError("review changed request-worker source")
    clamps = [clamp,
        clamp.replace("else if", "if"),
        "    int requestedStrength = m_monsterStrength + 3;\n    int strength = requestedStrength < 1 ? 1 : requestedStrength > 5 ? 5 : requestedStrength;",
        "    int strength = std::_cpp_min(5, std::_cpp_max(1, m_monsterStrength + 3));",
        "    int minimumStrength = 1;\n    int maximumStrength = 5;\n" + clamp.replace("strength < 1", "strength < minimumStrength").replace("strength = 1;", "strength = minimumStrength;").replace("strength > 5", "strength > maximumStrength").replace("strength = 5;", "strength = maximumStrength;"),
    ]
    repairs = [repair,
        "        m_computerPlayerCount = m_humanPlayerCount = 1;",
        "        m_humanPlayerCount = 1;\n        m_computerPlayerCount = m_humanPlayerCount;",
    ]
    seat_forms = [seats,
        seats.replace("        if (m_isHumanSeat[seat])", "        unsigned char human = m_isHumanSeat[seat];\n        if (human)"),
        seats.replace("        if (m_isHumanSeat[seat])", "        unsigned char& human = generator.m_fixedHumanPlayers[seat];\n        if (m_isHumanSeat[seat])").replace("generator.m_fixedHumanPlayers[seat] = 1;", "human = 1;"),
        "    const int* towns = m_townType;\n    int* choices = generator.m_townChoices;\n" + seats.replace("generator.m_townChoices[seat] = m_townType[seat];", "choices[seat] = towns[seat];"),
    ]
    for c, r, s in itertools.product(range(5), range(3), range(4)):
        body = original.replace(clamp, clamps[c]).replace(repair, repairs[r]).replace(seats, seat_forms[s])
        if (c, r, s) == (0, 0, 0) and current != original:
            yield "unchanged", current
            continue
        yield "clamp_%d+repair_%d+seats_%d" % (c,r,s), body


def flag_refinement(parent, form):
    target = "generator.m_fixedHumanPlayers[seat] = 1;"
    if target not in parent:
        target = "human = 1;"
    if parent.count(target) != 1:
        raise ValueError("review request human-seat store")
    kind = ("unsigned char", "int", "bool", "unsigned char", "int")[form]
    declaration = "    " + kind + " humanFlag = " + ("true" if kind == "bool" else "1") + ";\n"
    body = parent.replace(target, target.replace("= 1;", "= humanFlag;"))
    if form < 3:
        return body.replace("\n{\n", "\n{\n" + declaration, 1)
    marker = "    for (int seat = 0; seat < 8; ++seat) {"
    if body.count(marker) != 1:
        raise ValueError("review request seat loop")
    return body.replace(marker, declaration + marker)


def count_refinement(parent, form):
    marker = "    if (m_humanPlayerCount + m_computerPlayerCount < 2) {"
    if parent.count(marker) != 1:
        raise ValueError("review request player-count guard")
    forms = [
        "    int computerPlayers = m_computerPlayerCount;\n    if (m_humanPlayerCount + computerPlayers < 2) {",
        "    int humanPlayers = m_humanPlayerCount;\n    int computerPlayers = m_computerPlayerCount;\n    if (humanPlayers + computerPlayers < 2) {",
        "    int computerPlayers = m_computerPlayerCount;\n    int humanPlayers = m_humanPlayerCount;\n    if (humanPlayers + computerPlayers < 2) {",
        "    int playerCount = m_humanPlayerCount + m_computerPlayerCount;\n    if (playerCount < 2) {",
        "    const int& computerPlayers = m_computerPlayerCount;\n    if (m_humanPlayerCount + computerPlayers < 2) {",
    ]
    return parent.replace(marker, forms[form])


def frontier(source, checkpoint, refine=flag_refinement):
    retained = generator("generate-rmg-key-tent-family.py").parents(source, checkpoint, definition)
    current = definition(source)
    forms, seen = [("unchanged", current)], {current}
    for identity, parent in retained:
        if parent not in seen:
            seen.add(parent)
            forms.append((identity + "+parent", parent))
    for form in range(5):
        for identity, parent in retained:
            body = refine(parent, form)
            if body not in seen:
                seen.add(body)
                forms.append((identity + "+" + refine.__name__ + "_%d" % form, body))
            if len(forms) == 60:
                return forms
    return forms


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    parser.add_argument("--flags-from", type=Path)
    parser.add_argument("--counts-from", type=Path)
    args = parser.parse_args()
    source = (HOMM3_DIR / "src/rmg.cpp").read_text()
    original = definition(source)
    if args.flags_from and args.counts_from:
        parser.error("choose one refinement")
    forms = frontier(source, args.counts_from, count_refinement) if args.counts_from else frontier(source, args.flags_from) if args.flags_from else variants(original)
    payload = dict(schema=1, units=["rmg"], evidence=__doc__, axes=[
        generator("generate-rmg-position-family.py").axis("request_worker", "src/rmg.cpp", original, forms)])
    args.output.write_text(json.dumps(payload, indent=2)+"\n")
    load_manifest(args.output, HOMM3_DIR)
    print(len(payload["axes"][0]["options"]), "request-worker states")


if __name__ == "__main__":
    main()
