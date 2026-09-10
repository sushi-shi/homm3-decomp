#!/usr/bin/env python3
"""Finite compatible precompiled-header states for retail videoPlay."""
import argparse
import json
from pathlib import Path

from homm3.vc6.tu_state_sweep import _project_header_pool, make_variants


p = argparse.ArgumentParser(description=__doc__)
p.add_argument("output", type=Path)
p.add_argument("--states", type=int, default=120)
p.add_argument("--seed", type=int, default=0x5972D0)
args = p.parse_args()

root = Path(__file__).resolve().parents[2]
source = (root / "src/smackmgr.cpp").read_text()
anchor = '#include "terrain.h"\n'
assert source.count(anchor) == 1

pool = _project_header_pool(source, "smackmgr")
variants = make_variants(args.states, args.seed, "smackmgr", pool)
options = [{"name": "control"}]
for variant in variants:
    options.append({
        "name": "pch-%04d-%s" % (variant.trial, variant.tag.rsplit("-", 1)[-1]),
        "replace": variant.body + anchor,
    })

args.output.write_text(json.dumps({
    "schema": 1,
    "unit": "smackmgr",
    "function": "?videoPlay@@YIHHHHHH@Z",
    "axes": [{
        "name": "compatible-tu-header-state",
        "find": anchor,
        "options": options,
    }],
    "evidence": [
        "Dreamcast identifies E:\\gamedcs\\smackmgr.cpp and the source signature, but its 0x14ac38 body is a platform stub. The retail Complete body remains authoritative.",
        "why-reg v2 finds identical definition slots and instruction schedule in candidate and retail, with x/vw/vh assigned to a different callee-saved permutation. Its model classifies this as a C1 front-end pseudo-processing-order state, after source owner aliases copy-propagate without moving the divergence.",
        "Use the repository's compatibility-filtered absent-header pool and deterministic five-to-ten-header blocks, matching tu_state_sweep.py. These disposable variants model the missing original PCH declaration state without editing declarations or function semantics.",
        "The source-family runner scores every retained smackmgr function. A candidate is admissible only if videoPlay reaches exact, all 28 exact siblings remain exact, and the result reproduces in a canonical build.",
        "%d deterministic header states plus the unchanged control; unsafe or macro-rewriting headers are excluded by the shared sweep policy." % args.states,
    ],
}, indent=2) + "\n")
