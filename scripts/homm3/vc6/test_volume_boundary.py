"""Portable contract checks for the actual ordinary ConvertVolume body.

The host oracle checks setting selection and arithmetic, not VC6 inlining or
hardware sound output. Test inputs avoid signed multiplication overflow.
"""

from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest


class VolumeBoundaryTests(unittest.TestCase):
    @unittest.skipUnless(shutil.which("g++"), "requires native C++ compiler")
    def test_actual_volume_body_and_negative_controls(self):
        root = Path(__file__).resolve().parents[3]
        source = (root / "src/soundmgr.cpp").read_text()
        start = source.index("int soundManager::convertVolume(int volumeValue, int volumeType)")
        body = source[start:source.index("\n}\n", start) + 3]
        header = (root / "include/soundmgr.h").read_text()
        start = header.index("enum EVolumeType {")
        enumeration = header[start:header.index("\n};", start) + 3]
        variants = [
            ("Actual", body, True),
            ("WrongSetting", body.replace("g_unk698760;", "g_unk698764;"), False),
            ("WrongRange", body.replace("setting <= 10", "setting <= 9"), False),
            ("WrongScale", body.replace("volumeValue / 10", "volumeValue / 11"), False),
            ("WrongMinimum", body.replace("result = 1;", "result = 0;"), False),
            ("WrongMaximum", body.replace("result = 127;", "result = 128;"), False),
        ]
        fixture = r"""
int g_unk698760, g_unk698764;
struct soundManager { int convertVolume(int volumeValue, int volumeType); };
// @BODY@
bool check() {
    const int types[] = {-1, 100, 101, 102};
    soundManager manager;
    for (int music = -2; music <= 12; ++music)
    for (int effects = -2; effects <= 12; ++effects)
    for (unsigned type = 0; type != sizeof(types) / sizeof(types[0]); ++type)
    for (int volume = -256; volume <= 256; ++volume) {
        g_unk698760 = music;
        g_unk698764 = effects;
        const int selected = types[type] == 101 ? music : effects;
        int expected = 0;
        if (selected > 0 && selected < 11) {
            long scaled = (selected + 1L) * volume / 10;
            expected = scaled < 1 ? 1 : scaled > 127 ? 127 : int(scaled);
        }
        if (manager.convertVolume(volume, types[type]) != expected
            || g_unk698760 != music || g_unk698764 != effects)
            return false;
    }
    return true;
}
"""
        programs, checks = [], []
        for name, candidate, expected in variants:
            if not expected:
                self.assertNotEqual(candidate, body, name)
            programs.append("namespace " + name + " {\n"
                            + fixture.replace("// @BODY@", candidate) + "\n}")
            checks.append("if (" + name + "::check() != " + str(expected).lower()
                          + ') { std::fputs("' + name + '\\n", stderr); return 1; }')
        program = ("#include <cstdio>\n" + enumeration + "\n" + "\n".join(programs)
                   + "\nint main() {\n" + "\n".join(checks) + "\n}\n")
        with tempfile.TemporaryDirectory(prefix="volume-boundary-") as directory:
            source_path = Path(directory) / "oracle.cpp"
            source_path.write_text(program)
            executable = Path(directory) / "oracle"
            for optimization in ("-O0", "-O2"):
                result = subprocess.run(["g++", "-std=c++98", optimization,
                                         str(source_path), "-o", str(executable)],
                                        capture_output=True, text=True, timeout=180)
                self.assertEqual(result.returncode, 0, result.stderr[-6000:])
                result = subprocess.run([str(executable)], capture_output=True,
                                        text=True, timeout=60)
                self.assertEqual(result.returncode, 0, result.stdout + result.stderr)


if __name__ == "__main__":
    unittest.main()
