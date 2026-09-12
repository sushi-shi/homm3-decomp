"""Compare canonical video helper guards and pause-draining loops.

DC smackmgr.cpp:75/152/238 supplies the ordinary declarations and order;
its four-byte port stubs cannot describe the PC bodies. Retail 0x5971b0
shares one sound-service tail across four handles. Retail 0x597850 guards
the decrement and resumes only at zero, and 0x5975f0 drains the counter
before the sound/Smacker/Bink close chain. Every option preserves those
operations, their order and zero-count behavior, with no inline override.
"""
import argparse
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]


def function_body(source, signature):
    start = source.index(signature + "\n{")
    return source[start:source.index("\n}", start) + 2]


def variants():
    sound = [
        """void videoSoundOnOff(int on)
{
    if (g_smackVideo || g_smackVideo2)
        g_soundManager->serviceSounds();
    else if (g_binkVideo || g_binkVideo2)
        g_soundManager->serviceSounds();
}""",
        """void videoSoundOnOff(int on)
{
    if (g_smackVideo || g_smackVideo2 || g_binkVideo || g_binkVideo2)
        g_soundManager->serviceSounds();
}""",
        """void videoSoundOnOff(int on)
{
    if (!g_smackVideo && !g_smackVideo2 && !g_binkVideo && !g_binkVideo2)
        return;
    g_soundManager->serviceSounds();
}""",
    ]
    tail = """    if (g_smackVideo || g_smackVideo2)
        g_smackPaused = 0;
    if (g_binkVideo) {
        g_binkPaused = 0;
        _BinkPause(g_binkVideo, 0);
    }
    if (g_binkVideo2) {
        g_binkPaused = 0;
        _BinkPause(g_binkVideo2, 0);
    }
    videoSoundOnOff(1);
"""
    guards = [
        "    if (g_videoPauseCount == 0)\n        return;\n"
        "    if (--g_videoPauseCount != 0)\n        return;\n",
        "    if (g_videoPauseCount == 0 || --g_videoPauseCount != 0)\n        return;\n",
    ]
    resume = ["void videoResume()\n{\n" + guard + tail + "}" for guard in guards]
    resume += [
        "void videoResume()\n{\n    if (g_videoPauseCount != 0 && --g_videoPauseCount == 0) {\n"
        + "".join("    " + line + "\n" for line in tail.splitlines()) + "    }\n}",
        "void videoResume()\n{\n    if (g_videoPauseCount != 0) {\n        if (--g_videoPauseCount == 0) {\n"
        + "".join("        " + line + "\n" for line in tail.splitlines()) + "        }\n    }\n}",
    ]
    loops = [
        "    while (g_videoPauseCount != 0)\n        videoResume();",
        "    if (g_videoPauseCount != 0) {\n        do {\n            videoResume();\n"
        "        } while (g_videoPauseCount != 0);\n    }",
        "    for (;;) {\n        if (g_videoPauseCount == 0)\n            break;\n        videoResume();\n    }",
        "resumeVideo:\n    if (g_videoPauseCount != 0) {\n        videoResume();\n        goto resumeVideo;\n    }",
    ]
    close = ["void videoClose()\n{\n" + loop + "\n    g_soundManager->serviceSounds();\n"
             "    SmackManager::closeSmacker();\n    BinkManager::closeBink();\n}" for loop in loops]
    return sound, resume, close


def make_manifest():
    source = (ROOT / "src/smackmgr.cpp").read_text()
    axes = []
    for name, signature, choices in zip(
            ("sound-guard", "resume-guard", "close-loop"),
            ("void videoSoundOnOff(int on)", "void videoResume()", "void videoClose()"), variants()):
        current = function_body(source, signature)
        if current not in choices:
            raise ValueError(f"Review changed {signature} before generating a new family")
        options = [{"name": "unchanged"}]
        options.extend({"name": f"form-{index}", "replace": choice}
                       for index, choice in enumerate(choices) if choice != current)
        axes.append({"name": name, "find": current, "options": options})
    return {"schema": 1, "source": "src/smackmgr.cpp", "units": ["smackmgr"],
            "evidence": __doc__, "axes": axes}


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(make_manifest(), indent=2) + "\n")
