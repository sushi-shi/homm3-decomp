"""Regression checks for the authored RMG source generators, before VC6 smoke."""
import importlib.util
import itertools
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest


def generator(name):
    path = Path(__file__).resolve().parents[2] / "experiments" / name
    spec = importlib.util.spec_from_file_location(name, path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


class RmgSourceFamilyTests(unittest.TestCase):
    def edit_family_sources(self, original, axes, options):
        changed = dict(original)
        for item, option in zip(axes, options):
            edits = [dict(source=item["source"], find=item["find"], replace=option["replace"])]
            edits += option.get("extra_edits", [])
            for edit in edits:
                kind = next(key for key in ("find", "insert_before", "insert_after") if key in edit)
                anchor = edit[kind]
                self.assertEqual(changed[edit["source"]].count(anchor), 1)
                replacement = edit["replace"] if kind == "find" else edit["text"]
                self.assertNotIn("#pragma", replacement)
                self.assertNotIn("inline", replacement)
                if kind == "insert_before":
                    replacement += anchor
                elif kind == "insert_after":
                    replacement = anchor + replacement
                changed[edit["source"]] = changed[edit["source"]].replace(anchor, replacement)
        return changed

    def test_line_refresh_family_rebases_proxy_and_coupled_tile_query(self):
        module = generator("generate-rmg-line-refresh-family.py")
        root = Path(__file__).resolve().parents[3]
        original = {name: (root / name).read_text() for name in (module.HEADER, module.SOURCE)}
        axes = module.make_axes(original[module.HEADER], original[module.SOURCE])
        self.assertEqual([len(item["options"]) for item in axes], [6, 4, 2, 8])
        for item in axes:
            self.assertEqual(item["find"], item["options"][0]["replace"])
            for option in item["options"]:
                changed = dict(original)
                edits = [dict(source=item["source"], find=item["find"], replace=option["replace"])]
                edits += option.get("extra_edits", [])
                for edit in edits:
                    self.assertEqual(changed[edit["source"]].count(edit["find"]), 1)
                    self.assertNotIn("#pragma", edit["replace"])
                    self.assertNotIn("inline", edit["replace"])
                    changed[edit["source"]] = changed[edit["source"]].replace(edit["find"], edit["replace"])
                rebased = module.make_axes(changed[module.HEADER], changed[module.SOURCE])
                self.assertEqual([len(axis["options"]) for axis in rebased], [6, 4, 2, 8])

    def test_line_proxy_copy_family_rebases_declarations_definitions_and_callers(self):
        module = generator("generate-rmg-line-proxy-copy-family.py")
        root = Path(__file__).resolve().parents[3]
        original = {name: (root / name).read_text() for name in (module.HEADER, module.SOURCE)}
        axes = module.make_axes(original[module.HEADER], original[module.SOURCE])
        self.assertEqual([len(item["options"]) for item in axes], [7, 4, 2, 2])
        for item in axes:
            self.assertEqual(item["find"], item["options"][0]["replace"])
            for option in item["options"]:
                changed = self.edit_family_sources(original, [item], [option])
                rebased = module.make_axes(changed[module.HEADER], changed[module.SOURCE])
                self.assertEqual([len(axis["options"]) for axis in rebased], [7, 4, 2, 2])
                # After adopting any explicit copy, the previous value-ctor
                # family must still select its own overload unambiguously.
                previous = module.parent().make_axes(changed[module.HEADER], changed[module.SOURCE])
                self.assertEqual([len(axis["options"]) for axis in previous], [6, 4, 2, 8])
                for restored in rebased[0]["options"]:
                    final = self.edit_family_sources(changed, [rebased[0]], [restored])
                    self.assertEqual([len(axis["options"]) for axis in module.make_axes(
                        final[module.HEADER], final[module.SOURCE])], [7, 4, 2, 2])

    def test_line_proxy_binding_family_preserves_retained_at_abi_and_rebases(self):
        module = generator("generate-rmg-line-proxy-binding-family.py")
        root = Path(__file__).resolve().parents[3]
        original = {name: (root / name).read_text() for name in (module.HEADER, module.SOURCE)}
        axes = module.make_axes(original[module.HEADER], original[module.SOURCE])
        self.assertEqual([len(item["options"]) for item in axes], [12, 4, 2, 2])
        for item in axes:
            self.assertEqual(item["find"], item["options"][0]["replace"])
            for option in item["options"]:
                changed = self.edit_family_sources(original, [item], [option])
                self.assertIn("TRmgLinePainterTile at(const TRmgGridPoint& point);", changed[module.HEADER])
                body = module.helpers().definition(changed[module.SOURCE], "TRmgLinePainterInterface::at")
                self.assertIn("TRmgLinePainterInterface::at(const TRmgGridPoint& point)", body)
                rebased = module.make_axes(changed[module.HEADER], changed[module.SOURCE])
                self.assertEqual([len(axis["options"]) for axis in rebased], [12, 4, 2, 2])
                previous = module.parent().parent().make_axes(changed[module.HEADER], changed[module.SOURCE])
                self.assertEqual([len(axis["options"]) for axis in previous], [6, 4, 2, 8])

    @unittest.skipUnless(shutil.which("g++"), "portable source-family check needs g++")
    def test_line_refresh_generated_cpp_preserves_proxy_values_and_grid_translation(self):
        self.check_line_proxy_cpp("generate-rmg-line-refresh-family.py")

    @unittest.skipUnless(shutil.which("g++"), "portable source-family check needs g++")
    def test_line_proxy_copy_generated_cpp_preserves_value_ownership(self):
        self.check_line_proxy_cpp("generate-rmg-line-proxy-copy-family.py")

    @unittest.skipUnless(shutil.which("g++"), "portable source-family check needs g++")
    def test_line_proxy_binding_generated_cpp_preserves_value_ownership(self):
        self.check_line_proxy_cpp("generate-rmg-line-proxy-binding-family.py")

    def check_line_proxy_cpp(self, name):
        module = generator(name)
        root = Path(__file__).resolve().parents[3]
        original = {name: (root / name).read_text() for name in (module.HEADER, module.SOURCE)}
        axes = module.make_axes(original[module.HEADER], original[module.SOURCE])
        terrain_header = (root / "include/rmg_terrain.h").read_text()
        tile_start = terrain_header.index("struct rmgTerrainTile {")
        tile = terrain_header[tile_start:terrain_header.index("\n};", tile_start) + 3]
        interface_constructor = module.helpers().definition(
            (root / "src/rmg_support.cpp").read_text(), "TRmgLinePainterInterface::TRmgLinePainterInterface")
        program = []
        total = 0
        for index, options in enumerate(itertools.product(*(item["options"] for item in axes))):
            changed = self.edit_family_sources(original, axes, options)
            header, source = changed[module.HEADER], changed[module.SOURCE]
            point_start = header.index("struct TRmgGridPoint {")
            point = header[point_start:header.index("\n};", point_start) + 3]
            model_start = header.index("class TRmgLinePainterInterface {")
            model = header[model_start:header.index("SIZE(TRmgLinePainterTile,", model_start)]
            program.extend([f"namespace Case{index} {{\n",
                            "struct TPoint { int m_x, m_y; TPoint(int x, int y) : m_x(x), m_y(y) {} };\n",
                            point, "\n", tile, "\nstruct TRmgLinePatternTable {};\n",
                            "struct TRmgLinePainterTile;\n", model, "\n", interface_constructor, "\n"])
            program.extend([module.constructor_definition(source), "\n",
                            module.constructor_definition(source, copy=True, required=False), "\n"])
            if "TRmgGridPoint::operator+=(" in source:
                program.extend([module.helpers().definition(source, "TRmgGridPoint::operator+="), "\n"])
            for method in ("TRmgLinePainterTile::getLand",
                           "TRmgLinePainterTile::getTile", "TRmgLinePainterTile::setTile",
                           "TRmgLinePainterInterface::at"):
                program.extend([module.helpers().definition(source, method), "\n"])
            program.append("""
struct Painter : TRmgLinePainterInterface {
    TRmgGridPoint last;
    rmgTerrainTile saved;
    Painter() : TRmgLinePainterInterface(TRmgGridPoint(10, 10)), saved(3, 4) {
        saved.m_flipX = 13; saved.m_flipY = 17;
    }
    virtual TRmgLinePatternTable* getPattern(int) { return 0; }
    virtual void setTile(const TRmgGridPoint& point, const rmgTerrainTile& tile) { last = point; saved = tile; }
    virtual void setOverlay(const TRmgGridPoint&, int) {}
    virtual int canPaint(const TRmgGridPoint&) { return 0; }
    virtual void getTile(const TRmgGridPoint& point, rmgTerrainTile& tile) { last = point; tile = saved; }
    virtual int getLand(const TRmgGridPoint& point) { last = point; return 19; }
};
int check() {
    Painter painter;
    TRmgGridPoint point(7, 9);
""")
            entry_forms = generator("generate-rmg-line-proxy-copy-family.py").entry_forms()
            refresh = module.helpers().definition(source, "refreshRmgLinePoint")
            entry = next(body for _, body in entry_forms if body in refresh)
            program.append(entry.replace("painter->at", "painter.at") + "\n")
            program.append("""
    point.m_x = 100; point.m_y = 200;
    if (tile.m_painter != &painter || tile.m_point.m_x != 7 || tile.m_point.m_y != 9) return 1;
    if (tile.getLand() != 19 || painter.last.m_x != 7 || painter.last.m_y != 9) return 2;
    TRmgLinePainterTile copied(tile);
    const TRmgLinePainterTile copiedAgain = copied;
    copied.m_point.m_x = 300; copied.m_point.m_y = 400;
    if (tile.m_point.m_x != 7 || tile.m_point.m_y != 9) return 6;
    if (copiedAgain.m_painter != &painter || copiedAgain.m_point.m_x != 7 || copiedAgain.m_point.m_y != 9) return 7;
""")
            query = next(row for row in module.tile_forms() if row[1] in model)
            program.append(query[3])
            program.append("""
    if (current.m_terrain != 3 || current.m_frame != 4 || current.m_flipX != 13 || current.m_flipY != 17) return 3;
    current.m_frame = 23; current.m_flipX = 128; current.m_flipY = 255;
    tile.setTile(current);
    if (painter.last.m_x != 7 || painter.last.m_y != 9 || painter.saved.m_terrain != 3
        || painter.saved.m_frame != 23 || painter.saved.m_flipX != 128 || painter.saved.m_flipY != 255) return 4;
    for (unsigned x = 0; x != 5; ++x) for (unsigned y = 0; y != 5; ++y)
    for (int dx = -1; dx != 2; ++dx) for (int dy = -1; dy != 2; ++dy) {
        TRmgGridPoint origin(x, y);
        TRmgGridPoint sum = origin + TPoint(dx, dy);
        if (sum.m_x != x + dx || sum.m_y != y + dy || origin.m_x != x || origin.m_y != y) return 5;
    }
    return 0;
}
}
""")
            total = index + 1
        program.append("int main() {\n")
        for index in range(total):
            program.append(f"if (Case{index}::check()) return 1;\n")
        program.append("return 0;\n}\n")
        # This oracle checks the complete source cross-product, including
        # unsigned wraparound and copied-coordinate ownership. Its mock painter
        # and extra declarations never enter a matching VC6 translation unit.
        with tempfile.TemporaryDirectory(prefix="rmg-line-refresh-test-") as directory:
            cpp, executable = Path(directory) / "refresh.cpp", Path(directory) / "refresh"
            cpp.write_text("".join(program))
            built = subprocess.run([shutil.which("g++"), "-std=c++98", "-fno-elide-constructors",
                                    str(cpp), "-o", str(executable)],
                                   capture_output=True, text=True, timeout=60)
            self.assertEqual(built.returncode, 0, built.stderr)
            checked = subprocess.run([str(executable)], capture_output=True, text=True, timeout=20)
            self.assertEqual(checked.returncode, 0, checked.stderr)

    def test_line_selector_family_preserves_cases_and_rebases_joined_exits(self):
        module = generator("generate-rmg-line-selector-family.py")
        root = Path(__file__).resolve().parents[3]
        source = (root / module.SOURCE).read_text()
        axes = module.make_axes(source)
        self.assertIn(len(axes[0]["options"]), (288, 289))
        self.assertEqual(len(axes[1]["options"]), 36)
        refined = module.make_axes(source, refine=True)
        self.assertIn(len(refined[0]["options"]), (48, 49))
        self.assertEqual(len(refined[1]["options"]), 6)
        for item in refined:
            self.assertEqual(item["find"], item["options"][0]["replace"])
        for item in axes:
            self.assertEqual(item["find"], item["options"][0]["replace"])
            for option in item["options"]:
                self.assertNotIn("#pragma", option["replace"])
                self.assertNotIn("inline", option["replace"])
            for option in (item["options"][0], item["options"][len(item["options"]) // 2], item["options"][-1]):
                replacement = source.replace(item["find"], option["replace"])
                for edit in option.get("extra_edits", []):
                    replacement = replacement.replace(edit["find"], edit["replace"])
                rebased = module.make_axes(replacement)
                self.assertIn(len(rebased[0]["options"]), (288, 289))
                self.assertEqual(len(rebased[1]["options"]), 36)
        for option in axes[1]["options"]:
            body = option["replace"]
            self.assertEqual(body.count("m_ranges[5].m_valueCount"), 1)
            self.assertIn("neighbours[order[2]] && neighbours[order[4]]", body)
            self.assertIn("neighbours[order[1]] || neighbours[order[5]]", body)
            self.assertEqual(body.count("return;"), 1)

    def test_line_selector_end_family_rebases_complete_decision_arms(self):
        module = generator("generate-rmg-line-selector-end-family.py")
        source = (Path(__file__).resolve().parents[3] / module.parent().SOURCE).read_text()
        axes = module.make_axes(source)
        self.assertIn(len(axes[0]["options"]), (48, 49))
        self.assertEqual(len(axes[1]["options"]), 6)
        for item in axes:
            self.assertEqual(item["find"], item["options"][0]["replace"])
            for option in item["options"]:
                changed = source.replace(item["find"], option["replace"])
                for edit in option.get("extra_edits", []):
                    changed = changed.replace(edit["find"], edit["replace"])
                rebased = module.make_axes(changed)
                self.assertIn(len(rebased[0]["options"]), (48, 49))
                self.assertEqual(len(rebased[1]["options"]), 6)

    @unittest.skipUnless(shutil.which("g++"), "portable source-family check needs g++")
    def test_line_selector_generated_cpp_covers_all_masks_and_optional_patterns(self):
        module = generator("generate-rmg-line-selector-family.py")
        root = Path(__file__).resolve().parents[3]
        source = (root / module.SOURCE).read_text()
        original = module.helpers().definition(source, module.SIGNATURE)
        axes = module.make_axes(source)
        cases = [original]
        for item in axes:
            for option in item["options"][1:]:
                body = original.replace(item["find"], option["replace"])
                for edit in option.get("extra_edits", []):
                    body = body.replace(edit["find"], edit["replace"])
                cases.append(body)
        # Exercise the complete focused cross-product as well: a shared exit
        # changes the lifetime scope containing the independently chosen loop.
        focused_families = [module.make_axes(source, refine=True),
                           generator("generate-rmg-line-selector-end-family.py").make_axes(source)]
        for refined in focused_families:
            for options in itertools.product(*(item["options"] for item in refined)):
                body = original
                for item, option in zip(refined, options):
                    body = body.replace(item["find"], option["replace"])
                    for edit in option.get("extra_edits", []):
                        body = body.replace(edit["find"], edit["replace"])
                cases.append(body)
        cases = list(dict.fromkeys(cases))
        program = ["enum { TILE_DIR_NORTH, TILE_DIR_NORTHEAST, TILE_DIR_EAST, TILE_DIR_SOUTHEAST,\n"
                   "TILE_DIR_SOUTH, TILE_DIR_SOUTHWEST, TILE_DIR_WEST, TILE_DIR_NORTHWEST };\n"
                   "struct TRmgLinePatternRange { unsigned m_firstIndex, m_valueCount; };\n"
                   "struct TRmgLinePatternTable { unsigned m_patternCount; int* m_patterns; TRmgLinePatternRange m_ranges[9]; };\n"
                   "const int g_rmgLineReflectedNeighbours[2][2][8] = {\n"
                   "{{0,1,2,3,4,5,6,7},{4,3,2,1,0,7,6,5}},\n"
                   "{{0,7,6,5,4,3,2,1},{4,5,6,7,0,1,2,3}} };\n"
                   "const unsigned char g_rmgLineReflections[4][2] = {{0,0},{0,1},{1,0},{1,1}};\n"]
        for index, body in enumerate(cases):
            program.extend([f"namespace Case{index} {{\n", body, "\n}\n"])
        program.append("""
void expected(const unsigned char* n, unsigned options, int& p, unsigned char& x, unsigned char& y) {
    x = y = 0;
    if (n[0] && n[2] && n[4] && n[6]) { p = 8; return; }
    if (n[0] && n[4]) { p = n[2] || n[6] ? 6 : 2; x = !n[2] && n[6] ? 1 : 0; return; }
    if (n[2] && n[6]) { p = n[4] || n[0] ? 7 : 3; y = !n[4] && n[0] ? 1 : 0; return; }
    for (unsigned i = 0; i != 4; ++i) {
        unsigned a = i / 2, b = i % 2;
        const int* row = g_rmgLineReflectedNeighbours[a][b];
        if (n[row[2]] && n[row[4]]) {
            p = 4 + ((options & 2) && (n[row[1]] || n[row[5]]) ? 1 : 0);
            x = a; y = b; return;
        }
    }
    if (!(options & 1)) { p = n[2] || n[6] ? 3 : 2; return; }
    if (n[2] || n[6]) { p = 1; x = n[6]; return; }
    p = 0; y = n[4] ? 0 : 1;
}
int main() {
    for (unsigned options = 0; options != 4; ++options)
    for (unsigned mask = 0; mask != 256; ++mask)
    for (unsigned amplitude = 1; amplitude <= 128; amplitude *= 128) {
        unsigned char neighbours[8];
        for (unsigned bit = 0; bit != 8; ++bit) neighbours[bit] = mask & (1 << bit) ? amplitude : 0;
        TRmgLinePatternTable table = {0};
        table.m_ranges[0].m_valueCount = options & 1 ? 0x80000000u : 0;
        table.m_ranges[5].m_valueCount = options & 2 ? 0x80000000u : 0;
        int wanted; unsigned char wantedX, wantedY;
        expected(neighbours, options, wanted, wantedX, wantedY);
""")
        for index in range(len(cases)):
            program.append(f"{{ int p = -1; unsigned char x = 37, y = 41;\n"
                           f"Case{index}::selectRmgLinePattern(neighbours, &table, p, x, y);\n"
                           "if (p != wanted || x != wantedX || y != wantedY) return 1; }\n")
        program.append("}\nreturn 0;\n}\n")
        with tempfile.TemporaryDirectory(prefix="rmg-line-selector-test-") as directory:
            cpp = Path(directory) / "selector.cpp"
            executable = Path(directory) / "selector"
            cpp.write_text("".join(program))
            built = subprocess.run([shutil.which("g++"), "-std=c++98", str(cpp), "-o", str(executable)],
                                   capture_output=True, text=True, timeout=60)
            self.assertEqual(built.returncode, 0, built.stderr)
            checked = subprocess.run([str(executable)], capture_output=True, text=True, timeout=20)
            self.assertEqual(checked.returncode, 0, checked.stderr)

    def test_line_walk_family_preserves_axis_records_and_paint_boundaries(self):
        module = generator("generate-rmg-line-walk-family.py")
        root = Path(__file__).resolve().parents[3]
        header = (root / "include/rmg.h").read_text()
        source = (root / "src/rmg_terrain.cpp").read_text()
        axes = module.make_axes(header, source)
        self.assertEqual([len(item["options"]) for item in axes], [24, 8, 32])
        refined = module.make_axes(header, source, refine=True)
        self.assertIn(len(refined[0]["options"]), (3, 4))
        self.assertEqual(len(refined[1]["options"]), 2)
        self.assertIn(len(refined[2]["options"]), (12, 13))
        for item in refined:
            self.assertEqual(item["find"], item["options"][0]["replace"])
        for item in axes:
            self.assertEqual(item["find"], item["options"][0]["replace"])
            for option in item["options"]:
                replacement = option["replace"]
                self.assertNotIn("#pragma", replacement)
                self.assertNotIn("inline", replacement)
                new_header, new_source = header, source
                if item["source"].endswith(".h"):
                    new_header = header.replace(item["find"], replacement)
                else:
                    new_source = source.replace(item["find"], replacement)
                rebased = module.make_axes(new_header, new_source)
                self.assertEqual([len(value["options"]) for value in rebased], [24, 8, 32])
        for option in axes[2]["options"]:
            body = option["replace"]
            self.assertEqual(body.count("paintPoint("), 3)
            self.assertEqual(body.count("minor->m_position += minor->m_step;"), 1)
            self.assertEqual(body.count("major->m_position += major->m_step;"), 1)
            self.assertEqual(body.count("error -= major->m_distance;"), 1)
            self.assertTrue(body.endswith("    m_position = destination;\n"))

    @unittest.skipUnless(shutil.which("g++"), "portable source-family check needs g++")
    def test_line_walk_generated_cpp_preserves_every_visited_point(self):
        module = generator("generate-rmg-line-walk-family.py")
        root = Path(__file__).resolve().parents[3]
        header = (root / "include/rmg.h").read_text()
        source = (root / "src/rmg_terrain.cpp").read_text()
        axes = module.make_axes(header, source)
        baseline = module.helpers().definition(source, "TRmgLineWalker::drawTo")
        cases = [(axes[0]["find"], baseline)]
        for item in axes:
            for option in item["options"][1:]:
                constructor = axes[0]["find"]
                body = baseline
                if item["source"].endswith(".h"):
                    constructor = option["replace"]
                else:
                    body = body.replace(item["find"], option["replace"])
                cases.append((constructor, body))
        program = ["#include <vector>\n#include <utility>\n",
                   "typedef std::pair<unsigned, unsigned> Pair;\n",
                   "struct TRmgGridPoint { unsigned m_x, m_y; TRmgGridPoint() {}\n"
                   "TRmgGridPoint(const unsigned& x, const unsigned& y) : m_x(x), m_y(y) {} };\n"]
        for index, (constructor, body) in enumerate(cases):
            program.extend([f"namespace Case{index} {{\n",
                            "struct TRmgLineWalkAxis { unsigned m_position, m_distance; int m_step;\n",
                            constructor, "\n};\n",
                            "struct TRmgLineWalker { TRmgGridPoint m_position; std::vector<Pair> visits;\n"
                            "void paintPoint(const TRmgGridPoint& point) { visits.push_back(Pair(point.m_x, point.m_y)); }\n"
                            "void drawTo(const TRmgGridPoint& destination); };\n",
                            body, "\n}\n"])
        program.append("int main() {\n"
                       "for (unsigned x = 0; x != 5; ++x) for (unsigned y = 0; y != 5; ++y)\n"
                       "for (unsigned a = 0; a != 5; ++a) for (unsigned b = 0; b != 5; ++b) {\n"
                       "TRmgGridPoint from(x, y), to(a, b);\n"
                       "Case0::TRmgLineWalker control; control.m_position = from; control.drawTo(to);\n"
                       "if (control.visits.empty() || control.visits.front() != Pair(a, b)) return 1;\n"
                       "unsigned dx = x >= a ? x - a : a - x, dy = y >= b ? y - b : b - y;\n"
                       "if (control.visits.size() != dx + dy + (dx == dy ? 1 : 0)) return 2;\n"
                       "if (dx == dy && control.visits.back() != Pair(x, y)) return 5;\n")
        for index in range(1, len(cases)):
            program.append(f"{{ Case{index}::TRmgLineWalker probe; probe.m_position = from; probe.drawTo(to);\n"
                           "if (probe.visits != control.visits) return 3;\n"
                           "if (probe.m_position.m_x != a || probe.m_position.m_y != b) return 4; }\n")
        program.append("}\nreturn 0;\n}\n")
        # This host build tests generated C++ semantics only. VC6 under Wine
        # remains the sole codegen verdict and sees no harness declarations.
        with tempfile.TemporaryDirectory(prefix="rmg-line-walk-test-") as directory:
            cpp = Path(directory) / "walk.cpp"
            executable = Path(directory) / "walk"
            cpp.write_text("".join(program))
            built = subprocess.run([shutil.which("g++"), "-std=c++98", str(cpp), "-o", str(executable)],
                                   capture_output=True, text=True, timeout=60)
            self.assertEqual(built.returncode, 0, built.stderr)
            checked = subprocess.run([str(executable)], capture_output=True, text=True, timeout=20)
            self.assertEqual(checked.returncode, 0, checked.stderr)

    def test_clear_declarations_keep_all_reads_after_vector_erase(self):
        module = generator("generate-rmg-clear-declaration-family.py")
        root = Path(__file__).resolve().parents[3]
        source = (root / "src/rmg.cpp").read_text()
        item, = module.make_axes(source)
        self.assertEqual(item["find"], item["options"][0]["replace"])
        self.assertGreaterEqual(len(item["options"]), 300)
        for option in item["options"][1:]:
            body = option["replace"]
            call = next(body.index(text) for text in
                        ("m_objects.erase(", "m_objects.clear(", "objects.erase(", "objects.clear(")
                        if text in body)
            for kind, local, member in module.previous().SNAPSHOTS:
                self.assertEqual(body.count(kind + " " + local), 1)
                self.assertEqual(body.count(f"{local} = {member};"), 1)
                self.assertLess(call, body.index(f"{local} = {member};"))
            self.assertTrue(body.endswith(item["find"][item["find"].index("    connection.m_present = 0;"):]))
            self.assertNotIn("#pragma", body)
        for option in (item["options"][17], item["options"][-1]):
            rebased, = module.make_axes(source.replace(item["find"], option["replace"]))
            self.assertEqual(rebased["options"][0]["replace"], option["replace"])

    def test_clear_lifetime_family_preserves_fields_and_pointer_vector_semantics(self):
        module = generator("generate-rmg-clear-lifetime-family.py")
        root = Path(__file__).resolve().parents[3]
        source = (root / "src/rmg.cpp").read_text()
        item, = module.make_axes(source)
        self.assertIn(len(item["options"]), (216, 217))
        self.assertEqual(item["find"], item["options"][0]["replace"])
        tail = item["find"][item["find"].index("    connection.m_present = 0;"):]
        for option in item["options"]:
            body = option["replace"]
            self.assertTrue(body.endswith(tail))
            self.assertEqual(sum(body.count(call.strip()) for _, call in module.CALLS), 1)
            self.assertNotIn("#pragma", body)
            for kind, local, member in module.SNAPSHOTS:
                self.assertEqual(body.count(kind + " " + local), 1)
                self.assertEqual(body.count(member + ";") + body.count(member + ");"), 1)
        for option in (item["options"][0], item["options"][17], item["options"][-1]):
            rebased, = module.make_axes(source.replace(item["find"], option["replace"]))
            self.assertIn(len(rebased["options"]), (216, 217))

    def test_zone_math_family_preserves_signed_distance_and_selector_lifetime(self):
        module = generator("generate-rmg-zone-math-family.py")
        root = Path(__file__).resolve().parents[3]
        source = (root / "src/rmg.cpp").read_text()
        axes = module.make_axes(source)
        self.assertEqual([len(item["options"]) for item in axes], [48, 27, 48, 18])
        for item in axes:
            self.assertEqual(item["find"], item["options"][0]["replace"])
            self.assertEqual(source.count(item["find"]), 1)
            for option in item["options"]:
                self.assertNotIn("#pragma", option["replace"])
                rebased = module.make_axes(source.replace(item["find"], option["replace"]))
                self.assertEqual([len(value["options"]) for value in rebased], [48, 27, 48, 18])
        for item in (axes[0], axes[2]):
            for option in item["options"]:
                body = option["replace"]
                self.assertEqual(body.count("sqrt("), 1)
                self.assertEqual(body.count("static_cast<double>("), 1)
                self.assertNotIn("unsigned", body)
                self.assertEqual(body.count("int distance = static_cast<int>("), 1)
        for option in axes[1]["options"]:
            body = option["replace"]
            self.assertIn("if (otherSize < minimumSize)", body)
            self.assertEqual(body.count("other->m_slot->m_size"), 1)
            self.assertEqual(body.count("minimumSize / 2"), 1)
        for option in axes[3]["options"]:
            body = option["replace"]
            self.assertEqual(body.count("rand()"), 1)
            self.assertLess(body.index("return 0;"), body.index("rand()"))
            self.assertEqual(body.count("candidates["), 1)

    def test_tile_value_family_keeps_four_fields_and_virtual_boundaries(self):
        module = generator("generate-rmg-tile-value-family.py")
        root = Path(__file__).resolve().parents[3]
        header = (root / "include/rmg_terrain.h").read_text()
        source = (root / "src/rmg_support.cpp").read_text()
        terrain = (root / "src/rmg_terrain.cpp").read_text()
        axes = module.make_axes(header, source, terrain)
        self.assertEqual([len(item["options"]) for item in axes], [26, 49, 3, 5, 3])
        refined = module.make_axes(header, source, terrain, refine=True)
        self.assertIn(len(refined[0]["options"]), (1, 2))
        self.assertEqual([len(item["options"]) for item in refined[1:]], [49, 3, 5, 1])
        for item in axes:
            self.assertEqual(item["find"], item["options"][0]["replace"])
        for label, body in module.copy_forms():
            if label == "implicit":
                self.assertNotIn("rmgTerrainTile(", body)
                continue
            for field in module.FIELDS:
                self.assertEqual(body.count("other.m_" + field), 1)
            self.assertNotIn("m_tailPadding", body)
        for label, body in module.assignment_forms():
            if label == "implicit":
                self.assertNotIn("operator=", body)
            else:
                for field in module.FIELDS:
                    self.assertEqual(body.count(f"m_{field} = other.m_{field};"), 1)
                self.assertEqual(body.count("return *this;"), 1)
        for item in axes:
            for option in item["options"]:
                new_header, new_source, new_terrain = header, source, terrain
                if item["source"].endswith(".h"):
                    new_header = new_header.replace(item["find"], option["replace"])
                elif item["source"] == "src/rmg_terrain.cpp":
                    new_terrain = new_terrain.replace(item["find"], option["replace"])
                else:
                    new_source = new_source.replace(item["find"], option["replace"])
                    for edit in option.get("extra_edits", []):
                        new_source = new_source.replace(edit["find"], edit["replace"])
                rebased = module.make_axes(new_header, new_source, new_terrain)
                self.assertEqual([len(value["options"]) for value in rebased], [26, 49, 3, 5, 3])
                for label, body in module.getters("TRmgRoadLinePainter"):
                    self.assertEqual(body.count("m_adapter->getTile(point)"), 1)
                    self.assertIn("rmgTerrainTile& tile)", body)
        for label, body in module.setters("TRmgLinePainter"):
            self.assertEqual(body.count("m_adapter->setTile(point, snapshot)"), 1)
            if label.startswith("constructed+"):
                for field in module.FIELDS:
                    self.assertEqual(body.count("tile.m_" + field), 1)
                self.assertIn("snapshot(tile.m_terrain, tile.m_frame)", body)

    def test_copy_cursor_family_preserves_allocation_masks_and_false_return(self):
        module = generator("generate-rmg-copy-cursor-family.py")
        root = Path(__file__).resolve().parents[3]
        axes = module.make_axes((root / "src/rmg.cpp").read_text(),
                                (root / "src/rmg_support.cpp").read_text())
        self.assertEqual([len(item["options"]) for item in axes], [62, 32, 32])
        for item, generate in zip(axes, (module.pattern_variants, module.footprint_variants,
                                         module.placement_variants)):
            self.assertEqual(item["find"], item["options"][0]["replace"])
            expected = list(generate(item["find"]))
            for option in item["options"]:
                self.assertNotIn("#pragma", option["replace"])
                self.assertEqual(list(generate(option["replace"])), expected)
        for option in axes[0]["options"]:
            body = option["replace"]
            self.assertEqual(body.count("new int[m_patternCount]"), 1)
            self.assertEqual(body.count("throw TAllocationFailure();"), 1)
            self.assertEqual(body.count("std::copy("), 1)
            self.assertLess(body.index("throw TAllocationFailure"), body.index("std::copy("))
            self.assertLess(body.index("std::copy("), body.index("for (unsigned int value"))
            self.assertIn("++m_ranges[previous].m_valueCount;", body)
        for option in axes[1]["options"]:
            body = option["replace"]
            self.assertEqual(body.count("getMapItem("), 1)
            self.assertEqual(body.count("++x"), 1)
            self.assertEqual(body.count("++y"), 1)
            self.assertEqual(body.count("getBitPos(x, y)"), 2)
            self.assertLess(body.index("m_triggerMask.test("), body.index("m_passableMask.test("))
        for option in axes[2]["options"]:
            body = option["replace"]
            self.assertEqual(body.count("hasConnectedOutline("), 1)
            self.assertEqual(body.count("isPlacementBlocked("), 1)
            self.assertEqual(body.count("buildOutline()"), 1)
            self.assertEqual(body.count("m_objects[0]->m_properties->m_prototype->m_objectType"), 1)
            self.assertEqual(body.count("item->m_zoneState.m_zone < 0"), 1)
            self.assertEqual(body.count("item->m_zoneState.m_zone != zoneIndex"), 1)
            if "return connected;" in body:
                self.assertIn("if (!connected)\n        return connected;", body)
            if " terrain = " in body:
                self.assertLess(body.index("if (!item->m_tileData.m_roadPassable)"),
                                body.index(" terrain = "))

    def test_outline_refinement_preserves_walk_and_coordinate_progression(self):
        module = generator("generate-rmg-outline-refine-family.py")
        source = (Path(__file__).resolve().parents[3] / "src/rmg.cpp").read_text()
        axes = module.make_axes(source)
        self.assertEqual([len(item["options"]) for item in axes[:5]], [15, 6, 4, 2, 48])
        self.assertIn(len(axes[5]["options"]), (16, 17))
        self.assertEqual(len(axes[6]["options"]), 49)
        for item in axes:
            self.assertEqual(item["find"], item["options"][0]["replace"])
            for option in item["options"]:
                self.assertNotIn("#pragma", option["replace"])
        for _, body in module.outline_entries():
            self.assertEqual(body.count("--position.m_x"), 1)
            self.assertEqual(body.count("m_passableMask.test("), 1)
            self.assertEqual(body.count("m_triggerMask.test("), 1)
        for _, body in module.connected_variants(axes[4]["find"]):
            self.assertEqual(body.count("previouslyBlocked = blocked;"), 1)
            self.assertEqual(body.count("unsigned char previouslyBlocked"), 1)
            self.assertIn("index < outline.size() + 1", body)
            self.assertEqual(list(module.connected_variants(body)),
                             list(module.connected_variants(axes[4]["find"])))
        for _, body in module.footprint_variants(axes[5]["find"]):
            self.assertEqual(body.count("getMapItem("), 1)
            self.assertEqual(body.count("++x"), 1)
            self.assertEqual(body.count("++y"), 1)
            self.assertEqual(body.count("--row") + body.count("--nearby.m_y"), 1)
            self.assertEqual(body.count("--column") + body.count("--nearby.m_x"), 1)
            self.assertEqual(body.count("getBitPos(x, y)"), 2)
            self.assertLess(body.index("m_triggerMask.test("), body.index("m_passableMask.test("))
            self.assertEqual(list(module.footprint_variants(body)),
                             list(module.footprint_variants(axes[5]["find"])))
        for _, body in module.placement_variants(axes[6]["find"]):
            self.assertEqual(body.count("int x = position.m_x;"), 1)
            self.assertEqual(body.count("int y = position.m_y;"), 1)
            self.assertLess(body.index("int x ="), body.index("isPlacementBlocked("))
            self.assertLess(body.index("hasConnectedOutline("), body.index("x -= prototype"))
            self.assertEqual(body.count("getMapItem("), 1)
            self.assertEqual(list(module.placement_variants(body)),
                             list(module.placement_variants(axes[6]["find"])))

    def test_outline_family_keeps_masks_helpers_and_walk_order(self):
        module = generator("generate-rmg-outline-family.py")
        root = Path(__file__).resolve().parents[3]
        source = (root / "src/rmg.cpp").read_text()
        header = (root / "include/rmg.h").read_text()
        axes = module.make_axes(header, source)
        self.assertEqual([len(item["options"]) for item in axes[:4]], [4, 6, 16, 72])
        self.assertIn(len(axes[4]["options"]), (12, 13))
        self.assertEqual(len(axes[5]["options"]), 12)
        self.assertIn(len(axes[6]["options"]), (36, 37))
        for item in axes:
            self.assertEqual(item["options"][0]["replace"], item["find"])
            for option in item["options"]:
                self.assertNotIn("#pragma", option["replace"])
        for option in axes[3]["options"]:
            body = option["replace"]
            self.assertEqual(body.count("m_passableMask.test("), 2)
            self.assertEqual(body.count("m_triggerMask.test("), 2)
            self.assertEqual(body.count("direction = (direction - 2) & 7;"), 1)
            self.assertEqual(body.count("direction = (direction - 4) & 7;"), 1)
            self.assertEqual(body.count("} while (++attempts < 4);"), 1)
            self.assertEqual(body.count("} while (position != start);")
                             + body.count("} while (start != position);"), 1)
        for option in axes[4]["options"]:
            body = option["replace"]
            self.assertLess(body.index("m_triggerMask.test("), body.index("m_passableMask.test("))
            self.assertLess(body.index("for (unsigned int y"), body.index("for (unsigned int x"))
            self.assertEqual(body.count("isRoadEntrance()"), 2)
            self.assertEqual(body.count("hasBorderObject()"), 1)
            self.assertEqual(body.count("m_recommendedTerrainMask.test(eTerrainWater)"), 2)
        for option in axes[5]["options"]:
            body = option["replace"]
            self.assertIn("index < outline.size() + 1", body)
            self.assertIn("outline[index % outline.size()]", body)
            self.assertEqual(body.count("isRoadEntrance()"), 2)
            self.assertEqual(body.count("hasSubterraneanGate()"), 1)
            self.assertEqual(body.count("foundBoundary = 1;"), 1)
        for option in axes[6]["options"]:
            body = option["replace"]
            for call in ("isPlacementBlocked(", "hasConnectedOutline(", "buildOutline()", "getMapItem("):
                self.assertEqual(body.count(call), 1)
            self.assertLess(body.index("isPlacementBlocked("), body.index("buildOutline()"))
            self.assertLess(body.index("buildOutline()"), body.index("hasConnectedOutline("))
            self.assertLess(body.index("hasConnectedOutline("), body.index("m_hasTrigger)"))
            self.assertEqual(body.count("item->m_zoneState.m_zone < 0"), 1)
            self.assertEqual(body.count("item->m_zoneState.m_zone != zoneIndex"), 1)
        # Every former winner is a valid baseline for the next generation.
        for item, generate, choices in (
                (axes[3], module.outline_body, ("shared", "independent", "copy", "push")),
                (axes[4], module.footprint_body, ("scalars", "shared", "pointer")),
                (axes[5], module.connected_body, ("copy", "expression")),
                (axes[6], module.placement_body, ("parameter", "point", "direct", "expression"))):
            expected = generate(item["find"], *choices)
            for option in item["options"]:
                self.assertEqual(generate(option["replace"], *choices), expected)

    def test_guard_and_placement_family_preserves_boundaries_and_random_order(self):
        module = generator("generate-rmg-guard-placement-family.py")
        root = Path(__file__).resolve().parents[3]
        source = (root / "src/rmg.cpp").read_text()
        header = (root / "include/rmg.h").read_text()
        axes = module.make_axes(header, source)
        self.assertEqual([len(item["options"]) for item in axes], [4, 6, 16, 4, 72])
        for item in axes:
            self.assertEqual(item["options"][0]["replace"], item["find"])
        for option in axes[0]["options"]:
            body = option["replace"]
            self.assertEqual(body.count("prototypeIndices[creature] = -1;"), 1)
            self.assertEqual(body.count("RMG_GUARD_ROE_EXCLUDED_FIRST"), 1)
            self.assertEqual(body.count("RMG_GUARD_ROE_CREATURE_LIMIT"), 1)
        for option in axes[1]["options"]:
            body = option["replace"]
            self.assertEqual(body.count("rand() % variation"), 2)
            self.assertEqual(body.count("new rmgMonsterObject(properties, m_nextObjectId++,"), 1)
            self.assertLess(body.index("rand() % variation"), body.index("new rmgMonsterObject"))
            self.assertNotIn("rand() % variation - rand()", body)
        for label, body in module.constructors():
            self.assertEqual(body.count(": type_object(properties)"), 1)
            for field in ("m_count", "m_disposition", "m_objectId"):
                self.assertEqual(body.count(field), 1)
            self.assertNotIn("m_unknown28", body)
            self.assertNotIn("inline", body)
        for option in axes[3]["options"]:
            self.assertEqual(option["replace"].count("outfile->write(&intBuffer, sizeof(short));"), 1)
        for option in axes[4]["options"]:
            body = option["replace"]
            self.assertEqual(body.count("rand()"), 1)
            self.assertEqual(body.count("addObject("), 1)
            self.assertEqual(body.count("canPlaceObject(properties, position, zone)"), 1)
            self.assertLess(body.index("for (position.m_y"), body.index("for (position.m_x"))
            self.assertNotIn("#pragma", body)
            self.assertNotIn("inline", body)
            self.assertEqual(module.placement(body, "xy", "copy", "assign", "push"),
                             module.placement(axes[4]["find"], "xy", "copy", "assign", "push"))

    def test_map_position_family_keeps_layout_constructor_and_arithmetic(self):
        module = generator("generate-rmg-position-family.py")
        root = Path(__file__).resolve().parents[3]
        header = (root / "include/rmg.h").read_text()
        source = (root / "src/rmg.cpp").read_text()
        axes = module.make_axes(header, source)
        self.assertEqual([len(item["options"]) for item in axes], [8, 7, 288])
        for name, text in module.copy_forms()[2:]:
            for field in "xyz":
                self.assertEqual(text.count(f"m_{field} = other.m_{field};"), 1)
        for name, text in module.assignment_forms()[1:]:
            for field in "xyz":
                self.assertEqual(text.count(f"m_{field} = other.m_{field};"), 1)
            self.assertEqual(text.count("return *this;"), 1)
        for option in axes[2]["options"]:
            body = option["replace"]
            self.assertEqual(body.count("TRmgMapPosition::operator+(TPoint offset) const"), 1)
            self.assertEqual(body.count("result += offset;"), 1)
            for op in ("+=", "-="):
                for field in "xy":
                    self.assertEqual(body.count(f"m_{field} {op} offset.m_{field};"), 1)
            self.assertNotIn("#pragma", body)
        # Rebase from every explicit special-member form, not only implicit.
        for copy in axes[0]["options"]:
            for assignment in axes[1]["options"]:
                candidate = header.replace(axes[0]["find"], copy["replace"]).replace(
                    axes[1]["find"], assignment["replace"])
                rebased = module.make_axes(candidate, source)
                self.assertEqual([len(item["options"]) for item in rebased], [8, 7, 288])

    def test_path_scopes_keep_post_placement_query_and_same_zone_walk(self):
        module = generator("generate-rmg-path-family.py")
        source = (Path(__file__).resolve().parents[3] / "src/rmg.cpp").read_text()
        original = module.definition(source, "type_random_map_generator::openConnectionPath")
        axes = module.path_axes(original)
        self.assertEqual([len(item["options"]) for item in axes], [3, 72, 3])
        for option in axes[0]["options"]:
            body = option["replace"]
            self.assertEqual(body.count("selectObjectPrototype("), 1)
            self.assertEqual(body.count("addObject("), 1)
            self.assertIn("if (!item->m_connection.m_present)", body[body.index("addObject("):])
        for option in axes[1]["options"]:
            body = option["replace"]
            self.assertIn("nearby->m_zoneState.m_zone == zone", body)
            self.assertEqual(body.count("m_borderObject = 0"), 1)
            self.assertNotIn("m_subterraneanGate =", body)
        selector = module.definition(source, "type_random_map_generator::selectObjectPrototype")
        self.assertEqual(module.selector_body(selector, "member", "continue", "push_back"), selector)
        for form in itertools.product(("member", "reference", "const_reference"),
                                      ("continue", "nested"), ("push_back", "insert_value", "insert_count")):
            body = module.selector_body(selector, *form)
            self.assertEqual(body.count("m_recommendedTerrainMask.test(terrain)"), 1)
            self.assertEqual(body.count("rand()"), 1)
            self.assertLess(body.index("m_subtype"), body.index("m_slotCategory"))
            self.assertNotIn("#pragma", body)

    def test_shipyard_family_preserves_helpers_and_query_order(self):
        module = generator("generate-rmg-shipyard-family.py")
        source = (Path(__file__).resolve().parents[3] / "src/rmg.cpp").read_text()
        body = module.definition(source, "type_random_map_generator::canPlaceShipyard")
        axes = module.shipyard_axes(body)
        self.assertEqual([len(item["options"]) for item in axes], [5, 6, 4, 5])
        for choices in itertools.product(*(item["options"] for item in axes)):
            candidate = body
            for item, choice in zip(axes, choices):
                self.assertEqual(candidate.count(item["find"]), 1)
                candidate = candidate.replace(item["find"], choice["replace"])
                for edit in choice.get("extra_edits", []):
                    self.assertEqual(candidate.count(edit["find"]), 1)
                    candidate = candidate.replace(edit["find"], edit["replace"])
            self.assertEqual(candidate.count("getMapItem("), 3)
            self.assertEqual(candidate.count("hasSubterraneanGate()"), 1)
            self.assertLess(candidate.index("for (nearby.m_y"), candidate.index("for (nearby.m_x"))
            self.assertLess(candidate.index("for (nearby.m_x"), candidate.index("for (waterOffset"))
            self.assertNotIn("#pragma", candidate)
        flood = module.definition(source, "type_random_map_generator::floodConnectionRegion")
        self.assertEqual(len(module.flood_axis(flood)["options"]), 6)

    def test_border_storage_preserves_each_bound_and_tail(self):
        module = generator("generate-rmg-border-flood-family.py")
        source = ("void type_random_map_generator::markBorderObjectArea()\n{\n"
                  "    int minimumX = max(position.m_x - 1, 0);\n"
                  "    int maximumX = min(position.m_x + 2, m_map.m_mapWidth);\n"
                  "    int minimumY = max(position.m_y - 1, 0);\n"
                  "    int maximumY = min(position.m_y + 2, m_map.m_mapHeight);\n"
                  "    for (int y = minimumY; y < maximumY; ++y) {\n"
                  "        for (int x = minimumX; x < maximumX; ++x) {}\n    }\n}")
        seen = set()
        for storage in ("scalars", "corners", "rectangle"):
            for order in itertools.permutations(range(4)):
                body = module.border(source, storage, order)
                self.assertEqual(body.count("max(position."), 2)
                self.assertEqual(body.count("min(position."), 2)
                self.assertLess(body.index("for (int y ="), body.index("for (int x ="))
                self.assertNotIn("#pragma", body)
                self.assertEqual(module.border(body, "scalars", (0, 1, 2, 3)), source)
                seen.add(body)
        self.assertEqual(len(seen), 72)
        self.assertEqual(module.border(source, "scalars", (0, 1, 2, 3)), source)

    def test_function_replacement_includes_return_type(self):
        module = generator("generate-rmg-diagonal-family.py")
        original = "// evidence\nstatic const int& clampRmgTerrainCoordinate(\n    const int& value)\n{\n    return value;\n}\n"
        self.assertEqual(module.definition(original, "clampRmgTerrainCoordinate"),
                         original.split("\n", 1)[1].rstrip())

    def test_neighbour_alternatives_preserve_query_order_and_helpers(self):
        module = generator("generate-rmg-neighbour-clear-family.py")
        root = Path(__file__).resolve().parents[3]
        terrain, source = (root / "src/rmg_terrain.cpp").read_text(), (root / "src/rmg.cpp").read_text()
        axes = module.make_axes(terrain, source)
        for item in axes:
            self.assertEqual(item["find"], item["options"][0]["replace"])
        for option in axes[2]["options"]:
            new_source = source.replace(axes[2]["find"], option["replace"])
            if option["name"].endswith("expression") or option["name"] == "baseline":
                rebased = module.make_axes(terrain, new_source)
                self.assertIn(len(rebased[2]["options"]), (18, 19))
            self.assertEqual(option["replace"].count("m_objects.erase("), 1)
        directions = ["NORTH", "SOUTH", "WEST", "EAST", "NORTHWEST", "NORTHEAST", "SOUTHWEST", "SOUTHEAST"]
        for construction, query in itertools.product(
                ("temporary", "direct", "copy_init", "const_reference", "reused", "distinct"),
                ("nested", "named")):
            with self.subTest(construction=construction, query=query):
                body = module.visits(construction, query)
                self.assertEqual(body.count("getTerrain("), 8)
                self.assertEqual(body.count("getRmgTerrainNeighbourKind("), 8)
                offsets = [body.index("TILE_DIR_" + name + "]") for name in directions]
                self.assertEqual(offsets, sorted(offsets))
                self.assertNotIn("#pragma", body)
                self.assertNotIn("g_rmgTerrainRules", body)

    def test_diagonal_shapes_keep_tables_clamps_and_predicates(self):
        module = generator("generate-rmg-diagonal-family.py")
        for which in ("First", "Second"):
            prefix = ("unsigned char rmgTerrainPainter::check" + which + "Diagonal(\n"
                      "    const TRmgGridPoint& point, const TRmgTerrainFlip& flip)\n{\n"
                      "    static TPoint offsets[4] = { TPoint(2, 2) };\n")
            original = prefix + "    int terrain = getTerrain(point);\n    return 0;\n}"
            for construction, second, dimensions in itertools.product(
                    ("direct", "copy_init", "reference", "staged"),
                    ("reuse", "direct", "staged", "reference"), ("members", "accessors")):
                if construction == "reference" and second == "reuse":
                    with self.assertRaises(ValueError):
                        module.body(original, which, construction, second, dimensions)
                    continue
                body = module.body(original, which, construction, second, dimensions)
                self.assertTrue(body.startswith(prefix))
                self.assertEqual(body.count("getTerrain("), 3)
                self.assertEqual(body.count("clampRmgTerrainCoordinate("), 4 if which == "First" else 2)
                self.assertEqual(body.count(("==" if which == "First" else "!=") + " terrain"), 2)
                self.assertNotIn("#pragma", body)
                self.assertNotIn("~TPoint", body)


if __name__ == "__main__":
    unittest.main()
