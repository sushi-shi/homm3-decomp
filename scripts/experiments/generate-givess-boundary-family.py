"""Bounded release-verification and lexical-scope controls for hero::giveSS.

The retained 0x4e22d0 body is exact under an auto-inline override, but its
retail callers keep it out of line. DC hero.cpp:4628..4631 have no line rows
before the skill-array read; these are possible bounds-verification lines,
not proof of assertions. Test only the actual indexed-array precondition,
combined or split, with the DC 4637/4639 outer-else/inner-if scope. No dummy
work, runtime failure path, helper duplicate or new inline pin is generated.
"""
import argparse
import json
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
BODY = """#pragma auto_inline(off)
// Before normalization (locals): iWhichSS, iNumLevelsToGive, iOldLevel.
VA(0x004e22d0, 0x61)  // anchor-caller (SetSS, CheckLevel), dc 0xd37c0
int hero::giveSS(int whichSS, int numLevelsToGive)
{
    int oldLevel = m_skillLevel[whichSS];
    if (m_skillLevel[whichSS] > 0) {
        m_skillLevel[whichSS] += numLevelsToGive;
    } else if (m_skillCount < 8) {
        m_skillLevel[whichSS] = numLevelsToGive;
        m_skillOrder[whichSS] = m_skillCount + 1;
        m_skillCount++;
    }
    if (m_skillLevel[whichSS] > 3)
        m_skillLevel[whichSS] = 3;
    return m_skillLevel[whichSS] - oldLevel;
}
#pragma auto_inline(on)"""
FLAT_ELSE = """    } else if (m_skillCount < 8) {
        m_skillLevel[whichSS] = numLevelsToGive;
        m_skillOrder[whichSS] = m_skillCount + 1;
        m_skillCount++;
    }"""
NESTED_ELSE = """    } else {
        if (m_skillCount < 8) {
            m_skillLevel[whichSS] = numLevelsToGive;
            m_skillOrder[whichSS] = m_skillCount + 1;
            m_skillCount++;
        }
    }"""


def make_manifest():
    unpinned = BODY.replace("#pragma auto_inline(off)\n", "").replace(
        "\n#pragma auto_inline(on)", "")
    limit = "sizeof(m_skillLevel) / sizeof(m_skillLevel[0])"
    adopted = unpinned.replace(FLAT_ELSE, NESTED_ELSE).replace(
        "    int oldLevel =", "    HOMM3_RELEASE_VERIFY(whichSS >= 0\n"
        "        && whichSS < " + limit + ");\n    int oldLevel =", 1)
    source = (ROOT / "src/hero.cpp").read_text()
    if source.count(BODY) == 1:
        anchor = BODY
    elif source.count(adopted) == 1:
        anchor = adopted
    else:
        raise ValueError("Review changed GiveSS body before generating controls")
    verifications = (
        ("no-verification", ""),
        ("combined-bounds", "    HOMM3_RELEASE_VERIFY(whichSS >= 0 && whichSS < " + limit + ");\n"),
        ("split-bounds", "    HOMM3_RELEASE_VERIFY(whichSS >= 0);\n"
         "    HOMM3_RELEASE_VERIFY(whichSS < " + limit + ");\n"),
    )
    options = [dict(name="unchanged")]
    for scope, nested in (("flat-else", False), ("nested-else", True)):
        for name, check in verifications:
            replacement = unpinned.replace(FLAT_ELSE, NESTED_ELSE) if nested else unpinned
            if check:
                check = ("    // Probe: giveSS called by setSS/checkLevel; DC 4628..4631 gap\n"
                         "    // permits, but does not prove, this indexed-array invariant.\n"
                         "    // The no-verification and flattened options are negative controls.\n" + check)
            replacement = replacement.replace("    int oldLevel =", check + "    int oldLevel =", 1)
            options.append(dict(name=scope + "-" + name, replace=replacement))
    return dict(schema=1, source="src/hero.cpp", units=["hero"], evidence=__doc__,
                axes=[dict(name="giveSS-boundary", find=anchor, options=options)])


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(make_manifest(), indent=2) + "\n")
