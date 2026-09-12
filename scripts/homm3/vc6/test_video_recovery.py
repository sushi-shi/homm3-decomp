"""Check video pause transitions and ordered external effects independently."""
import importlib.util
import itertools
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[3]


class VideoRecoveryTests(unittest.TestCase):
    @unittest.skipUnless(shutil.which("g++"), "requires native C++ compiler")
    def test_family_and_adopted_helpers_against_transition_oracle(self):
        spec = importlib.util.spec_from_file_location(
            "video_family", ROOT / "scripts/experiments/generate-video-recovery-family.py")
        family = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(family)
        source = (ROOT / "src/smackmgr.cpp").read_text()
        actual = tuple(family.function_body(source, signature) for signature in
                       ("void videoSoundOnOff(int on)", "void videoResume()", "void videoClose()"))
        variants = [("Actual", actual, True)]
        variants.extend((f"Family{index}", body, True) for index, body in
                        enumerate(itertools.product(*family.variants())))
        sound, resume, close = actual
        variants += [
            ("MissingSound", (sound.replace("g_soundManager->serviceSounds();", "(void)0;"), resume, close), False),
            ("WrongResumeArgument", (sound, resume.replace("_BinkPause(g_binkVideo, 0)", "_BinkPause(g_binkVideo, 1)"), close), False),
            ("MissingPauseClear", (sound, resume.replace("g_smackPaused = 0;", "(void)0;"), close), False),
            ("MissingSmackerClose", (sound, resume, close.replace("SmackManager::closeSmacker();", "(void)0;")), False),
            ("WrongCloseOrder", (sound, resume, close.replace(
                "SmackManager::closeSmacker();\n    BinkManager::closeBink();",
                "BinkManager::closeBink();\n    SmackManager::closeSmacker();")), False),
        ]
        fixture = r"""
std::vector<int> effects;
int g_smackVideo, g_smackVideo2, g_binkVideo, g_binkVideo2;
int g_videoPauseCount, g_smackPaused, g_binkPaused;
struct Sound { void serviceSounds() { effects.push_back(10); } } sound;
Sound* g_soundManager = &sound;
struct SmackManager { static void closeSmacker() { effects.push_back(20); } };
struct BinkManager { static void closeBink() { effects.push_back(30); } };
void _BinkPause(int handle, int pause) { effects.push_back(handle * 100 + pause); }
// @BODIES@
void setup(int mask, int count, int paused) {
    effects.clear();
    g_smackVideo = (mask & 1) != 0;
    g_smackVideo2 = (mask & 2) != 0;
    g_binkVideo = (mask & 4) ? 3 : 0;
    g_binkVideo2 = (mask & 8) ? 4 : 0;
    g_videoPauseCount = count;
    g_smackPaused = g_binkPaused = paused;
}
bool check() {
    for (int mask = 0; mask != 16; ++mask)
    for (int count = 0; count != 6; ++count)
    for (int paused = 0; paused != 2; ++paused)
    for (int operation = 0; operation != 3; ++operation) {
        setup(mask, count, paused);
        std::vector<int> expected;
        const bool resume = operation == 1 ? count == 1 : operation == 2 && count > 0;
        if (resume) {
            if (mask & 4) expected.push_back(300);
            if (mask & 8) expected.push_back(400);
            if (mask) expected.push_back(10);
        }
        if (operation == 0 && mask) expected.push_back(10);
        if (operation == 2) {
            expected.push_back(10);
            expected.push_back(20);
            expected.push_back(30);
        }
        const int expectedCount = operation == 2 ? 0 : count - (operation == 1 && count > 0);
        if (operation == 0) videoSoundOnOff(1);
        if (operation == 1) videoResume();
        if (operation == 2) videoClose();
        if (effects != expected || g_videoPauseCount != expectedCount
            || g_smackPaused != (resume && (mask & 3) ? 0 : paused)
            || g_binkPaused != (resume && (mask & 12) ? 0 : paused)) return false;
    }
    return true;
}
"""
        programs, checks = [], []
        for name, bodies, valid in variants:
            if not valid:
                self.assertNotEqual(bodies, actual, name)
            programs.append("namespace " + name + " {\n" + fixture.replace(
                "// @BODIES@", "\n".join(bodies)) + "\n}")
            checks.append("if (" + name + "::check() != " + str(valid).lower()
                          + ') { std::fputs("' + name + '\\n", stderr); return 1; }')
        program = "#include <vector>\n#include <cstdio>\n" + "\n".join(programs)
        program += "\nint main() {\n" + "\n".join(checks) + "\n}\n"
        with tempfile.TemporaryDirectory(prefix="homm3-video-recovery-") as directory:
            cpp, executable = Path(directory) / "oracle.cpp", Path(directory) / "oracle"
            cpp.write_text(program)
            proc = subprocess.run(["g++", "-std=c++98", "-O1", str(cpp), "-o", str(executable)],
                                  capture_output=True, text=True, timeout=180)
            self.assertEqual(proc.returncode, 0, proc.stderr[-6000:])
            proc = subprocess.run([str(executable)], capture_output=True, text=True, timeout=30)
            self.assertEqual(proc.returncode, 0, proc.stdout + proc.stderr)


if __name__ == "__main__":
    unittest.main()
