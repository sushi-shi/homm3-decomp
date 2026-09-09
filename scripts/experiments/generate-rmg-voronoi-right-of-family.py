#!/usr/bin/env python3
"""Ordinary edge-side predicate shared by lookup and site legalization.

Retail locate 0x5fd6b0 has the same thirteen blocks and no retained calls as
the candidate. Its first orientation uses two fewer instructions and an
eight-byte rather than sixteen-byte frame. The later two orientations agree.
The published Graphics Gems IV delaunay/quadedge.C RightOf predicate supplies
a hypothesis for a shared point/edge boundary, not proof of HoMM3 source.
No Dreamcast TRmgVoronoi procedure was found. Preserve the exact ordinary
getRmgPointOrientation helper, and never paste its arithmetic into callers.

Cross cyclic orientation argument order, actual endpoint bindings, by-value
versus const-reference query ownership, and int versus byte predicate result.
Use the same ordinary file-local helper for all three lookup tests and the
legalization test. All choices preserve the known non-overflowing map domain.
"""
import argparse
import itertools
import json
from pathlib import Path

from homm3.core.common import HOMM3_DIR
from homm3.vc6 import source_families
from homm3.vc6.test_rmg_families import generator

SOURCE = "src/rmg_support.cpp"
NAME = "isRmgPointRightOfEdge"


def predicate(order, binding, reference, byte):
    origin, destination = "edge->m_sitePosition", "edge->m_twin->m_sitePosition"
    lines = []
    if binding in (1, 2):
        qualifier = "TPoint" if binding == 1 else "const TPoint&"
        lines += [qualifier + " origin = " + origin + ";",
                  qualifier + " destination = " + destination + ";"]
        origin, destination = "origin", "destination"
    elif binding == 3:
        lines += ["TRmgBoundaryVertex* twin = edge->m_twin;"]
        destination = "twin->m_sitePosition"
    elif binding == 4:
        lines += ["TPoint destination = " + destination + ";",
                  "TPoint origin = " + origin + ";"]
        origin, destination = "origin", "destination"
    arguments = [origin, "point", destination]
    arguments = arguments[order:] + arguments[:order]
    lines += ["return getRmgPointOrientation(" + ", ".join(arguments) + ") > 0;"]
    point_type = "const TPoint&" if reference else "TPoint"
    result_type = "unsigned char" if byte else "int"
    return ("static " + result_type + " " + NAME + "(" + point_type
            + " point, TRmgBoundaryVertex* edge)\n{\n"
            + "\n".join("    " + line for line in lines) + "\n}")


def variants(source):
    helper = generator("generate-rmg-position-family.py")
    locate = helper.definition(source, "TRmgVoronoi::locate")
    insertion = helper.definition(source, "TRmgVoronoi::addSite")
    adopted = NAME + "(" in locate
    retained = helper.definition(source, NAME) if adopted else None
    if adopted and retained != predicate(0, 3, 0, 0):
        raise ValueError("review changed shared edge-side predicate")
    old_site_call = ("getRmgPointOrientation(edge->m_sitePosition,\n"
                     "                previous->m_twin->m_sitePosition, edge->m_twin->m_sitePosition) > 0")
    new_site_call = NAME + "(previous->m_twin->m_sitePosition, edge)"
    direct = locate
    if adopted:
        for receiver, comparison in (("edge", ">"), ("next", "<="), ("previous", ">")):
            call = ("!" if comparison == "<=" else "") + NAME + "(point, " + receiver + ")"
            replacement = ("getRmgPointOrientation(" + receiver + "->m_sitePosition, point, "
                           + receiver + "->m_twin->m_sitePosition) " + comparison + " 0")
            if direct.count(call) != 1:
                raise ValueError("review adopted lookup call")
            direct = direct.replace(call, replacement)
        if insertion.count(new_site_call) != 1:
            raise ValueError("review adopted legalization call")
        direct_site = insertion.replace(new_site_call, old_site_call)
    else:
        direct_site = insertion
    changed = direct
    for receiver, comparison in (("edge", ">"), ("next", "<="), ("previous", ">")):
        old = ("getRmgPointOrientation(" + receiver + "->m_sitePosition, point, "
               + receiver + "->m_twin->m_sitePosition) " + comparison + " 0")
        if changed.count(old) != 1:
            raise ValueError("review changed lookup orientation")
        changed = changed.replace(old, ("!" if comparison == "<=" else "")
                                  + NAME + "(point, " + receiver + ")")
    if direct_site.count(old_site_call) != 1:
        raise ValueError("review changed site legalization predicate")
    site = direct_site.replace(old_site_call, new_site_call)
    yield dict(name="original", replace=locate)
    if adopted:
        yield dict(name="direct_orientation_control", replace=direct,
                   extra_edits=[dict(source=SOURCE, find=insertion, replace=direct_site),
                                dict(source=SOURCE, find=retained, replace="")])
    for choice in itertools.product(range(3), range(5), range(2), range(2)):
        if adopted and choice == (0, 3, 0, 0):
            continue  # The unchanged first option already contains this child.
        order, binding, reference, byte = choice
        edits = [dict(source=SOURCE, find=insertion, replace=site)]
        if adopted:
            edits.append(dict(source=SOURCE, find=retained, replace=predicate(*choice)))
        else:
            # Keep the retail VA annotation attached to locate, not the new
            # unclaimed helper, even in an isolated experimental snapshot.
            edits.append(dict(source=SOURCE,
                              insert_before="// The zone-building callers pass an eight-byte TPoint and receive an edge.",
                              text=predicate(*choice) + "\n\n"))
        yield dict(name=f"order_{order}+binding_{binding}+reference_{reference}+byte_{byte}",
                   replace=changed, extra_edits=edits)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    source = (HOMM3_DIR / SOURCE).read_text()
    helper = generator("generate-rmg-position-family.py")
    options = list(variants(source))
    payload = dict(schema=1, units=["rmg_support"], evidence=__doc__, axes=[dict(
        name="voronoi_right_of_boundary", source=SOURCE,
        find=helper.definition(source, "TRmgVoronoi::locate"), options=options)])
    args.output.write_text(json.dumps(payload, indent=2) + "\n")
    source_families.load_manifest(args.output, HOMM3_DIR)
    print(len(options), "shared edge-side states")


if __name__ == "__main__":
    main()
