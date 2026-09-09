"""Exercise the actual market entry and dispatch arm with native artifact arrays."""

from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest


class MarketArtifactOwnerTests(unittest.TestCase):
    @unittest.skipUnless(shutil.which("g++"), "requires native C++ compiler")
    def test_actual_sell_setup_preserves_arguments_outputs_and_order(self):
        root = Path(__file__).resolve().parents[3]
        source = (root / "src/tradpost.cpp").read_text()
        start = source.index("void TSellArtifactWindow::setupNewTrade()")
        setup = source[start:source.index("\n}", start) + 2]
        handler = source[source.index("int TSellArtifactWindow::windowHandler(message* msg)"):]
        handler = handler[:handler.index("\n}\n")]
        self.assertNotIn("#pragma", handler)
        self.assertNotIn("computeTradeRatios(", handler)
        self.assertEqual(handler.count("setupNewTrade();"), 4)
        call = ("    computeTradeRatios(g_selectedArtifact, g_leftResource,\n"
                "        &g_giveQuantity, &g_ratioInverted, &g_maxTradeUnits);")
        variants = [
            ("Actual", setup, True),
            ("BadArtifact", setup.replace("g_selectedArtifact,", "g_leftResource,"), False),
            ("BadOutput", setup.replace("&g_ratioInverted,", "&g_giveQuantity,"), False),
            ("BadAmount", setup.replace("g_rightAmount = 1;", "g_rightAmount = 2;"), False),
            ("MissingCall", setup.replace(call, ""), False),
            ("BadOrder", setup.replace(call, "g_rightAmount = 1;\n" + call), False),
        ]
        fixture = """
int g_selectedArtifact, g_leftResource, g_giveQuantity, g_ratioInverted;
int g_maxTradeUnits, g_rightAmount, observedArtifact, observedResource;
int observedAmount, calls;
struct TSellArtifactWindow {
    void setupNewTrade();
    void computeTradeRatios(int artifact, int resource, int* ratio, int* left, int* max) {
        ++calls;
        observedArtifact = artifact;
        observedResource = resource;
        observedAmount = g_rightAmount;
        *ratio = 23;
        *left = 29;
        *max = 31;
    }
};
// @SETUP@
bool check() {
    for (int artifact = 0; artifact < 82; ++artifact)
    for (int resource = 0; resource < 7; ++resource) {
        g_selectedArtifact = artifact;
        g_leftResource = resource;
        g_giveQuantity = g_ratioInverted = g_maxTradeUnits = -77;
        g_rightAmount = 9;
        calls = 0;
        TSellArtifactWindow window;
        window.setupNewTrade();
        if (calls != 1 || observedArtifact != artifact || observedResource != resource
            || observedAmount != 9 || g_rightAmount != 1 || g_giveQuantity != 23
            || g_ratioInverted != 29 || g_maxTradeUnits != 31)
            return false;
    }
    return true;
}
"""
        program, checks = [], []
        for name, body, valid in variants:
            if not valid:
                self.assertNotEqual(body, setup, name)
            program.append("namespace " + name + " {\n"
                           + fixture.replace("// @SETUP@", body) + "\n}")
            checks.append("if (" + name + "::check() != " + str(valid).lower() + ") return 1;")
        self.compile_and_run("\n".join(program) + "\nint main() {\n"
                             + "\n".join(checks) + "\n}\n")

    @unittest.skipUnless(shutil.which("g++"), "requires native C++ compiler")
    def test_actual_entry_aliases_selected_owner_and_rejects_negative_controls(self):
        root = Path(__file__).resolve().parents[3]
        source = (root / "src/tradpost.cpp").read_text()
        start = source.index("void doBlackMarket(hero* inHero,")
        entry = source[start:source.index("\n}", start) + 2]
        self.assertIn("TArtifact* blackArtifacts", entry)
        self.assertNotIn("TMarketArtifactList", source)
        names = ["g_marketArtifacts", "g_marketHero", "g_marketCount",
                 "g_marketWindow", "g_marketSource"]
        declarations = []
        for name in names:
            line = next(line for line in source.splitlines()
                        if line.startswith("DATA(") and line.endswith(" " + name + ";"))
            declarations.append(line[line.index("static "):])
        events = (root / "src/events.cpp").read_text()
        start = events.index("    case BLACK_MARKET:")
        arm = events[start:events.index("    case BOAT:", start)]
        game = (root / "include/game.h").read_text()
        start = game.index("struct TBlackMarket {")
        record = game[start:game.index("\n};", start) + 3]
        member = next(line.strip() for line in game.splitlines()
                      if "TArtifact m_marketArtifacts[7];" in line)
        artifact = (root / "include/artifact.h").read_text()
        start = artifact.index("enum TArtifact {")
        enumeration = artifact[start:artifact.index("\n};", start) + 3]
        variants = [
            ("Actual", entry, arm, True),
            ("BadPointer", entry.replace("= blackArtifacts;", "= blackArtifacts + 1;"), arm, False),
            ("BadHero", entry.replace("= inHero;", "= 0;"), arm, False),
            ("BadCount", entry.replace("g_marketCount = 5;", "g_marketCount = 4;"), arm, False),
            ("BadWindow", entry.replace("g_marketWindow = 2;", "g_marketWindow = 0;"), arm, False),
            ("BadSource", entry.replace("g_marketSource = 2;", "g_marketSource = 0;"), arm, False),
            ("MissingModal", entry.replace("doMarket();", ""), arm, False),
            ("BadRecord", entry, arm.replace("[cell->m_extraInfo]", "[0]"), False),
            ("BadBranch", entry, arm.replace("if (!humanPlayer)", "if (humanPlayer)"), False),
        ]
        fixture = (root / "scripts/experiments/market-artifact-owner-oracle.cpp").read_text()
        candidates, checks = [], []
        for name, body, caller, valid in variants:
            if not valid:
                self.assertNotEqual((body, caller), (entry, arm), name)
            candidate = fixture
            for marker, value in (("RECORD", record), ("MEMBER", member),
                                  ("GLOBALS", "\n".join(declarations)),
                                  ("ENTRY", body), ("ARM", caller)):
                candidate = candidate.replace("// @" + marker + "@", value)
            candidates.append("namespace " + name + " {\n" + candidate + "\n}")
            checks.append("if (" + name + "::check() != " + str(valid).lower()
                          + ') { std::fputs("' + name + '\\n", stderr); return 1; }')
        program = ("#include <cstdio>\n#include <vector>\n" + enumeration + "\n"
                   + "\n".join(candidates) + "\nint main() {\n"
                   + "\n".join(checks) + "\n}\n")
        self.compile_and_run(program)

    def compile_and_run(self, program):
        with tempfile.TemporaryDirectory(prefix="market-artifact-owner-") as directory:
            source_path = Path(directory) / "oracle.cpp"
            source_path.write_text(program)
            executable = Path(directory) / "oracle"
            result = subprocess.run(["g++", "-std=c++98", "-O1", str(source_path),
                                     "-o", str(executable)], capture_output=True,
                                    text=True, timeout=180)
            self.assertEqual(result.returncode, 0, result.stderr[-6000:])
            result = subprocess.run([str(executable)], capture_output=True, text=True, timeout=60)
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)


if __name__ == "__main__":
    unittest.main()
