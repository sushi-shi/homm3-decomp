"""Restore the positively compiler-generated CTextDialog destructor boundary.

Dreamcast class 0x2c52 / field list 0x2c53 marks ~CTextDialog with member
attributes 0x107 (public virtual, compgenx), as for ~CWaitForReadyPlayersDlg.
The explicit ~CAnimatedDlg and ~TDialogBox controls have 0x007. DC 0x82068
contains only the inherited TDialogBox teardown. This is positive declaration
evidence, not an inference from an empty body or absent local inventory.

Retail's CAnimatedDlg teardown calls TDialogBox directly at +0x41, whereas
the reconstructed explicit, out-of-line empty CTextDialog layer prevents that
natural expansion. Remove the false explicit declaration/body, retaining its
0x490770 body as an IMPLICIT_DTOR claim and preserving the deleting wrapper.
Cross this canonical special-member recovery with the existing/removed
CAnimatedDlg auto-inline fence. Do not create a false inline definition or
an explicit CWaitForReadyPlayersDlg destructor to route individual callers.
"""

import argparse
import json
from pathlib import Path
import tomllib

from homm3.core.cc_wrap import scan_header_deps


ROOT = Path(__file__).resolve().parents[2]


def make_manifest():
    header = ROOT / "include/dialogbox.h"
    sources = tomllib.loads((ROOT / "config/units.toml").read_text())["unit"]
    units = [unit["unit"] for unit in sources
             if str(header) in scan_header_deps(ROOT / unit["source"], ROOT / "include",
                                                ROOT / "vendor/zlib-1.1.3")]
    if not {"dialogbox", "remote"} <= set(units):
        raise ValueError("Review dialogbox.h's consumer closure")
    source = (ROOT / "src/dialogbox.cpp").read_text()
    original = """VA(0x00490770, 0x6B)  // deleting-dtor callee + inlined base dtor, dc 0x82068
CTextDialog::~CTextDialog()
{
}"""
    if source.count(original) != 1:
        raise ValueError("Review the explicit CTextDialog destructor claim")
    remote = (ROOT / "src/remote.cpp").read_text()
    start = remote.rfind("#pragma auto_inline(off)\n", 0,
                        remote.index("CAnimatedDlg::~CAnimatedDlg()"))
    end = remote.index("#pragma auto_inline(on)", start) + len("#pragma auto_inline(on)")
    fence = remote[start:end]
    return dict(schema=1, source="src/dialogbox.cpp", units=units, evidence=__doc__, axes=[
        dict(name="text-dialog-special-member", source="include/dialogbox.h",
             find="    virtual ~CTextDialog();\n", options=[
                 dict(name="explicit-control"),
                 dict(name="implicit", replace="", extra_edits=[
                     dict(source="src/dialogbox.cpp", find=original,
                          replace="VA_COMPGEN(0x00490770, 0x6B, IMPLICIT_DTOR, CTextDialog)")])]),
        dict(name="animated-dialog-fence", source="src/remote.cpp", find=fence, options=[
            dict(name="existing-fence"),
            dict(name="removed", replace=fence.replace("#pragma auto_inline(off)\n", "")
                 .replace("\n#pragma auto_inline(on)", ""))])])


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(make_manifest(), indent=2) + "\n")
