"""homm3.vc6.inline_replay - replay C2's /Ob2 budget rule over a traced root.

Pure and compiler-free. The rule, validated against every root of a full
`predict-inline --tu` trace (rmg 497/497, town 90/90 reproduce each
recorded budget, verdict and running size):

- every site list (the root's own sites, and each expanded body's) keeps a
  running budget; a site costing over 40 is refused when that budget is
  below its cost, and otherwise its cost is charged to every enclosing
  running budget;
- an expanded site's body starts with (budget - cost if over 40) divided
  by the site's remaining-sibling count, itself included;
- the root's size starts at its cb; every expanded site adds its cost, and
  a site costing over 40 adds it once more for each enclosing expansion;
- once that size reaches 35000 nothing further expands, although the
  remaining sites are still budget-tested.

Children of a site the trace refused are unknown, so a what-if that turns
a refusal into an expansion counts the site's own cost only.
"""
from __future__ import annotations

SIZE_CAP = 35000


def tree(sites: list[dict]) -> list[dict]:
    """Nest a root's sites (in trace order) by depth."""
    root: dict = {"children": []}
    stack = [(0, root)]
    for site in sites:
        node = dict(site, children=[])
        while stack[-1][0] >= site["depth"]:
            stack.pop()
        stack[-1][1]["children"].append(node)
        stack.append((site["depth"], node))
    return root["children"]


def _replay(nodes, budget, enclosing, out, state, ancestors):
    cell = [budget]
    for node in nodes:
        cost = node["cb"]
        allows = cost <= 40 or cell[0] >= cost
        expands = allows and state["size"] < SIZE_CAP
        out.append(dict(site=node, budget=cell[0], size=state["size"],
                        budget_allows=allows, expands=expands))
        if not expands:
            continue
        site_budget = cell[0]
        charge = cost if cost > 40 else 0
        for running in [cell] + enclosing:
            running[0] -= charge
        state["size"] += cost
        if cost > 40:
            for pending in ancestors:
                pending[0] += cost
        added = [0]
        _replay(node["children"], (site_budget - charge) // node["remaining"],
                [cell] + enclosing, out, state, ancestors + [added])
        state["size"] += added[0]


def replay(root: dict, *, cb: int | None = None,
           extra_sites: dict[int, int] | None = None) -> list[dict]:
    """Decisions for one traced root.

    `cb` overrides the root's estimate (its budget is clamp(2 x cb, 1000,
    35000)); `extra_sites` maps a trace index to a number of free sites
    inserted directly after that site in the same site list, which raises
    the remaining count of that site and of its earlier siblings."""
    sites = [dict(site, index=index) for index, site in enumerate(root["sites"])]
    for index, count in (extra_sites or {}).items():
        depth = sites[index]["depth"]
        for earlier in range(index, -1, -1):
            if sites[earlier]["depth"] < depth:
                break
            if sites[earlier]["depth"] == depth:
                sites[earlier]["remaining"] += count
    cb = root["cb"] if cb is None else cb
    out: list[dict] = []
    _replay(tree(sites), min(35000, max(1000, 2 * cb)), [], out,
            {"size": cb}, [])
    return out


def reproduces(root: dict) -> bool:
    """True when the replay reproduces every recorded budget test."""
    return all(row["budget"] == row["site"]["budget"]
               and row["budget_allows"] == row["site"]["budget_allows"]
               and row["size"] == row["site"]["running"]
               for row in replay(root))
