# -*- coding: utf-8 -*-
"""Export the stock Function ID analyzer's verdicts from the cached project.

Ghidra's default analysis (S2) already ran the Function ID analyzer against
its shipped databases (vsOlder_x86 covers the VC6 era); its bookmarks are an
independent library-identification channel that costs nothing to read back.
Bookmark comments carry the matched symbol(s); conflicts are kept - a
conflict is evidence of library-ness even when the name is undecided.
"""
from homm3.carve import common


def _export(program):
    # locals only: pyghidra injects GhidraScript bean properties into the
    # module globals, so a global named `category` hits a read-only property
    image_base = program.getImageBase().getOffset()
    fm = program.getFunctionManager()
    rows = []
    iterator = program.getBookmarkManager().getBookmarksIterator()
    while iterator.hasNext():
        bookmark = iterator.next()
        kind = str(bookmark.getCategory())
        if kind not in ("Function ID Analyzer", "Function ID Conflict"):
            continue
        address = bookmark.getAddress()
        fn = fm.getFunctionAt(address)
        rows.append(("0x%x" % (address.getOffset() - image_base),
                     fn.getName() if fn is not None else "-",
                     kind.replace("Function ID ", ""),
                     (bookmark.getComment() or "").replace("\t", " ")))
    rows.sort(key=lambda row: int(row[0], 16))
    out = common.HOMM3_DIR / "build/dna/fid_bookmarks.tsv"
    common.write_tsv(out, "homm3.carve.ghidra.export_fid",
                     ["entry_rva", "name", "category", "evidence"], rows)
    print("[export_fid] wrote %d stock-FID records -> %s" % (len(rows), out))


_export(currentProgram)  # noqa: F821 - injected by the pyghidra harness
