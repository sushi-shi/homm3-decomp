"""Batch error isolation, structured byte facts and bounded image reads."""
import contextlib
import io
import json
from pathlib import Path
import tempfile
from types import SimpleNamespace
import unittest
from unittest.mock import patch

from homm3.core.image import Section
from homm3.sema import _asm, data, diff, source
from homm3.sema.__main__ import _build_parser


class BatchTest(unittest.TestCase):
    def test_bad_selector_does_not_hide_other_results_and_refreshes_once(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            (root / "u.obj").touch()
            (root / "u.c.obj").touch()
            def resolve(selector):
                if selector == "bad":
                    from homm3.sema._common import die
                    die("bad selector")
                return selector, "u", 0x1000, 1, 0
            ctx = SimpleNamespace(symbols=SimpleNamespace(resolve_fn=resolve),
                                  fn_fuzzy=lambda *a: 100)
            args = _build_parser().parse_args(["diff", "f", "bad", "g", "--json", "--why-bytes"])
            text = "00000000 <fn>:\n 0: c3\tret\n"
            with patch.object(diff, "get_context", return_value=ctx), \
                    patch.multiple(_asm, TARGET=root, NORMAL_BASE=root, NORMAL_TARGET=root), \
                    patch.object(_asm, "objdump", return_value=text), \
                    patch.object(_asm, "refresh_unit", return_value="refreshed") as refresh, \
                    patch.object(source, "load", side_effect=source.SourceError("test")), \
                    contextlib.redirect_stdout(io.StringIO()) as out, \
                    contextlib.redirect_stderr(io.StringIO()):
                self.assertEqual(diff.run(args), 2)
            payload = json.loads(out.getvalue())
            self.assertEqual([r["rc"] for r in payload["results"]], [0, 2, 0])
            self.assertIn("bad selector", payload["results"][1]["error"])
            self.assertTrue(payload["results"][2]["summary"]["agree"])
            refresh.assert_called_once_with("u")

    def test_json_contains_hex_not_python_bytes_repr(self):
        self.assertEqual(diff._json_value({"row": {"raw": b"\x00\xff"}}),
                         {"row": {"raw": "00ff"}})


class DataReadTest(unittest.TestCase):
    def test_reads_only_raw_backed_span_not_following_section_or_zero_tail(self):
        section = Section(".data", 0x1000, 4, 16, 2, False)
        image = SimpleNamespace(data=b"XXabcdNEXTSECTION", section_of=lambda r: section)
        self.assertEqual(data.read(image, 0x1001, 3), b"bcd")
        for rva, size in ((0x1003, 2), (0x1004, 1), (0x1000, 0), (0x1000, 1048577)):
            with self.subTest(rva=rva, size=size), self.assertRaises(ValueError):
                data.read(image, rva, size)


if __name__ == "__main__":
    unittest.main()
