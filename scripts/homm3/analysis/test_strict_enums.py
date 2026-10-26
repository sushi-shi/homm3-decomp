"""Strict enum audit planning and counting; no compiler binaries required."""
from pathlib import Path
import tempfile
import textwrap
import unittest

from homm3.analysis.strict_enums import (REFERENCE_OPTION, STRICT_OPTIONS, domain_sites, plan,
                                         run, summarize)


def project(root: Path) -> None:
    (root / "config").mkdir()
    (root / "include").mkdir()
    (root / "config/project.toml").write_text(textwrap.dedent("""
        [inputs.retail]
        name = "game"
        [inputs.editor]
        image = true
    """))
    flags = textwrap.dedent("""
        [build]
        includes = ["include"]
        [flags]
        cpp = ["/c", "/O2", "/DNDEBUG"]
        other = ["/c", "/O1", "/DNDEBUG", "/DEDITOR"]
        c = ["/c", "/TC"]
    """)
    (root / "config/units.toml").write_text(flags + textwrap.dedent("""
        [[unit]]
        unit = "army"
        source = "src/army.cpp"
        flags = "cpp"
        [[unit]]
        unit = "zlib"
        source = "vendor/zlib.c"
        flags = "c"
        [[unit]]
        unit = "font"
        source = "src/font.cpp"
        flags = "cpp"
    """))
    (root / "config/units.editor.toml").write_text(flags + textwrap.dedent("""
        [[unit]]
        unit = "font"
        source = "src/font.cpp"
        flags = "cpp"
        [[unit]]
        unit = "army"
        source = "src/army.cpp"
        flags = "other"
    """))


class StrictEnumTests(unittest.TestCase):
    def test_plan_skips_c_and_shares_identical_profiles(self):
        # Code-generation switches do not reach Clang; only a define splits.
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            project(root)
            jobs = plan(root, ["game", "editor"], None, "clang", None, False)
        self.assertEqual([(j["source"], j["owners"]) for j in jobs], [
            ("src/army.cpp", ["game:army"]),
            ("src/army.cpp", ["editor:army"]),
            ("src/font.cpp", ["game:font", "editor:font"]),
        ])
        command = jobs[0]["command"]
        self.assertNotIn("/c", command)
        self.assertEqual(command[1:1 + len(STRICT_OPTIONS)], STRICT_OPTIONS)
        self.assertNotIn(REFERENCE_OPTION, command)

    def test_reference_and_unit_selection(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            project(root)
            jobs = plan(root, ["editor"], {"font"}, "clang", None, True)
        self.assertEqual([j["owners"] for j in jobs], [["editor:font"]])
        self.assertIn(REFERENCE_OPTION, jobs[0]["command"])

    def test_errors_count_per_parse_and_dedupe_across_owners(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            log = (f"{root}/include/a.h:3:5: error: cannot initialize 'Domain' with 'int'\n"
                   f"{root}/src/a.cpp:9:1: warning: unused [-Wunused]\n"
                   f"{root}/src/a.cpp:10:2: error: invalid operands\n"
                   "2 errors generated.\n")
            job = dict(source="src/a.cpp", owners=["game:a"],
                       command=["sh", "-c", f"printf '%s' \"$0\" >&2; exit 1", log])
            first = run(job, root, 30)
            second = run(dict(job, owners=["editor:a"]), root, 30)
        self.assertEqual(first["status"], "error")
        self.assertEqual([(e["path"], e["line"]) for e in first["errors"]],
                         [("include/a.h", 3), ("src/a.cpp", 10)])
        summary = summarize([first, second])
        self.assertEqual(summary["unique"], 2)
        self.assertEqual(summary["sites"][("include/a.h", 3, 5,
                                           "cannot initialize 'Domain' with 'int'")],
                         {"game:a", "editor:a"})

    def test_delta_keeps_only_sites_the_reference_lacks(self):
        shared = ("src/a.cpp", 1, 1, "call to 'min' is ambiguous")
        domain = ("src/a.cpp", 2, 5, "invalid operands to binary expression")
        strict = dict(sites={shared: {"game:a"}, domain: {"game:a", "editor:a"}})
        reference = dict(sites={shared: {"game:a"}})
        self.assertEqual(domain_sites(strict, reference), {domain: {"game:a", "editor:a"}})

    def test_failed_parse_without_diagnostics_is_not_clean(self):
        with tempfile.TemporaryDirectory() as directory:
            job = dict(source="src/a.cpp", owners=["game:a"], command=["false"])
            result = run(job, Path(directory), 30)
        self.assertEqual(result["status"], "error")
        self.assertEqual(result["errors"], [])


if __name__ == "__main__":
    unittest.main()
