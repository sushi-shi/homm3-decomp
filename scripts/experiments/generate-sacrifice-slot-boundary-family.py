"""Restore updateSlot's two positive Dreamcast helper boundaries.

DC 0x125b3c, sacrifice_window.cpp:842, obtains its by-value artifact snapshot
through hero::get_artifact. Lines 847 and 855 call the same ordinary
update_artifact_widget defined at :800 (DC 0x125a4c). The retained retail
helper is 0x5639e0; updateSlot 0x562840 expands both source calls. The current
first copy is pasted into the caller and contains a setVisible fence.

Cross the existing/real artifact accessor with three first-call states:
untouched pasted-and-fenced control, pasted-unfenced negative control, and
the canonical ordinary helper call with no fence. The latter retains one
real helper body and its proven source order. Do not add inline, change the
public signature, invent assertions, or move the old fence into the helper.
The small six-state family exhausts the evidence-backed alternatives.
"""

import argparse
import json
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]


def make_manifest():
    source = (ROOT / "src/sacrifice_window.cpp").read_text()
    start = source.index("void type_sacrifice_window::updateSlot(long slot)\n{")
    end = source.index("\n}", start) + 2
    body = source[start:end]
    copied_start = body.index("        // Before normalization (locals): slot_widget.")
    copied_end = body.index("\n\n        m_slotWidgets[slot]->setVisible(1);", copied_start)
    pasted = body[copied_start:copied_end]
    if pasted.count("#pragma inline_depth(0)") != 1:
        raise ValueError("Review the pasted helper and fence parent")
    snapshot = "    type_artifact artifact = m_currentHero->m_equipped[slot];"
    if source.count(pasted) != 1 or body.count(snapshot) != 1:
        raise ValueError("Review updateSlot's source anchors")
    unfenced = pasted.replace("#pragma inline_depth(0)\n", "").replace("\n#pragma inline_depth()", "")
    return dict(schema=1, source="src/sacrifice_window.cpp", units=["sacrifice_window"],
                evidence=__doc__, axes=[
        dict(name="artifact-snapshot-boundary", find=snapshot, options=[
            dict(name="direct-field-control"),
            dict(name="canonical-getArtifact", replace="    type_artifact artifact = m_currentHero->getArtifact(slot);")]),
        dict(name="first-widget-update-boundary", find=pasted, options=[
            dict(name="pasted-fenced-control"),
            dict(name="pasted-unfenced-negative", replace=unfenced),
            dict(name="canonical-helper-unfenced", replace="        updateArtifactWidget(m_slotBackWidgets[slot], artifact);")]),
    ])


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(make_manifest(), indent=2) + "\n")
