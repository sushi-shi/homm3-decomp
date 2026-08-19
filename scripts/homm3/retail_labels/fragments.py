"""homm3.retail_labels.fragments - extraction's per-TU cache, parse-only.

build/gen/claims/<unit>.tsv is extraction's CACHE of the source macros (the
macros in src/ are the storage; `homm3 labels` rewrites the cache, `homm3
delink` refreshes all of it before the model runs). Same Claim shape as the
provider channels, with extraction's extras in meta:

    raw    the pre-join declarator-derived working name - the model's
           scan-order dedup replays over RAW names, exactly as the pre-port
           monolith did (Claim.name is the post-join spelling);
    dtor   the declarator carried a tilde (the ctor/dtor discriminator);
    ckind  VA_COMPGEN kind (STATIC_INIT_DISPATCH, SCALAR_DELETING_DTOR, ...);
    owner  VA_COMPGEN owner.

Row order inside a fragment is SCAN order (line order), and units read in
sorted-stem order reproduce the monolith's sorted src/*.c* sweep - the
scan-order name dedup depends on both.
"""

from __future__ import annotations

from pathlib import Path

from homm3.core import common
from homm3.core.tsv import read as read_tsv
from homm3.retail_labels import Claim

FRAGMENTS = common.HOMM3_DIR / "build/gen/claims"

HEADER = ["rva", "size", "name", "kind", "channel", "raw", "dtor",
          "ckind", "owner"]


def fragment_path(unit: str) -> Path:
    return FRAGMENTS / f"{unit}.tsv"


def unit_claims(unit: str) -> list[Claim]:
    path = fragment_path(unit)
    if not path.is_file():
        return []
    _b, _h, raw = read_tsv(path)
    out = []
    for r in raw:
        size = int(r["size"], 16) if r["size"].strip() else None
        meta = {"raw": r["raw"], "dtor": r["dtor"] == "1",
                "ckind": r["ckind"], "owner": r["owner"]}
        out.append(Claim(int(r["rva"], 16), r["name"], r["kind"],
                         r["channel"], size, unit, meta))
    return out


def all_claims() -> list[Claim]:
    """Every fragment's claims, units in sorted-stem order, rows in scan
    order - the exact order the model's dedup replay requires."""
    out: list[Claim] = []
    if FRAGMENTS.is_dir():
        for path in sorted(FRAGMENTS.glob("*.tsv")):
            out.extend(unit_claims(path.stem))
    return out
