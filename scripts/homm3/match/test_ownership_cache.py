"""Cache reuse must retain the ownership gate's complete input coverage."""
from pathlib import Path
import json
import tempfile
import unittest
from unittest.mock import patch

from homm3.match import source_ownership as ownership


class OwnershipCacheTest(unittest.TestCase):
    def setUp(self):
        self.real_scan = ownership.scan_unit
        self.root = Path(self.enterContext(tempfile.TemporaryDirectory()))
        for relative in ("src/a.cpp", "src/b.cpp", "include/header.h", "config/units.toml",
                         "scripts/homm3/core/clang.py", "scripts/homm3/vc6/_source.py",
                         "scripts/homm3/manifest.py", "scripts/homm3/retail_labels/source.py"):
            path = self.root / relative
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_text("// fixture\n")
        self.enterContext(patch.object(ownership.manifest, "units", return_value=[
            {"source": "src/a.cpp"}, {"source": "src/b.cpp"}]))
        self.enterContext(patch.object(ownership.clang, "mirror", return_value=None))
        self.includes = {"src/a.cpp": [], "src/b.cpp": []}
        self.failures = []
        def scan(unit, root):
            source = unit["source"]
            return [], list(self.failures), [source, "include/header.h", *self.includes[source]]
        self.scan = self.enterContext(patch.object(ownership, "scan_unit", side_effect=scan))

    def collect(self, **kwargs):
        result = ownership.collect(self.root, jobs=1, **kwargs)
        scanned = [call.args[0]["source"] for call in self.scan.call_args_list]
        self.scan.reset_mock()
        return result, scanned

    def edit(self, relative):
        path = self.root / relative
        path.write_text(path.read_text() + "// edited\n")

    def test_warm_cache_source_edit_and_forced_scan(self):
        first, scans = self.collect()
        self.assertEqual(scans, ["src/a.cpp", "src/b.cpp"])
        self.assertEqual(self.collect(), (first, []))
        self.edit("src/a.cpp")
        self.assertEqual(self.collect(), (first, ["src/a.cpp"]))
        self.assertEqual(self.collect(fresh=True), (first, ["src/a.cpp", "src/b.cpp"]))

    def test_included_source_invalidates_consumers_too(self):
        self.includes["src/b.cpp"] = ["src/a.cpp"]
        self.collect()
        self.edit("src/a.cpp")
        self.assertEqual(self.collect()[1], ["src/a.cpp", "src/b.cpp"])

    def test_mirrored_standard_headers_are_cacheable_and_invalidate_all(self):
        mirror = self.root / "build/gen/msvc-include"
        mirror.mkdir(parents=True)
        (mirror / "vector").write_text("// standard header without a suffix")
        (mirror / "sys").mkdir()
        (mirror / "sys/stat.h").write_text("// nested standard header")
        self.includes["src/a.cpp"] = ["build/gen/msvc-include/vector",
                                     "build/gen/msvc-include/sys/stat.h"]
        with patch.object(ownership.clang, "mirror", return_value=mirror):
            first, _ = self.collect()
            self.assertEqual(self.collect(), (first, []))
            self.edit("build/gen/msvc-include/vector")
            self.assertEqual(self.collect()[1], ["src/a.cpp", "src/b.cpp"])
            self.edit("build/gen/msvc-include/sys/stat.h")
            self.assertEqual(self.collect()[1], ["src/a.cpp", "src/b.cpp"])

    def test_relative_vendor_include_is_cacheable_and_invalidates_all(self):
        header = self.root / "vendor/sdk/include/sdk.h"
        header.parent.mkdir(parents=True)
        header.write_text("// third-party declaration")
        self.includes["src/a.cpp"] = ["src/../vendor/sdk/include/sdk.h"]
        first, _ = self.collect()
        self.assertEqual(self.collect(), (first, []))
        self.edit("vendor/sdk/include/sdk.h")
        self.assertEqual(self.collect()[1], ["src/a.cpp", "src/b.cpp"])

    def test_header_config_or_header_addition_invalidates_all(self):
        self.collect()
        for relative in ("include/header.h", "config/units.toml", "scripts/homm3/core/clang.py",
                         "scripts/homm3/retail_labels/source.py"):
            self.edit(relative)
            self.assertEqual(self.collect()[1], ["src/a.cpp", "src/b.cpp"])
        (self.root / "src/new.h").write_text("// newly resolvable include")
        self.assertEqual(self.collect()[1], ["src/a.cpp", "src/b.cpp"])

    def test_parse_errors_and_corrupt_entries_are_retried(self):
        self.failures = ["PARSE missing include"]
        self.collect()
        self.assertEqual(self.collect()[1], ["src/a.cpp", "src/b.cpp"])
        self.failures = []
        self.collect()
        for entry in (self.root / "build/source-ownership/units").glob("*.json"):
            entry.write_text("{")
        self.assertEqual(self.collect()[1], ["src/a.cpp", "src/b.cpp"])
        for entry in (self.root / "build/source-ownership/units").glob("*.json"):
            saved = json.loads(entry.read_text())
            saved['inputs'] = list(saved['inputs'])  # valid JSON, wrong schema
            entry.write_text(json.dumps(saved))
        self.assertEqual(self.collect()[1], ["src/a.cpp", "src/b.cpp"])

    def test_real_clang_tracks_macro_only_and_included_source_dependencies(self):
        # Macro headers have no declaration cursors: include enumeration must
        # still reach them, and included .cpp changes must reparse consumers.
        self.scan.side_effect = self.real_scan
        (self.root / "include/header.h").write_text("#define VALUE 1\n")
        (self.root / "src/a.cpp").write_text('#include "header.h"\nvoid a() {}\n')
        (self.root / "src/b.cpp").write_text('#include "a.cpp"\nvoid b() {}\n')
        before, _ = self.collect()
        self.assertEqual(before[1], [])
        self.assertIn("include/header.h", before[2])
        self.assertEqual(self.collect()[1], [])
        self.edit("src/a.cpp")
        after, scanned = self.collect()
        self.assertEqual(scanned, ["src/a.cpp", "src/b.cpp"])
        self.assertEqual(before, after)
