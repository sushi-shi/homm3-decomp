"""Test real nullable-sprite bindings at CAnimatedDlg's ordinary destructor.

The fixed parent recovers the positively compiler-generated CTextDialog
destructor (DC LF_ONEMETHOD compgenx), so its empty layer expands naturally.
DC 0x11d250 and retail 0x554ab0 prove the nullable sprite cleanup followed by
base teardown. Complete calls CSprite::dispose virtually; the older DC static
resource-manager interface is not an interchangeable retail boundary.

These alternatives bind the one actually consumed sprite value by pointer,
const pointer, reference, or condition-local lifetime. They keep the null guard,
virtual call, ordinary destructor declaration and compiler-owned base cleanup.
No reference spelling is claimed recovered from the DC local inventory.
Cross each with removal of the existing fence, checking all remote callers.
"""

import argparse
import json
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]


def make_manifest():
    source = (ROOT / "src/remote.cpp").read_text()
    body = """CAnimatedDlg::~CAnimatedDlg()
{
    if (m_sprite)
        m_sprite->dispose();
}"""
    if source.count(body) != 1:
        raise ValueError("Review the nullable sprite destructor parent")
    prefix = "#pragma auto_inline(off)\nVA(0x00554ab0, 0x55)"
    suffix = "#pragma auto_inline(on)\n\n// E:\\gamedcs\\remote.cpp:1553"
    for anchor in (prefix, suffix):
        if source.count(anchor) != 1:
            raise ValueError("Review the CAnimatedDlg fence boundary")
    options = [dict(name="direct-member")]
    for name, declaration in [
        ("pointer-value", "CSprite* sprite = m_sprite;"),
        ("const-pointer-value", "CSprite* const sprite = m_sprite;"),
        ("const-pointer-reference", "CSprite* const& sprite = m_sprite;"),
    ]:
        replacement = ("CAnimatedDlg::~CAnimatedDlg()\n{\n    " + declaration
                       + "\n    if (sprite)\n        sprite->dispose();\n}")
        options.append(dict(name=name, replace=replacement))
    options.extend([
        dict(name="condition-local", replace="""CAnimatedDlg::~CAnimatedDlg()
{
    if (CSprite* sprite = m_sprite)
        sprite->dispose();
}"""),
        dict(name="guarded-object-reference", replace="""CAnimatedDlg::~CAnimatedDlg()
{
    if (m_sprite) {
        CSprite& sprite = *m_sprite;
        sprite.dispose();
    }
}"""),
    ])
    return dict(schema=1, source="src/remote.cpp", units=["remote"], evidence=__doc__, axes=[
        dict(name="sprite-binding", find=body, options=options),
        dict(name="animated-dtor-fence", find=prefix, options=[
            dict(name="existing-fence"),
            dict(name="removed", replace=prefix.replace("#pragma auto_inline(off)\n", ""),
                 extra_edits=[dict(find=suffix, replace=suffix.replace("#pragma auto_inline(on)\n", ""))]),
        ]),
    ])


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(make_manifest(), indent=2) + "\n")
