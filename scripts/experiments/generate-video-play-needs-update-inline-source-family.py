#!/usr/bin/env python3
"""Exact videoNeedsUpdate source forms and their videoPlay inline expansion."""
import argparse
import itertools
import json
from pathlib import Path


p = argparse.ArgumentParser(description=__doc__)
p.add_argument("output", type=Path)
args = p.parse_args()

root = Path(__file__).resolve().parents[2]
source = (root / "src/smackmgr.cpp").read_text()
old = """unsigned char videoNeedsUpdate()
{
    if (g_smackVideo || g_smackVideo2)
        return g_smackDirty && !g_smackPaused;
    else if (g_binkVideo || g_binkVideo2)
        return g_binkDirty && !g_binkPaused;
    return 0;
}"""
assert source.count(old) == 1

expressions = {
    "truth-and-not": ("g_smackDirty && !g_smackPaused", "g_binkDirty && !g_binkPaused"),
    "truth-and-eq-zero": ("g_smackDirty && g_smackPaused == 0", "g_binkDirty && g_binkPaused == 0"),
    "ne-zero-and-not": ("g_smackDirty != 0 && !g_smackPaused", "g_binkDirty != 0 && !g_binkPaused"),
    "double-not-and-not": ("!!g_smackDirty && !g_smackPaused", "!!g_binkDirty && !g_binkPaused"),
}

options = []
for smack_form, bink_form, second_if in itertools.product(
        expressions, expressions, ("else-if", "if")):
    smack_expr = expressions[smack_form][0]
    bink_expr = expressions[bink_form][1]
    replacement = """unsigned char videoNeedsUpdate()
{
    if (g_smackVideo || g_smackVideo2)
        return SMACK_EXPR;
    SECOND (g_binkVideo || g_binkVideo2)
        return BINK_EXPR;
    return 0;
}""".replace("SMACK_EXPR", smack_expr).replace("BINK_EXPR", bink_expr).replace(
        "SECOND", "else if" if second_if == "else-if" else "if")
    option = {"name": "%s-%s-%s" % (smack_form, bink_form, second_if)}
    if replacement != old:
        option["replace"] = replacement
    options.append(option)

args.output.write_text(json.dumps({
    "schema": 1,
    "unit": "smackmgr",
    "function": "?videoPlay@@YIHHHHHH@Z",
    "axes": [{
        "name": "needs-update-inline-source",
        "find": old,
        "options": options,
    }],
    "evidence": [
        "videoNeedsUpdate is an ordinary retained helper defined later in smackmgr.cpp. VC6 auto-inlines its visible body into videoPlay while also emitting the exact standalone function.",
        "Retail and candidate agree on the helper calls and expanded predicate CFG. The remaining videoPlay delta is register allocation across that expansion, so source-equivalent helper IL can affect the caller even when the retained body bytes agree.",
        "Test equivalent truth, explicit comparison and fallthrough spellings for the two byte-and-int predicates. A state is admissible only if videoNeedsUpdate stays 100%, videoPlay becomes 100%, and every exact sibling stays exact.",
        "Thirty-two finite states preserve helper boundaries, reads, values, short-circuit behavior, return type, calls, and branches; no false inline keyword or compiler directive.",
    ],
}, indent=2) + "\n")
