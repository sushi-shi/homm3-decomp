#!/usr/bin/env python3
"""Test a coordinate-domain conversion at the concrete adapters' size return.

Retail 0x532790 forwards virtual map getSize, moves its hidden-result pointer
to ECX, then copies x/y with interleaved loads/stores. All prior same-type
return/copy lifetimes fail that sequence. The underlying map owns signed int
dimensions, while the painting adapter exposes unsigned grid coordinates.
The existing recovered coordinate template allows testing a signed map-size
specialization and one ordinary templated converting constructor, without
inventing a user-defined same-type copy constructor or changing grid layout.
No instruction in the map's two-load return proves dimension signedness;
this is a retail-only hypothesis, to be checked against every consumer.
The map getSize symbol changes return type in these candidates; its old
ledger name will score missing and must be checked under the new signature
before any source adoption. Other signatures, including adapter getSize,
remain stable and are scored normally.
"""
import argparse
import itertools
import json
from pathlib import Path

from homm3.core.common import HOMM3_DIR
from homm3.vc6.source_families import load_manifest


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    header = (HOMM3_DIR / "include/rmg.h").read_text()
    source = (HOMM3_DIR / "src/rmg.cpp").read_text()
    old_base = """TRmgGridPoint type_random_map::getSize()
{
    return TRmgGridPoint(m_size.m_x, m_size.m_y);
}"""
    header_edits = []
    for owner in ("TRmgMapInterface", "type_random_map"):
        start = header.index("class " + owner + " ")
        end = header.index("\n};", start) + 3
        old = header[start:end]
        new = old.replace("virtual TRmgGridPoint getSize()", "virtual TRmgGridPointT<int> getSize()")
        assert old != new
        header_edits.append({"source": "include/rmg.h", "find": old, "replace": new})
    options = [{"name": "unsigned_map_control", "replace": old_base}]
    for ctor, returned, adapter in itertools.product(("initializers", "assignments"),
            ("constructed", "named", "assigned"), ("unsigned_named", "signed_named", "direct")):
        sig = "    template<class OtherCoordinate>\n    TRmgGridPointT(const TRmgGridPointT<OtherCoordinate>& point)"
        conversion = (sig + "\n        : m_x(point.m_x), m_y(point.m_y) {}\n" if ctor == "initializers" else
                      sig + "\n    {\n        m_x = point.m_x;\n        m_y = point.m_y;\n    }\n")
        edits = header_edits + [{"source": "include/rmg.h", "insert_before": "    TRmgGridPointT(const TPoint& point);", "text": conversion}]
        if returned == "constructed":
            base_body = "    return TRmgGridPointT<int>(m_size.m_x, m_size.m_y);"
        elif returned == "named":
            base_body = "    TRmgGridPointT<int> size(m_size.m_x, m_size.m_y);\n    return size;"
        else:
            base_body = "    TRmgGridPointT<int> size;\n    size.m_x = m_size.m_x;\n    size.m_y = m_size.m_y;\n    return size;"
        for owner in ("TRmgRoadMapAdapter", "TRmgMapAdapter"):
            old = "TRmgGridPoint " + owner + "::getSize()\n{\n    TRmgGridPoint size = m_map->getSize();\n    return size;\n}"
            if adapter == "signed_named":
                new = old.replace("    TRmgGridPoint size", "    TRmgGridPointT<int> size")
            elif adapter == "direct":
                new = "TRmgGridPoint " + owner + "::getSize()\n{\n    return m_map->getSize();\n}"
            else:
                new = old
            assert source.count(old) == 1
            if old != new:
                edits = edits + [{"source": "src/rmg.cpp", "find": old, "replace": new}]
        options.append({"name": ctor + "+" + returned + "+" + adapter,
                        "replace": "TRmgGridPointT<int> type_random_map::getSize()\n{\n" + base_body + "\n}",
                        "extra_edits": edits})
    payload = {"schema": 1, "source": "src/rmg.cpp", "evidence": __doc__,
               "units": ["rmg", "rmg_support", "rmg_terrain", "tiles",
                         "singleselectionpopups", "singleselectionwindow", "scenarioinfo"],
               "axes": [{"name": "size_domain", "find": old_base, "options": options}]}
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(payload, indent=2) + "\n")
    load_manifest(args.output, HOMM3_DIR)
    print("19 map/adapter coordinate-domain states")


if __name__ == "__main__":
    main()
