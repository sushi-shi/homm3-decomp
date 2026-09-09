"""Recover the canonical typed saved-game vector helper templates.

DC game.cpp:2698/2716 attributes all point/long/university instances to the
same source bodies. The mangled signatures prove bool results and vector
references (the dossier's uchar rendering loses native bool here). Complete
substitutes TAbstractFile for the old gz handle and uses a wide save-count
slot with two-byte writes and signed-short payload lengths.

Retail Load's gate-pair arm explicitly stores zero before resize, unlike
the four point arms. Native vector<long> ownership supplies that operation;
the current point-vector union omits it. Preserve helper calls and public
resize/subscript APIs, with actual default-value and return-scope choices.
The old two save fences are deletion controls only. No fence is introduced
inside a helper, and no artificial overload or pointer adapter replaces the
union. Existing save claims move to inactive specialization declarations.
"""

import argparse
import itertools
import json
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
SOURCE = "src/game.cpp"


def span(source, first, last):
    start = source.index(first)
    return source[start:source.index(last, start)]


def make_manifest():
    source = (ROOT / SOURCE).read_text()
    forward = span(source, "// Defined at the foot of this file, where retail emits it (0x4d2ac0):",
                   "// Before normalization (function): save_object_vector.")
    load = span(source, "    int poolCount = loadLithPoolCount(saved.m_version);",
                "#pragma inline_depth(0)\n    loadObjectVector(infile, &m_creatureBanks);")
    save = span(source, "    // PINNED for the same reason the heroPoolMap bitset test above is:",
                "    // Retail CALLS ~SavedGameHeader out of line at BOTH of these exits")
    writers = span(source, "// E:\\gamedcs\\game.cpp:2716\n// The pool writer game::Save uses",
                   "// E:\\gamedcs\\game.cpp:2754, dc 0xc1f64.")
    count_helper = span(source, "// Before normalization (function): load_lith_pool_count.\n",
                        "// Canonical BlackMarkets, TownPool and generator-vector readers")
    insertion = "#if 0  // @carcass\n\n// E:\\gamedcs\\game.cpp:2774\n"
    if source.count(insertion) != 1:
        raise ValueError("Review the helper source-order anchor")

    options = [dict(name="unchanged")]
    for fill, read_return, write_return, unpin, pool_count in itertools.product(
            ("default", "named"), ("guard", "expression"),
            ("guard", "expression"), range(2), ("legacy-helper", "ternary")):
        helpers = """// E:\\gamedcs\\game.cpp:2698; original load_vector / dest_vector.
// The point, long and university instances share this source template.
template <class T>
bool loadVector(TAbstractFile* infile, std::vector<T>& destVector)
{
    short count;
    if (infile->read(&count, sizeof(count)) < sizeof(count))
        return false;
"""
        helpers += ("    destVector.resize(count);\n" if fill == "default" else
                    "    T emptyValue = T();\n    destVector.resize(count, emptyValue);\n")
        if read_return == "guard":
            helpers += """    if (infile->read(&destVector[0], count * sizeof(T)) < count * sizeof(T))
        return false;
    return true;
}

"""
        else:
            helpers += """    return infile->read(&destVector[0], count * sizeof(T)) >= count * sizeof(T);
}

"""
        helpers += """// E:\\gamedcs\\game.cpp:2716; original save_vector / src_vector.
// Complete writes two bytes of an int slot, then uses its signed-short value.
template <class T>
bool saveVector(TAbstractFile* outfile, std::vector<T>& srcVector)
{
    int count = srcVector.size();
    if (outfile->write(&count, sizeof(short)) < sizeof(short))
        return false;
"""
        if write_return == "guard":
            helpers += """    if (outfile->write(&srcVector[0], static_cast<short>(count) * sizeof(T))
        < static_cast<short>(count) * sizeof(T))
        return false;
    return true;
}

"""
        else:
            helpers += """    return outfile->write(&srcVector[0], static_cast<short>(count) * sizeof(T))
        >= static_cast<short>(count) * sizeof(T);
}

"""
        new_load = ("    int poolCount = loadLithPoolCount(saved.m_version);\n"
                    if pool_count == "legacy-helper" else
                    "    int poolCount = saved.m_version < 32 ? 3 : 8;\n")
        new_load += """    for (i = 0; i < poolCount; ++i)
        loadVector(infile, m_lithPools[i]);
    for (i = 0; i < poolCount; ++i)
        loadVector(infile, m_lithExitPools[i]);
    loadVector(infile, m_whirlpools);
    loadVector(infile, m_undergroundGateExits);
    loadVector(infile, m_undergroundGatePairs);
    loadVector(infile, m_universities);
"""
        new_save = """    for (i = 0; i < 8; ++i) {
        unsigned char lithSaved;
#pragma inline_depth(0)
        lithSaved = saveVector(outfile, m_lithPools[i]);
#pragma inline_depth()
        if (!lithSaved)
            return -1;
    }
#pragma inline_depth(0)
    for (i = 0; i < 8; ++i) {
        if (!saveVector(outfile, m_lithExitPools[i]))
            return -1;
    }
    saveVector(outfile, m_whirlpools);
    saveVector(outfile, m_undergroundGateExits);
    saveVector(outfile, m_undergroundGatePairs);
    saveVector(outfile, m_universities);
    saveObjectVector(outfile, &m_creatureBanks);
#pragma inline_depth()

"""
        if unpin:
            new_save = new_save.replace("#pragma inline_depth(0)\n", "").replace(
                "#pragma inline_depth()\n", "")
        claims = """// The retained template instances are claimed in retail address order.
// Their one active implementation appears at the DC source-order boundary.
#if 0  // @carcass -- claim-only template instances
VA(0x004d2ac0, 0x60)  // point/long ICF twin, dc 0xc1dd4 / 0xc1e58
bool saveVector(TAbstractFile* outfile, std::vector<type_point>& srcVector)
{
    // @stub
}

VA(0x004d2b20, 0x60)  // university stride and sole Save call, dc 0xc1edc
bool saveVector(TAbstractFile* outfile, std::vector<type_university>& srcVector)
{
    // @stub
}
#endif

"""
        edits = [dict(find=forward, replace=""),
                 dict(insert_before=insertion, text=helpers),
                 dict(find=save, replace=new_save),
                 dict(find=writers, replace=claims)]
        if pool_count == "ternary":
            edits.append(dict(find=count_helper, replace=""))
        options.append(dict(name=f"{fill}-read-{read_return}-write-{write_return}-unpin-{unpin}-{pool_count}",
                            replace=new_load, extra_edits=edits))
    return dict(schema=1, source=SOURCE, units=["game"], evidence=__doc__,
                axes=[dict(name="native-vector-helpers", find=load, options=options)])


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(make_manifest(), indent=2) + "\n")
