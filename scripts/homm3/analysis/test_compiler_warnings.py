"""Diagnostic parsing and coverage controls; no compiler binaries required."""
import json
import os
from pathlib import Path
import sys
import tempfile
import unittest

from homm3.analysis.compiler_warnings import aggregate, parse_diagnostics, run_job, write_report


class CompilerWarningTests(unittest.TestCase):
    def test_real_compiler_formats_and_notes(self):
        log = """In file included from /repo/src/a.cpp:1:
/repo/include/a.h:20:9: warning: unused parameter 'value' [-Wunused-parameter]
/repo/src/a.cpp:5:2: note: in instantiation requested here
Z:\\repo\\src\\a.cpp(47) : warning C4701: local variable 'x' may be used without having been initialized
Z:\\repo\\src\\a.cpp(62) : error C2065: 'missing' : undeclared identifier
clang: warning: unknown argument ignored in clang-cl: '-bad' [-Wunknown-argument]
3 warnings generated.
"""
        rows, unparsed = parse_diagnostics(log)
        self.assertEqual(unparsed, [])
        self.assertEqual(len(rows), 4)
        self.assertEqual([r["code"] for r in rows],
                         ["-Wunused-parameter", "C4701", "C2065", "-Wunknown-argument"])
        self.assertEqual(rows[1]["line"], 47)
        self.assertIsNone(rows[1]["column"])
        self.assertEqual(rows[2]["severity"], "error")
        self.assertEqual(rows[0]["message"], "unused parameter 'value'")

    def test_header_dedup_keeps_distinct_instantiations_and_compilers(self):
        first, _ = parse_diagnostics("/repo/include/a.h:20:9: warning: unused 'int' [-Wunused]\n")
        second, _ = parse_diagnostics("/repo/include/a.h:20:9: warning: unused 'long' [-Wunused]\n")
        jobs = [dict(compiler="clang", unit="a", diagnostics=first),
                dict(compiler="clang", unit="b", diagnostics=first + second),
                dict(compiler="msvc", unit="a", diagnostics=first)]
        result = aggregate(jobs, Path("/repo"), Path("/sdk"), Path("/mirror"))
        self.assertEqual(len(result), 3)
        row = next(r for r in result if r["compiler"] == "clang" and "'int'" in r["message"])
        self.assertEqual(row["occurrences"], 2)
        self.assertEqual(row["units"], ["a", "b"])
        self.assertEqual(row["origin"], "project-header")

    def test_windows_and_mirror_ownership_remain_separate(self):
        rows, _ = parse_diagnostics("""Z:\\repo\\include\\a.h(20) : warning C4100: unused
Z:\\sdk\\INCLUDE\\VECTOR(10) : warning C4100: unused
/repo/build/gen/msvc-include/vector:10:2: warning: unused [-Wunused]
""")
        result = aggregate([dict(compiler="test", unit="a", diagnostics=rows)],
                           Path("/repo"), Path("/sdk"), Path("/repo/build/gen/msvc-include"))
        self.assertEqual({(r["path"], r["origin"]) for r in result},
                         {("include/a.h", "project-header"),
                          ("<msvc>/include/vector", "sdk"), ("<msvc-mirror>/vector", "sdk")})

    def test_unknown_diagnostic_is_not_silently_dropped(self):
        _, unparsed = parse_diagnostics("unexpected-format warning: important diagnostic\n")
        self.assertEqual(unparsed, ["unexpected-format warning: important diagnostic"])

    def test_locationless_vc6_diagnostics_keep_their_tu_identity(self):
        rows, unparsed = parse_diagnostics("warning C4711: function '$E50' selected for automatic inline expansion\n")
        self.assertEqual(unparsed, [])
        self.assertEqual(rows[0]["code"], "C4711")
        result = aggregate([dict(compiler="msvc", unit=u, diagnostics=rows) for u in ["a", "b"]],
                           Path("/repo"), Path("/sdk"), Path("/mirror"))
        self.assertEqual(len(result), 2)
        self.assertTrue(all(r["origin"] == "unlocated" for r in result))

    def test_report_keeps_failed_zero_diagnostic_tu_in_denominator(self):
        with tempfile.TemporaryDirectory() as directory:
            out = Path(directory)
            metadata = dict(revision="abc", started_at="now", inputs_unchanged=True,
                            compilers={"clang": dict(version="test", mode="test")},
                            non_manifest_sources=[], selected_units=["a", "b"], manifest_total=3)
            jobs = [dict(compiler="clang", unit="a", status="complete", diagnostics=[], log="a.log"),
                    dict(compiler="clang", unit="b", status="timeout", diagnostics=[], log="b.log")]
            write_report(out, metadata, jobs, [])
            report = (out / "report.md").read_text()
            self.assertIn("| clang | 2 | 1 | 0 | 0 | 0 |", report)
            self.assertIn("Selected **2/3** manifest TUs.", report)
            self.assertIn("| clang | b | timeout |", report)
            self.assertEqual(len(json.loads((out / "jobs.json").read_text())), 2)

    def test_compiler_error_overrides_zero_exit_and_produced_object(self):
        with tempfile.TemporaryDirectory() as directory:
            command = [sys.executable, "-c", "import pathlib, sys; "
                       "pathlib.Path('a.obj').write_bytes(b'object'); "
                       "print('a.cpp(1) : warning C4701: uninitialized'); "
                       "print('a.cpp(2) : error C2065: undeclared', file=sys.stderr)"]
            result = run_job(dict(compiler="msvc", unit="a", source="a.cpp", command=command,
                                  directory=directory, env=dict(os.environ),
                                  log_path=str(Path(directory) / "a.log"),
                                  object=str(Path(directory) / "a.obj")), timeout=5)
            self.assertEqual(result["returncode"], 0)
            self.assertTrue(result["object_produced"])
            self.assertEqual(result["status"], "compile-error")
            self.assertEqual({r["code"] for r in result["diagnostics"]}, {"C4701", "C2065"})

    def test_timeout_cannot_pass_because_an_object_exists(self):
        with tempfile.TemporaryDirectory() as directory:
            obj = Path(directory) / "a.obj"
            obj.write_bytes(b"partial object")
            result = run_job(dict(compiler="msvc", unit="a", source="a.cpp",
                                  command=[sys.executable, "-c", "import time; time.sleep(30)"],
                                  directory=directory, env=dict(os.environ),
                                  log_path=str(Path(directory) / "a.log"), object=str(obj)), timeout=0.1)
            self.assertTrue(result["object_produced"])
            self.assertEqual(result["status"], "timeout")

    def test_zero_exit_without_an_object_is_incomplete(self):
        with tempfile.TemporaryDirectory() as directory:
            result = run_job(dict(compiler="msvc", unit="a", source="a.cpp",
                                  command=[sys.executable, "-c", "pass"], directory=directory,
                                  env=dict(os.environ), log_path=str(Path(directory) / "a.log"),
                                  object=str(Path(directory) / "a.obj")), timeout=5)
            self.assertEqual(result["returncode"], 0)
            self.assertEqual(result["status"], "missing-object")


if __name__ == "__main__":
    unittest.main()
