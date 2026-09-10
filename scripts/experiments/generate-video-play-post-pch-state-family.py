#!/usr/bin/env python3
"""Finite compatible smackmgr headers after the terrain PCH include."""
import argparse
import json
from pathlib import Path

from homm3.vc6.tu_state_sweep import _project_header_pool, make_variants


p = argparse.ArgumentParser(description=__doc__)
p.add_argument("output", type=Path)
p.add_argument("--states", type=int, default=240)
p.add_argument("--seed", type=int, default=0x5972D1)
args = p.parse_args()

root = Path(__file__).resolve().parents[2]
source = (root / "src/smackmgr.cpp").read_text()
anchor = '#include "textresource.h"\n'
assert source.count(anchor) == 1

pool = _project_header_pool(source, "smackmgr")
variants = make_variants(args.states, args.seed, "smackmgr", pool)
options = [{"name": "control"}]
for variant in variants:
    options.append({
        "name": "post-pch-%04d-%s" % (variant.trial, variant.tag.rsplit("-", 1)[-1]),
        "replace": anchor + variant.body,
    })

args.output.write_text(json.dumps({
    "schema": 1,
    "unit": "smackmgr",
    "function": "?videoPlay@@YIHHHHHH@Z",
    "axes": [{
        "name": "post-pch-header-state",
        "find": anchor,
        "options": options,
    }],
    "evidence": [
        "Dreamcast proves the source module and shows a declaration/type stream that differs from the reconstruction, while the Complete videoPlay body is platform-only and has no Dreamcast locals.",
        "why-reg identifies identical schedule and definition slots with a different C1 pseudo processing order. The matching guide recognizes real TU/PCH state as the remaining natural compiler-state source.",
        "The earlier 121-state family inserted absent headers before terrain.h. This family inserts compatibility-filtered project headers after the current final include, the normal boundary where original TU-specific headers enter after the PCH.",
        "%d deterministic states plus the control. Every retained smackmgr row is scored; a winner must reproduce and be minimized to justified real includes before adoption." % args.states,
    ],
}, indent=2) + "\n")
