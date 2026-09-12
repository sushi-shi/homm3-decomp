"""Validate sound service order, receiver identity and post-Miles state reads."""
import importlib.util
from pathlib import Path
import re
import shutil
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[3]


class SoundServiceRecoveryTests(unittest.TestCase):
    @unittest.skipUnless(shutil.which("g++"), "requires native C++ compiler")
    def test_service_family_and_current_body(self):
        spec = importlib.util.spec_from_file_location(
            "sound_service_family", ROOT / "scripts/experiments/generate-sound-service-recovery-family.py")
        family = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(family)
        source = (ROOT / "include/soundmgr.h").read_text()
        start = source.index(family.SIGNATURE + "\n{")
        actual = source[start:source.index("\n}", start) + 2]
        variants = [("Actual", actual, True)]
        variants.extend((f"Family{index}", body, True) for index, (_, body) in enumerate(family.variants()))
        captured = next(body for name, body in family.variants() if name == "section-0-stream-1-guard-0")
        variants += [
            ("WrongSection", actual.replace("&m_sectionSoundCall", "&g_soundManager->m_sectionSoundCall"), False),
            ("WrongManager", actual.replace("g_soundManager->m_mp3Playing", "m_mp3Playing"), False),
            ("WrongFill", re.sub(r"AIL_service_stream\((g_mp3Stream|stream), 1\)", r"AIL_service_stream(\1, 0)", actual), False),
            ("MissingUnlock", actual.replace("LeaveCriticalSection(&m_sectionSoundCall);", "(void)0;"), False),
            ("EarlyStream", captured.replace("    AIL_serve();\n    void* stream = g_mp3Stream;", "    void* stream = g_mp3Stream;\n    AIL_serve();"), False),
            ("WrongSleep", actual.replace("Sleep(1)", "Sleep(0)"), False),
        ]
        fixture = r"""
struct CRITICAL_SECTION { int identity; };
struct soundManager {
    CRITICAL_SECTION m_sectionSoundCall;
    unsigned char m_mp3Playing;
    void serviceSounds();
};
std::vector<int> effects;
soundManager* g_soundManager;
void* g_mp3Stream;
unsigned char g_shutDownDone;
void* streamAfterServe;
void EnterCriticalSection(CRITICAL_SECTION* section) { effects.push_back(100 + section->identity); }
void LeaveCriticalSection(CRITICAL_SECTION* section) { effects.push_back(200 + section->identity); }
void AIL_serve() { effects.push_back(300); g_mp3Stream = streamAfterServe; }
void AIL_service_stream(void* stream, int fill) {
    effects.push_back(stream == streamAfterServe ? 400 : 401);
    effects.push_back(500 + fill);
}
void Sleep(int count) { effects.push_back(600 + count); }
// @BODY@
bool check() {
    int storage[2];
    for (int before = 0; before != 2; ++before)
    for (int after = 0; after != 2; ++after)
    for (int playing = 0; playing != 3; ++playing)
    for (int shutdown = 0; shutdown != 3; ++shutdown)
    for (int same = 0; same != 2; ++same) {
        soundManager receiver, global;
        receiver.m_sectionSoundCall.identity = 7;
        global.m_sectionSoundCall.identity = 9;
        receiver.m_mp3Playing = !playing;
        global.m_mp3Playing = playing;
        g_soundManager = same ? &receiver : &global;
        if (same) receiver.m_mp3Playing = playing;
        g_mp3Stream = before ? storage : 0;
        streamAfterServe = after ? storage + 1 : 0;
        g_shutDownDone = shutdown;
        effects.clear();
        receiver.serviceSounds();
        std::vector<int> expected;
        expected.push_back(107);
        expected.push_back(300);
        if (after && playing && !shutdown) {
            expected.push_back(400);
            expected.push_back(501);
        }
        expected.push_back(601);
        expected.push_back(207);
        if (effects != expected) return false;
    }
    return true;
}
"""
        programs, checks = [], []
        for name, body, valid in variants:
            if not valid:
                self.assertNotEqual(body, actual, name)
            programs.append("namespace " + name + " {\n" + fixture.replace("// @BODY@", body) + "\n}")
            checks.append("if (" + name + "::check() != " + str(valid).lower()
                          + ') { std::fputs("' + name + '\\n", stderr); return 1; }')
        program = "#include <vector>\n#include <cstdio>\n" + "\n".join(programs)
        program += "\nint main() {\n" + "\n".join(checks) + "\n}\n"
        with tempfile.TemporaryDirectory(prefix="homm3-sound-service-") as directory:
            cpp, executable = Path(directory) / "oracle.cpp", Path(directory) / "oracle"
            cpp.write_text(program)
            proc = subprocess.run(["g++", "-std=c++98", "-O1", str(cpp), "-o", str(executable)],
                                  capture_output=True, text=True, timeout=180)
            self.assertEqual(proc.returncode, 0, proc.stderr[-6000:])
            proc = subprocess.run([str(executable)], capture_output=True, text=True, timeout=30)
            self.assertEqual(proc.returncode, 0, proc.stdout + proc.stderr)


if __name__ == "__main__":
    unittest.main()
