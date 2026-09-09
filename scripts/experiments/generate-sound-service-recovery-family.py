"""Recover sound-service inline decisions through real guards and locals.

DC SoundMgr.h:140 (0xe6ef4) proves the header-owned inline declaration;
its WinCE stub does not describe the PC implementation. Retail 0x59a7d0
locks this receiver's critical section, serves Miles, checks the post-serve
stream/global manager/shutdown state in order, optionally fills the stream,
sleeps, and unlocks. Its retained body is already exact. Preserve that API
and sequence while comparing supported local bindings and guard scopes.
"""
import argparse
import itertools
import json
from pathlib import Path
import re
import subprocess

ROOT = Path(__file__).resolve().parents[2]
SIGNATURE = "inline void soundManager::serviceSounds()"


def variants():
    bodies = []
    for section, stream, guard in itertools.product(range(3), range(2), range(5)):
        prefix = ""
        section_arg = "&m_sectionSoundCall"
        if section == 1:
            prefix = "    CRITICAL_SECTION* section = &m_sectionSoundCall;\n"
            section_arg = "section"
        elif section == 2:
            prefix = "    CRITICAL_SECTION& section = m_sectionSoundCall;\n"
            section_arg = "&section"
        prefix += f"    EnterCriticalSection({section_arg});\n    AIL_serve();\n"
        stream_name = "g_mp3Stream"
        if stream:
            prefix += "    void* stream = g_mp3Stream;\n"
            stream_name = "stream"
        first = stream_name
        second = "g_soundManager->m_mp3Playing"
        third = "!g_shutDownDone"
        call = f"AIL_service_stream({stream_name}, 1);"
        guards = [
            f"    if ({first} && {second} && {third})\n        {call}\n",
            f"    if ({first}) {{\n        if ({second}) {{\n            if ({third})\n                {call}\n        }}\n    }}\n",
            f"    if ({first} && {second}) {{\n        if ({third})\n            {call}\n    }}\n",
            f"    if ({first}) {{\n        if ({second} && {third})\n            {call}\n    }}\n",
            f"    if (!{first})\n        goto finishedService;\n    if (!{second})\n        goto finishedService;\n    if (g_shutDownDone)\n        goto finishedService;\n    {call}\nfinishedService:\n",
        ]
        text = SIGNATURE + "\n{\n" + prefix + guards[guard]
        text += f"    Sleep(1);\n    LeaveCriticalSection({section_arg});\n}}"
        bodies.append((f"section-{section}-stream-{stream}-guard-{guard}", text))
    return bodies


def make_manifest():
    source = (ROOT / "include/soundmgr.h").read_text()
    start = source.index(SIGNATURE + "\n{")
    body = source[start:source.index("\n}", start) + 2]
    choices = variants()
    if body not in [text for _, text in choices]:
        raise ValueError("Review changed sound-service implementation before searching")
    units = []
    for block in subprocess.check_output(["ninja", "-t", "deps"], cwd=ROOT, text=True).split("\n\n"):
        if str(ROOT / "include/soundmgr.h") in block:
            match = re.match(r"build/objdiff/base/(.+)\.obj:", block)
            if match:
                units.append(match[1])
    if not {"soundmgr", "smackmgr", "singleselectionwindow"}.issubset(units):
        raise ValueError("Run the full build to refresh sound-header dependency records")
    options = [{"name": "unchanged"}] + [{"name": name, "replace": text}
               for name, text in choices if text != body]
    return {"schema": 1, "source": "include/soundmgr.h", "units": sorted(units), "evidence": __doc__,
            "axes": [{"name": "service-guards-and-bindings", "find": body, "options": options}]}


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(make_manifest(), indent=2) + "\n")
