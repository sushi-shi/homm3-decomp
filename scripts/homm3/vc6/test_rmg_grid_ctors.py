"""Check mixed grid construction and its real unsigned value operations."""
import itertools
import json
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

from homm3.vc6 import _source, source_families
from homm3.vc6.test_rmg_families import generator


class RmgGridConstructorTests(unittest.TestCase):
    def setUp(self):
        self.root = Path(__file__).resolve().parents[3]
        self.module = generator("generate-rmg-grid-ctors-family.py")
        self.header = (self.root / self.module.HEADER).read_text()

    def rendered(self, expanded=False):
        payload = self.module.make_manifest(self.header, expanded)
        with tempfile.TemporaryDirectory(prefix="rmg-grid-ctors-manifest-") as raw:
            path = Path(raw) / "manifest.json"
            path.write_text(json.dumps(payload))
            _, originals, axes = source_families.load_manifest(path, self.root)
        self.assertEqual([len(axis.options) for axis in axes], [6, 5, 10 if expanded else 2])
        self.assertEqual(source_families.render(originals, axes, (0, 0, 0)), originals)
        return [source_families.render(originals, axes, choice)[self.module.HEADER]
                for choice in itertools.product(range(6), range(5), range(10 if expanded else 2))]

    def test_sixty_unique_headers_keep_interfaces_and_rebase(self):
        headers = self.rendered()
        self.assertEqual(len(set(headers)), 60)
        for header in headers:
            self.assertEqual(header.count("TRmgGridPoint(const TRmgGridPoint& other)"), 1)
            self.assertEqual(header.count("TRmgGridPoint(const unsigned int& newX, const unsigned int& newY)"), 1)
            self.assertNotIn("TRmgGridPoint& operator=(", header)
            payload = self.module.make_manifest(header)
            for axis in payload["axes"]:
                self.assertEqual(axis["find"], axis["options"][0]["replace"])
        original = self.module.make_manifest(self.header)["axes"][0]["find"]
        with self.assertRaisesRegex(ValueError, "review the current grid copy_constructor"):
            self.module.make_manifest(self.header.replace(original, original.replace("other.m_y", "other.m_x")))

    def test_six_constructor_parents_cross_ten_returns_and_rebase(self):
        parents = list(itertools.product(("member_initializers", "field_stores_yx", "assignment"),
                                         ("member_initializers", "field_stores_yx")))
        payload = self.module.make_parent_manifest(self.header, parents)
        with tempfile.TemporaryDirectory(prefix="rmg-grid-ctor-parents-") as raw:
            path = Path(raw) / "manifest.json"
            path.write_text(json.dumps(payload))
            _, originals, axes = source_families.load_manifest(path, self.root)
        self.assertEqual([len(axis.options) for axis in axes], [6, 10])
        self.assertEqual(source_families.render(originals, axes, (0, 0)), originals)
        seen = set()
        for choices in itertools.product(range(6), range(10)):
            header = source_families.render(originals, axes, choices)[self.module.HEADER]
            seen.add(header)
            for axis in self.module.make_parent_manifest(header, parents)["axes"]:
                self.assertEqual(axis["find"], axis["options"][0]["replace"])
        self.assertEqual(len(seen), 60)
        with self.assertRaisesRegex(ValueError, "review six unique constructor parents"):
            self.module.make_parent_manifest(self.header, parents[:-1])

    @unittest.skipUnless(shutil.which("g++"), "grid-constructor oracle needs g++")
    def test_coordinates_copy_assignment_and_translation_without_elision(self):
        headers = self.rendered(expanded=True)
        self.assertEqual(len(set(headers)), 300)
        self.assert_value_operations(headers)

    def member_order_headers(self):
        headers = self.rendered(expanded=True)
        parents = [(str(index), headers[index]) for index in (0, 1, 9, 50, 51, 89, 200, 205, 290, 299)]
        payload = self.module.make_member_order_manifest(self.header, parents)
        with tempfile.TemporaryDirectory(prefix="rmg-grid-member-order-") as raw:
            path = Path(raw) / "manifest.json"
            path.write_text(json.dumps(payload))
            _, originals, axes = source_families.load_manifest(path, self.root)
        self.assertEqual(len(axes), 1)
        self.assertEqual(len(axes[0].options), 60)
        self.assertEqual(source_families.render(originals, axes, (0,)), originals)
        return [source_families.render(originals, axes, (index,))[self.module.HEADER] for index in range(60)]

    def test_ten_parents_cross_six_member_orders_without_layout_or_body_edits(self):
        headers = self.member_order_headers()
        self.assertEqual(len(set(headers)), 60)
        for header in headers:
            point = self.module.grid_definition(header)
            self.assertLess(point.index("unsigned int m_x;"), point.index("unsigned int m_y;"))
            self.assertEqual(point.count("TRmgGridPoint(const TRmgGridPoint& other)"), 1)
            self.assertEqual(point.count("TRmgGridPoint(const unsigned int& newX, const unsigned int& newY)"), 1)
            self.assertEqual(point.count("TRmgGridPoint& operator+=(const TPoint& offset);"), 1)
            self.assertNotIn("TRmgGridPoint& operator=(", point)
        with self.assertRaisesRegex(ValueError, "ten distinct grid parents"):
            self.module.make_member_order_manifest(self.header, [("baseline", self.header)] * 10)

    @unittest.skipUnless(shutil.which("g++"), "grid-constructor oracle needs g++")
    def test_reordered_members_preserve_unsigned_values_and_lifetimes(self):
        self.assert_value_operations(self.member_order_headers())

    def assignment_headers(self):
        headers = self.member_order_headers()
        parents = [(str(index), headers[index]) for index in (0, 1, 6, 13, 20, 27, 34, 41, 48, 55)]
        payload = self.module.make_assignment_manifest(self.header, parents)
        with tempfile.TemporaryDirectory(prefix="rmg-grid-assignment-") as raw:
            path = Path(raw) / "manifest.json"
            path.write_text(json.dumps(payload))
            _, originals, axes = source_families.load_manifest(path, self.root)
        self.assertEqual(len(axes), 1)
        self.assertEqual(len(axes[0].options), 60)
        self.assertEqual(source_families.render(originals, axes, (0,)), originals)
        return [source_families.render(originals, axes, (index,))[self.module.HEADER] for index in range(60)]

    def test_sixty_assignment_states_keep_one_value_assignment_interface(self):
        headers = self.assignment_headers()
        self.assertEqual(len(set(headers)), 60)
        for index, header in enumerate(headers):
            point = self.module.grid_definition(header)
            self.assertEqual(point.count("TRmgGridPoint& operator=(const TRmgGridPoint& other)"),
                             0 if index % 6 == 0 else 1)
            self.assertEqual(point.count("TRmgGridPoint(const TRmgGridPoint& other)"), 1)
            self.assertEqual(point.count("TRmgGridPoint& operator+=(const TPoint& offset);"), 1)
        with self.assertRaisesRegex(ValueError, "ten distinct grid parents"):
            self.module.make_assignment_manifest(self.header, [("baseline", self.header)] * 10)

    @unittest.skipUnless(shutil.which("g++"), "grid-constructor oracle needs g++")
    def test_assignment_helpers_preserve_aliases_and_unsigned_values(self):
        self.assert_value_operations(self.assignment_headers())

    def proxy_manifest(self):
        headers = self.assignment_headers()
        parents = [(str(index), headers[index]) for index in (0, 1, 6, 13, 20, 27, 34, 41, 48, 55)]
        source = (self.root / "src/rmg_terrain.cpp").read_text()
        return self.module.make_proxy_manifest(self.header, source, parents)

    def test_ten_grid_parents_cross_six_proxy_member_constructions(self):
        with tempfile.TemporaryDirectory(prefix="rmg-grid-proxy-") as raw:
            path = Path(raw) / "manifest.json"
            path.write_text(json.dumps(self.proxy_manifest()))
            _, originals, axes = source_families.load_manifest(path, self.root)
        self.assertEqual([len(axis.options) for axis in axes], [10, 6])
        self.assertEqual(source_families.render(originals, axes, (0, 0)), originals)
        seen = set()
        proxy = generator("generate-rmg-line-refresh-family.py")
        for choices in itertools.product(range(10), range(6)):
            changed = source_families.render(originals, axes, choices)
            seen.add(tuple(sorted(changed.items())))
            self.assertEqual(proxy.constructor_parameter(changed[proxy.SOURCE]), "const TRmgGridPoint&")
            self.assertEqual(changed[proxy.SOURCE].count("TRmgLinePainterInterface::at("), 1)
        self.assertEqual(len(seen), 60)

    @unittest.skipUnless(shutil.which("g++"), "proxy/grid oracle needs g++")
    def test_grid_parents_and_proxy_construction_preserve_value_ownership(self):
        from homm3.vc6.test_rmg_families import RmgSourceFamilyTests
        RmgSourceFamilyTests().check_line_proxy_cpp(
            "generate-rmg-line-refresh-family.py", axes=self.proxy_manifest()["axes"])

    def temporary_manifest(self):
        with tempfile.TemporaryDirectory(prefix="rmg-grid-proxy-parents-") as raw:
            path = Path(raw) / "manifest.json"
            path.write_text(json.dumps(self.proxy_manifest()))
            _, originals, axes = source_families.load_manifest(path, self.root)
        choices = list(itertools.product(range(10), range(6)))
        parents = [(str(index), source_families.render(originals, axes, choices[index]))
                   for index in (0, 1, 6, 13, 20, 27, 34, 41, 48, 55)]
        return self.module.make_temporary_manifest(originals, parents)

    def test_temporary_followup_carries_both_header_and_proxy_parents(self):
        with tempfile.TemporaryDirectory(prefix="rmg-grid-temporaries-") as raw:
            path = Path(raw) / "manifest.json"
            path.write_text(json.dumps(self.temporary_manifest()))
            _, originals, axes = source_families.load_manifest(path, self.root)
        self.assertEqual([len(axis.options) for axis in axes], [60])
        self.assertEqual(source_families.render(originals, axes, (0,)), originals)
        states = [source_families.render(originals, axes, (index,)) for index in range(60)]
        self.assertEqual(len({tuple(sorted(state.items())) for state in states}), 60)
        for state in states:
            point = self.module.grid_definition(state[self.module.HEADER])
            self.assertEqual(point.count(" += offset"), 1)
            self.assertEqual(state["src/rmg_terrain.cpp"].count("TRmgLinePainterInterface::at("), 1)

    @unittest.skipUnless(shutil.which("g++"), "proxy/grid oracle needs g++")
    def test_returned_temporaries_preserve_copied_proxy_coordinates(self):
        from homm3.vc6.test_rmg_families import RmgSourceFamilyTests
        RmgSourceFamilyTests().check_line_proxy_cpp(
            "generate-rmg-line-refresh-family.py", axes=self.temporary_manifest()["axes"])

    def visibility_manifest(self):
        with tempfile.TemporaryDirectory(prefix="rmg-grid-visibility-parents-") as raw:
            path = Path(raw) / "manifest.json"
            path.write_text(json.dumps(self.temporary_manifest()))
            _, originals, axes = source_families.load_manifest(path, self.root)
        parents = [(str(index), source_families.render(originals, axes, (index,)))
                   for index in (0, 1, 6, 13, 20, 27, 34, 41, 48, 55)]
        return self.module.make_visibility_manifest(originals, parents)

    def test_visibility_states_keep_exactly_one_canonical_translation_body(self):
        with tempfile.TemporaryDirectory(prefix="rmg-grid-visibility-") as raw:
            path = Path(raw) / "manifest.json"
            path.write_text(json.dumps(self.visibility_manifest()))
            _, originals, axes = source_families.load_manifest(path, self.root)
        self.assertEqual([len(axis.options) for axis in axes], [60])
        self.assertEqual(source_families.render(originals, axes, (0,)), originals)
        states = [source_families.render(originals, axes, (index,)) for index in range(60)]
        self.assertEqual(len({tuple(sorted(state.items())) for state in states}), 60)
        signature = "TRmgGridPoint operator+(const TPoint& offset) const"
        for state in states:
            point = self.module.grid_definition(state[self.module.HEADER])
            source = state["src/rmg_terrain.cpp"]
            self.assertEqual(point.count(signature + "\n    {")
                             + source.count("TRmgGridPoint TRmgGridPoint::operator+("), 1)
            self.assertEqual(source.count("TRmgGridPoint& TRmgGridPoint::operator+=("), 1)
            self.assertEqual(point.count("TRmgGridPoint(const TRmgGridPoint& other)"), 1)
            self.assertNotIn("inline TRmgGridPoint", source + point)

    @unittest.skipUnless(shutil.which("g++"), "proxy/grid oracle needs g++")
    def test_ordinary_translation_visibility_preserves_proxy_and_value_semantics(self):
        from homm3.vc6.test_rmg_families import RmgSourceFamilyTests
        RmgSourceFamilyTests().check_line_proxy_cpp(
            "generate-rmg-line-refresh-family.py", axes=self.visibility_manifest()["axes"])

    def binding_manifest(self):
        with tempfile.TemporaryDirectory(prefix="rmg-grid-binding-parents-") as raw:
            path = Path(raw) / "manifest.json"
            path.write_text(json.dumps(self.temporary_manifest()))
            _, originals, axes = source_families.load_manifest(path, self.root)
        parents = [(str(index), source_families.render(originals, axes, (index,)))
                   for index in (0, 7, 14, 21, 28)]
        return self.module.make_binding_manifest(originals, parents)

    def test_binding_states_change_only_the_expanded_constructor_parameter(self):
        with tempfile.TemporaryDirectory(prefix="rmg-grid-binding-") as raw:
            path = Path(raw) / "manifest.json"
            path.write_text(json.dumps(self.binding_manifest()))
            _, originals, axes = source_families.load_manifest(path, self.root)
        self.assertEqual([len(axis.options) for axis in axes], [60])
        self.assertEqual(source_families.render(originals, axes, (0,)), originals)
        proxy = self.module.proxy_module()
        states = [source_families.render(originals, axes, (index,)) for index in range(60)]
        self.assertEqual(len({tuple(sorted(state.items())) for state in states}), 60)
        for state in states:
            header, source = state[self.module.HEADER], state["src/rmg_terrain.cpp"]
            self.assertEqual(header.count("TRmgLinePainterTile at(const TRmgGridPoint& point);"), 1)
            self.assertEqual(source.count("TRmgLinePainterInterface::at(const TRmgGridPoint& point)"), 1)
            self.assertEqual(header.count(proxy.constructor_declaration(source)), 1)

    @unittest.skipUnless(shutil.which("g++"), "proxy/grid oracle needs g++")
    def test_value_or_reference_constructor_keeps_proxy_coordinate_ownership(self):
        from homm3.vc6.test_rmg_families import RmgSourceFamilyTests
        RmgSourceFamilyTests().check_line_proxy_cpp(
            "generate-rmg-line-proxy-binding-family.py", axes=self.binding_manifest()["axes"])

    def factory_manifest(self):
        with tempfile.TemporaryDirectory(prefix="rmg-grid-factory-parents-") as raw:
            path = Path(raw) / "manifest.json"
            path.write_text(json.dumps(self.binding_manifest()))
            _, originals, axes = source_families.load_manifest(path, self.root)
        parents = [(str(index), source_families.render(originals, axes, (index,)))
                   for index in (0, 1, 6, 13, 20, 27, 34, 41, 48, 55)]
        return self.module.make_factory_manifest(originals, parents)

    def test_factory_return_lifetimes_keep_both_caller_expressions_and_value_abi(self):
        with tempfile.TemporaryDirectory(prefix="rmg-grid-factory-") as raw:
            path = Path(raw) / "manifest.json"
            path.write_text(json.dumps(self.factory_manifest()))
            _, originals, axes = source_families.load_manifest(path, self.root)
        self.assertEqual([len(axis.options) for axis in axes], [60])
        self.assertEqual(source_families.render(originals, axes, (0,)), originals)
        proxy = self.module.proxy_module()
        states = [source_families.render(originals, axes, (index,)) for index in range(60)]
        self.assertEqual(len({tuple(sorted(state.items())) for state in states}), 60)
        refresh = proxy.helpers().definition(originals["src/rmg_terrain.cpp"], "refreshRmgLinePoint")
        for state in states:
            header, source = state[self.module.HEADER], state["src/rmg_terrain.cpp"]
            self.assertEqual(proxy.helpers().definition(source, "refreshRmgLinePoint"), refresh)
            self.assertEqual(header.count("TRmgLinePainterTile at(const TRmgGridPoint& point);"), 1)
            self.assertEqual(source.count("TRmgLinePainterTile TRmgLinePainterInterface::at("), 1)

    @unittest.skipUnless(shutil.which("g++"), "proxy/grid oracle needs g++")
    def test_factory_returns_copy_before_reference_bound_temporary_dies(self):
        from homm3.vc6.test_rmg_families import RmgSourceFamilyTests
        RmgSourceFamilyTests().check_line_proxy_cpp(
            "generate-rmg-line-proxy-binding-family.py", axes=self.factory_manifest()["axes"])

    def assert_value_operations(self, headers):
        source = (self.root / "src/rmg_terrain.cpp").read_text()
        found = _source.find_definitions(source, "TRmgGridPoint::operator+=")
        self.assertEqual(len(found), 1)
        item = found[0]
        compound = source[source.rfind("\n", 0, item.head) + 1:item.body_close + 1]
        program = []
        for index, header in enumerate(headers):
            start = header.index("struct TRmgGridPoint {")
            point = header[start:header.index("\n};", start) + 3]
            program += [f"namespace Case{index} {{\nstruct TPoint {{ int m_x, m_y; }};\n",
                        point, "\n", compound, r"""
TRmgGridPoint copied(TRmgGridPoint value) { return value; }
int check() {
    const unsigned int values[] = {0, 1, 0x7fffffffU, 0x80000000U, 0xffffffffU};
    const int offsets[] = {0, 1, -1, 127, (-2147483647 - 1)};
    for (unsigned int x = 0; x != 5; ++x)
    for (unsigned int y = 0; y != 5; ++y) {
        unsigned int inputX = values[x], inputY = values[y];
        TRmgGridPoint original(inputX, inputY);
        TRmgGridPoint alias(inputX, inputX);
        if (alias.m_x != inputX || alias.m_y != inputX) return 1;
        TRmgGridPoint copy(original);
        TRmgGridPoint returned = copied(copy);
        TRmgGridPoint assigned;
        assigned = returned;
        TRmgGridPoint& assignmentResult = (assigned = assigned);
        if (&assignmentResult != &assigned) return 7;
        if (original.m_x != inputX || original.m_y != inputY
            || copy.m_x != inputX || copy.m_y != inputY
            || returned.m_x != inputX || returned.m_y != inputY
            || assigned.m_x != inputX || assigned.m_y != inputY) return 2;
        ++assigned.m_x;
        if (original.m_x != inputX || copy.m_x != inputX || returned.m_x != inputX) return 3;
        for (unsigned int dx = 0; dx != 5; ++dx)
        for (unsigned int dy = 0; dy != 5; ++dy) {
            TPoint offset = {offsets[dx], offsets[dy]};
            TRmgGridPoint translated = original + offset;
            unsigned int expectedX = inputX + static_cast<unsigned int>(offsets[dx]);
            unsigned int expectedY = inputY + static_cast<unsigned int>(offsets[dy]);
            if (translated.m_x != expectedX || translated.m_y != expectedY) return 4;
            TRmgGridPoint compoundPoint = original;
            TRmgGridPoint& result = (compoundPoint += offset);
            if (&result != &compoundPoint || result.m_x != expectedX || result.m_y != expectedY) return 5;
            if (original.m_x != inputX || original.m_y != inputY) return 6;
        }
    }
    return 0;
}
}
"""]
        program += ["int main() {\n"]
        program += [f"if (Case{index}::check()) return {index + 1};\n" for index in range(len(headers))]
        program += ["return 0;\n}\n"]
        with tempfile.TemporaryDirectory(prefix="rmg-grid-ctors-oracle-") as raw:
            path = Path(raw)
            cpp, executable = path / "grid.cpp", path / "grid"
            cpp.write_text("".join(program))
            result = subprocess.run([shutil.which("g++"), "-std=c++98", "-fno-elide-constructors",
                                     str(cpp), "-o", str(executable)], capture_output=True, text=True, timeout=60)
            self.assertEqual(result.returncode, 0, result.stderr)
            result = subprocess.run([str(executable)], capture_output=True, text=True, timeout=60)
            self.assertEqual(result.returncode, 0, result.stderr)


if __name__ == "__main__":
    unittest.main()
