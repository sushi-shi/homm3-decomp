"""Portable behavior checks for real line painting source alternatives."""
from pathlib import Path
import itertools
import json
import shutil
import subprocess
import tempfile
import unittest

from homm3.vc6.test_rmg_families import generator
from homm3.vc6 import source_families


class LinePaintingTests(unittest.TestCase):
    def setUp(self):
        self.root = Path(__file__).resolve().parents[3]
        self.module = generator("generate-rmg-line-entry-family.py")
        self.header = (self.root / self.module.HEADER).read_text()
        self.source = (self.root / self.module.SOURCE).read_text()
        self.axes = self.module.make_axes(self.header, self.source)

    def test_entry_family_rebases_real_helpers_and_asymmetric_borders(self):
        self.assertEqual([len(self.axes[i]["options"]) for i in (0, 2, 3)], [6, 2, 4])
        self.assertIn(len(self.axes[1]["options"]), (8, 9))
        for axis in self.axes:
            self.assertEqual(self.source.count(axis["find"]), 1)
            self.assertEqual(axis["find"], axis["options"][0]["replace"])
            for option in axis["options"]:
                self.assertNotIn("#pragma", option["replace"])
                # Comments can discuss expansion; no declaration adds inline.
                self.assertNotIn("inline ", option["replace"])
                changed = self.source.replace(axis["find"], option["replace"])
                rebased = self.module.make_axes(self.header, changed)
                self.assertEqual([len(rebased[i]["options"]) for i in (0, 2, 3)], [6, 2, 4])
                self.assertIn(len(rebased[1]["options"]), (8, 9))
                clear = self.module.parent().helpers().definition(changed, "clearRmgLineRectangle")
                self.assertIn("rectangle.m_size.m_y < painter->m_size.m_y - 1", clear)
                self.assertEqual(clear.count("refreshRmgLinePoint(painter, point);"), 4)
                refresh = self.module.parent().helpers().definition(changed, "refreshRmgLinePoint")
                self.assertEqual(refresh.count("painter->at("), 2)

    @unittest.skipUnless(shutil.which("g++"), "portable painting oracle needs g++")
    def test_generated_entries_preserve_all_calls_border_visits_and_tile_writes(self):
        self.check_source_cases(self.source_cases(self.axes))

    def test_neighbour_family_rebases_without_changing_the_second_pass(self):
        module = generator("generate-rmg-line-neighbour-family.py")
        axes = module.make_axes(self.header, self.source)
        self.assertEqual(len(axes[0]["options"]), 12)
        self.assertIn(len(axes[1]["options"]), (24, 25))
        helper = self.module.parent().helpers()
        point = helper.definition(self.source, "TRmgLineWalker::paintPoint")
        loop = "    for (direction = 0; direction < TILE_DIR_COUNT; ++direction) {\n"
        prefix = point[:point.index(loop)]
        suffix = point[point.rindex(loop):]
        for axis in axes:
            self.assertEqual(self.source.count(axis["find"]), 1)
            self.assertEqual(axis["find"], axis["options"][0]["replace"])
            for option in axis["options"]:
                changed = self.source.replace(axis["find"], option["replace"])
                self.assertNotIn("#pragma", option["replace"])
                self.assertNotIn("inline ", option["replace"])
                rebased = module.make_axes(self.header, changed)
                self.assertEqual(len(rebased[0]["options"]), 12)
                self.assertIn(len(rebased[1]["options"]), (24, 25))
                clear = helper.definition(changed, "clearRmgLineRectangle")
                self.assertIn("rectangle.m_size.m_y < painter->m_size.m_y - 1", clear)
                self.assertEqual(clear.count("refreshRmgLinePoint(painter, point);"), 4)
                changed_point = helper.definition(changed, "TRmgLineWalker::paintPoint")
                self.assertTrue(changed_point.startswith(prefix))
                self.assertTrue(changed_point.endswith(suffix))

    def test_neighbour_family_rejects_unreviewed_snapshot_changes(self):
        module = generator("generate-rmg-line-neighbour-family.py")
        point = self.module.parent().helpers().definition(self.source, "TRmgLineWalker::paintPoint")
        changed = self.source.replace(point, point.replace("matches[direction] = 0;", "matches[direction] = 1;"))
        with self.assertRaisesRegex(ValueError, "snapshot loop"):
            module.make_axes(self.header, changed)

    @unittest.skipUnless(shutil.which("g++"), "portable painting oracle needs g++")
    def test_generated_neighbours_preserve_all_calls_border_visits_and_tile_writes(self):
        module = generator("generate-rmg-line-neighbour-family.py")
        self.check_source_cases(self.source_cases(module.make_axes(self.header, self.source)))

    def test_query_family_rebases_only_the_canonical_helpers_and_border_queries(self):
        module = generator("generate-rmg-line-query-family.py")
        axes = module.make_axes(self.header, self.source)
        self.assertEqual([len(axis["options"]) for axis in axes], [7, 6, 4])
        helper = self.module.parent().helpers()
        original_point = helper.definition(self.source, "TRmgLineWalker::paintPoint")
        for axis in axes:
            self.assertEqual(self.source.count(axis["find"]), 1)
            self.assertEqual(axis["find"], axis["options"][0]["replace"])
            for option in axis["options"]:
                self.assertNotIn("#pragma", option["replace"])
                self.assertNotIn("inline ", option["replace"])
                changed = self.source.replace(axis["find"], option["replace"])
                rebased = module.make_axes(self.header, changed)
                self.assertEqual([len(item["options"]) for item in rebased], [7, 6, 4])
                self.assertEqual(helper.definition(changed, "TRmgLineWalker::paintPoint"), original_point)

    @unittest.skipUnless(shutil.which("g++"), "portable painting oracle needs g++")
    def test_generated_queries_preserve_all_calls_border_visits_and_tile_writes(self):
        module = generator("generate-rmg-line-query-family.py")
        self.check_source_cases(self.source_cases(module.make_axes(self.header, self.source)))

    def boundary_family(self):
        module = generator("generate-rmg-grid-add-boundary-family.py")
        raw = module.make_axes(self.header, self.source)
        with tempfile.TemporaryDirectory(prefix="rmg-grid-add-manifest-") as directory:
            path = Path(directory) / "family.json"
            path.write_text(json.dumps(dict(schema=1, axes=raw)))
            _, originals, axes = source_families.load_manifest(path, self.root)
        return module, originals, axes

    def test_grid_add_boundary_keeps_one_definition_and_rebases_atomically(self):
        module, originals, axes = self.boundary_family()
        self.assertEqual([len(axis.options) for axis in axes], [3, 8, 7])
        for choices in itertools.product(*(range(len(axis.options)) for axis in axes)):
            texts = source_families.render(originals, axes, choices)
            header, source = texts[module.HEADER], texts[module.SOURCE]
            self.assertEqual(header.count(module.CLASS_BODY) + source.count(module.TU_BODY), 1)
            self.assertEqual(header.count(module.DECLARATION), source.count(module.TU_BODY))
            rebased = module.make_axes(header, source)
            self.assertEqual(len(rebased[0]["options"]), 3)
            for axis in rebased:
                self.assertEqual(axis["find"], axis["options"][0]["replace"])
            # No pragma or declaration noise is introduced at either boundary.
            self.assertEqual(source.count("#pragma"), self.source.count("#pragma"))
            self.assertEqual(header.count("#pragma"), self.header.count("#pragma"))

    @unittest.skipUnless(shutil.which("g++"), "portable painting oracle needs g++")
    def test_grid_add_definition_boundaries_preserve_real_painting_behavior(self):
        module, originals, axes = self.boundary_family()
        # Each placement crossed with every translation and every caller form,
        # plus both nontrivial placements at the opposite lifetime corner.
        choices = {(placement, translation, 0)
                   for placement in range(3) for translation in range(8)}
        choices.update((placement, 0, neighbour)
                       for placement in range(3) for neighbour in range(7))
        choices.update((placement, 7, 6) for placement in range(3))
        cases = []
        for choice in sorted(choices):
            texts = source_families.render(originals, axes, choice)
            cases.append((texts[module.HEADER], texts[module.SOURCE]))
        self.check_source_cases(cases)

    def source_cases(self, axes):
        # Every authored axis option plus their opposite-corner composition.
        # Copy/member ownership is also covered by the complete proxy products.
        cases = [self.source]
        corner = self.source
        for axis in axes:
            for option in axis["options"][1:]:
                cases.append(self.source.replace(axis["find"], option["replace"]))
            corner = corner.replace(axis["find"], axis["options"][-1]["replace"])
        return list(dict.fromkeys([*cases, corner]))

    def check_source_cases(self, cases):
        helper = self.module.parent().helpers()

        def declaration(text, opening):
            start = text.index(opening)
            return text[start:text.index("\n};", start) + 3]

        terrain = (self.root / "include/rmg_terrain.h").read_text()
        model_start = self.header.index("class TRmgLinePainterInterface {")
        model = self.header[model_start:self.header.index("SIZE(TRmgLinePainterTile,", model_start)]
        interface_ctor = helper.definition((self.root / "src/rmg_support.cpp").read_text(),
                                          "TRmgLinePainterInterface::TRmgLinePainterInterface")
        mask = helper.definition((self.root / "src/tiles.cpp").read_text(), "buildTileNeighbourMask")
        oracle = (self.root / "scripts/experiments/rmg-line-painting-oracle.cpp").read_text()
        program = ["#include <vector>\n#include <algorithm>\n"]
        for index, case in enumerate(cases):
            header, source = case if isinstance(case, tuple) else (self.header, case)
            program.extend([f"namespace Case{index} {{\n",
                            "struct TPoint { int m_x, m_y; TPoint(int x, int y) : m_x(x), m_y(y) {} };\n",
                            declaration(header, "struct TRmgGridPoint {"), "\n",
                            declaration(terrain, "struct rmgTerrainTile {"), "\n",
                            "struct TRmgLinePatternTable {};\nstruct TRmgLinePainterTile;\n",
                            model, "\n", declaration(self.header, "struct TRmgGridRectangle {"), "\n",
                            declaration(self.header, "class TRmgLineWalker {"), "\n",
                            "void refreshRmgLinePoint(TRmgLinePainterInterface*, const TRmgGridPoint&);\n",
                            "enum { TILE_DIR_NORTH, TILE_DIR_NORTHEAST, TILE_DIR_EAST, TILE_DIR_SOUTHEAST,\n"
                            "TILE_DIR_SOUTH, TILE_DIR_SOUTHWEST, TILE_DIR_WEST, TILE_DIR_NORTHWEST, TILE_DIR_COUNT };\n",
                            "const TPoint g_tileDirections[8] = { TPoint(0,-1), TPoint(1,-1), TPoint(1,0), TPoint(1,1),\n"
                            "TPoint(0,1), TPoint(-1,1), TPoint(-1,0), TPoint(-1,-1) };\n",
                            mask.replace("__fastcall ", ""), "\n", interface_ctor, "\n",
                            self.module.parent().constructor_definition(source), "\n",
                            self.module.parent().constructor_definition(source, copy=True, required=False), "\n"])
            if "TRmgGridPoint::operator+=(" in source:
                program.extend([helper.definition(source, "TRmgGridPoint::operator+="), "\n"])
            for name in ("TRmgLinePainterTile::getLand", "TRmgLinePainterTile::setTile",
                         "TRmgLinePainterTile::isBlocked", "TRmgLinePainterTile::setOverlay",
                         "TRmgLinePainterInterface::at", "TRmgGridRectangle::TRmgGridRectangle",
                         "clearRmgLineRectangle", "TRmgLineWalker::TRmgLineWalker", "TRmgLineWalker::paintPoint"):
                program.extend([helper.definition(source, name), "\n"])
            program.extend([oracle, "\n}\n"])
        program.append("int main() {\n")
        for index in range(len(cases)):
            program.append(f"if (Case{index}::check()) return {index + 1};\n")
        program.append("return 0;\n}\n")
        with tempfile.TemporaryDirectory(prefix="rmg-line-painting-test-") as directory:
            cpp, executable = Path(directory) / "painting.cpp", Path(directory) / "painting"
            cpp.write_text("".join(program))
            built = subprocess.run([shutil.which("g++"), "-std=c++98", "-fno-elide-constructors",
                                    str(cpp), "-o", str(executable)],
                                   capture_output=True, text=True, timeout=60)
            self.assertEqual(built.returncode, 0, built.stderr)
            checked = subprocess.run([str(executable)], capture_output=True, text=True, timeout=30)
            self.assertEqual(checked.returncode, 0, f"source case {checked.returncode}: {checked.stderr}")


if __name__ == "__main__":
    unittest.main()
