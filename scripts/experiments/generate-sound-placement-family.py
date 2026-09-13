"""Compare the canonical sound-service inline member's header placement.

DC SoundMgr.h:140 (dc 0xe6ef4) proves the header member, but its WinCE
stub does not settle whether the PC definition was inside the class.
Retail 0x59a7d0 and the MemorySample/launch_sample expansions prove the
same member operations; NextBinkFrame 0x44daa0 calls the retained body.
Keep the proven inline status, exact operations and every caller unchanged.
Compare the existing out-of-class definition, the dependency moves required
by an in-class definition, and that in-class definition. Move the real Miles
API block and existing globals, without duplicate declarations or helpers.
"""
import argparse
import json
from pathlib import Path
import re
import subprocess

ROOT = Path(__file__).resolve().parents[2]
SOURCE = "include/soundmgr.h"
SIGNATURE = "inline void soundManager::serviceSounds()"


def span(source, first, following):
    start = source.index(first)
    return source[start:source.index(following, start)]


def variants(source):
    imports = span(source, "// Miles Sound System imports.",
                   "// The CRT thread spawner")
    stream = span(source, "// Retail .bss 0x69fe78:",
                  "// Retail .bss 0x6a3258:")
    manager = span(source, "// Retail .bss 0x2993c4 (DC ?gpSoundManager",
                   "// Dreamcast records this named SoundMgr.h member")
    definition = span(source, "// Dreamcast records this named SoundMgr.h member",
                      "// --- globals ---")
    declaration = """    // Before normalization (function): soundManager::service_sounds.
    void serviceSounds();        // 0x59a7d0; DC SoundMgr.h:140 (header
                                  // inline there, emitted in kb.obj)
"""
    if source.count(declaration) != 1 or definition.count(SIGNATURE) != 1:
        raise ValueError("Review the canonical sound member declaration/definition")
    moved = source
    for block in (imports, stream, manager):
        if moved.count(block) != 1:
            raise ValueError("Review sound-service dependency ownership")
        moved = moved.replace(block, "", 1)
    # An elaborated type specifier declares the same class before its pointer
    # global. There remains exactly one declaration of this existing global.
    early_manager = manager.replace("extern soundManager*", "extern class soundManager*")
    anchor = "// soundManager (baseManager base = 0x38, basemgr.h SIZE-asserted)."
    if moved.count(anchor) != 1:
        raise ValueError("Review the sound class layout anchor")
    moved = moved.replace(anchor, imports + stream + early_manager + anchor, 1)
    member = definition.replace(SIGNATURE, "inline void serviceSounds()", 1)
    member = "".join("    " + line if line.strip() else line
                     for line in member.splitlines(True))
    inside = moved.replace(definition, "", 1).replace(declaration, member, 1)
    # These alternatives only relocate the body; guards and stream capture
    # are deliberately fixed to the previously verified implementation.
    yield "dependencies-before-class", moved
    yield "definition-inside-class", inside


def make_manifest():
    source = (ROOT / SOURCE).read_text()
    units = []
    for block in subprocess.check_output(["ninja", "-t", "deps"], cwd=ROOT, text=True).split("\n\n"):
        if str(ROOT / SOURCE) in block:
            match = re.match(r"build/objdiff/base/(.+)\.obj:", block)
            if match:
                units.append(match[1])
    if not {"binkmanager", "soundmgr", "smackmgr", "singleselectionwindow"}.issubset(units):
        raise ValueError("Refresh the full build's sound-header dependency records")
    options = [{"name": "unchanged"}]
    options += [{"name": name, "replace": text} for name, text in variants(source)]
    return {"schema": 1, "source": SOURCE, "units": sorted(units), "evidence": __doc__,
            "axes": [{"name": "sound-definition-placement", "find": source, "options": options}]}


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(make_manifest(), indent=2) + "\n")
