"""Trace isolation and negative controls; the real compiler is checked separately."""
from contextlib import ExitStack, redirect_stderr, redirect_stdout
import io
from pathlib import Path
import subprocess
import tempfile
import unittest
from unittest.mock import patch

from homm3.vc6.shim import build


class InlineTraceTest(unittest.TestCase):
    def test_named_comparisons_keep_overloads_and_budget_fields(self):
        rows = [
            "sym a caller", "sym b ?overload", "main a cb=1200",
            "site root=a owner=a callee=b cb=96 budget=95 depth=2 remain=3 running=1500",
            "site root=a owner=b callee=c cb=20 budget=0 depth=3 remain=1 running=1520",
        ]
        with patch("homm3.core.undname.demangle", return_value={"?overload": "void helper(int)"}):
            text = build._formatInlineTrace(rows)
        self.assertIn("Function: caller (cb=1200)", text)
        self.assertIn("#1 depth=2  void helper(int)", text)
        self.assertIn("size=96 budget=95 sites remaining=3 running size=1500", text)
        self.assertIn("owner: void helper(int)", text)
        self.assertIn("<unresolved symbol c>", text)

    def test_named_trace_distinguishes_pre_budget_rejection(self):
        rows = ["sym a caller", "sym b helper", "main a cb=272",
                "candidate root=a callee=b body_flags=00000000 callee_flags=0000032a",
                "candidate root=a callee=b body_flags=00018000 callee_flags=0000032a"]
        with patch("homm3.core.undname.demangle", return_value={}):
            text = build._formatInlineTrace(rows)
        self.assertEqual(text.count("Caller-state gate rejects helper before budget testing."), 1)

    def run_trace(self, *, changed_byte=False, missing_stream=False,
                  observed=True, registers=False, debug_path=False):
        with tempfile.TemporaryDirectory() as directory, ExitStack() as stack:
            root = Path(directory)
            kind = "register" if registers else "inline"
            stem = "bindings" if registers else "comparisons"
            named = root / f"gate/{kind}-trace/unit/{stem}.txt"
            named.parent.mkdir(parents=True)
            named.write_text("old passing trace\n")
            source = root / "original.cpp"
            source.write_text("namespace { int value; }\n")
            flags = ["/O2", "/Ob2", "/MT", "/GX"]
            streams = {suffix: f"fixed nonce {suffix}".encode()
                       for suffix in ("in", "gl", "sy", "ex")}
            feeds = []
            output_paths = []

            def compile_object(out, src, options, env=None):
                self.assertEqual(src, source)
                self.assertEqual(options[:-1], flags)
                if options[-1].startswith("/d1il"):
                    prefix = options[-1][5:]
                    for suffix, data in streams.items():
                        if not (missing_stream and suffix == "ex"):
                            Path(prefix + suffix).write_bytes(data)
                    return subprocess.CompletedProcess([], 1, "",
                        "fatal error C1083: Cannot open compiler intermediate file")
                self.assertTrue(options[-1].startswith("/d2il"))
                prefix = options[-1][5:]
                feed = {suffix: Path(prefix + suffix).read_bytes()
                        for suffix in streams}
                feeds.append(feed)
                output_paths.append(out)
                # Model C2 consuming the stream files. The next replay must
                # restore the original capture, not reuse the consumed input.
                for suffix in streams:
                    Path(prefix + suffix).unlink()
                body = bytearray(range(20))
                if debug_path:
                    body.extend(str(out).encode())
                body[4:8] = bytes([len(feeds)]) * 4
                if env:
                    if changed_byte:
                        body[12] ^= 1
                    if observed:
                        if registers:
                            self.assertEqual(env["HOMM3_VC6_REGISTER_TRACE"], "function")
                            row = ("sym abc caller\nregister root=abc site=0003356e "
                                   "selected=edx value=3:00000042 " + " ".join(
                                       f"{r}=free" for r in
                                       ("eax", "ecx", "edx", "ebx", "esp", "ebp", "esi", "edi")))
                        else:
                            row = "main abc cb=100\n"
                        Path(env["HOMM3_VC6_SHIM_LOG"]).write_text(row)
                out.write_bytes(body)
                return subprocess.CompletedProcess([], 0, "", "")

            stack.enter_context(patch.object(build, "GATE_DIR", root / "gate"))
            stack.enter_context(patch.object(build, "_ensure_wine_env"))
            stack.enter_context(patch.object(build, "ensure_overlay"))
            shim = stack.enter_context(patch.object(build, "compile_shim"))
            stack.enter_context(patch.object(build, "_cc_wrap", side_effect=compile_object))
            stack.enter_context(patch.object(build.cc_wrap, "winepath_w", side_effect=str))
            stack.enter_context(patch("homm3.vc6._unit.source_for_unit", return_value=source))
            stack.enter_context(patch("homm3.vc6._unit.flags_for_unit", return_value=flags))
            with redirect_stdout(io.StringIO()), redirect_stderr(io.StringIO()):
                try:
                    result = (build.runRegisterTrace if registers else
                              build.runInlineTrace)("unit", "function")
                except SystemExit:
                    result = "error"
            verdict = (root / f"gate/{kind}-trace/unit/verdict.txt").read_text()
            self.assertEqual(named.exists(), result == 0)
            if len(output_paths) == 2:
                self.assertEqual(output_paths[0], output_paths[1])
            return result, verdict, feeds, streams, shim.call_args_list

    def test_same_capture_replayed_and_only_timestamp_ignored(self):
        result, verdict, feeds, streams, calls = self.run_trace()
        self.assertEqual(result, 0)
        self.assertTrue(verdict.startswith("PASS:"))
        self.assertEqual(feeds, [streams, streams])
        self.assertEqual(calls[-1].kwargs, {"negative": False})

    def test_changed_object_byte_rejects_trace_and_restores_shim(self):
        result, verdict, _feeds, _streams, calls = self.run_trace(changed_byte=True)
        self.assertEqual(result, 1)
        self.assertIn("FAIL: 1 object differences", verdict)
        self.assertEqual(calls[-1].kwargs, {"negative": False})

    def test_incomplete_capture_never_installs_trace(self):
        result, verdict, feeds, _streams, calls = self.run_trace(missing_stream=True)
        self.assertEqual(result, "error")
        self.assertFalse(verdict.startswith("PASS:"))
        self.assertEqual(feeds, [])
        self.assertEqual(calls, [])

    def test_absent_function_rejects_trace_and_restores_shim(self):
        result, verdict, _feeds, _streams, calls = self.run_trace(observed=False)
        self.assertEqual(result, "error")
        self.assertIn("FAIL: no matching function", verdict)
        self.assertEqual(calls[-1].kwargs, {"negative": False})

    def test_debug_object_path_remains_identical(self):
        result, verdict, *_ = self.run_trace(debug_path=True)
        self.assertEqual(result, 0)
        self.assertTrue(verdict.startswith("PASS:"))

    def test_register_mode_uses_same_capture_and_identity_gate(self):
        result, verdict, feeds, streams, calls = self.run_trace(registers=True)
        self.assertEqual(result, 0)
        self.assertIn("not the full allocator", verdict)
        self.assertEqual(feeds, [streams, streams])
        self.assertEqual(calls[0].kwargs, {"registerTrace": True})
        self.assertEqual(calls[-1].kwargs, {"negative": False})
        result, verdict, _, _, calls = self.run_trace(registers=True, changed_byte=True)
        self.assertEqual(result, 1)
        self.assertIn("FAIL: 1 object differences", verdict)
        self.assertEqual(calls[-1].kwargs, {"negative": False})

    def test_missing_register_events_fail_closed(self):
        result, verdict, _, _, calls = self.run_trace(registers=True, observed=False)
        self.assertEqual(result, "error")
        self.assertIn("FAIL: no matching function", verdict)
        self.assertEqual(calls[-1].kwargs, {"negative": False})


if __name__ == "__main__":
    unittest.main()
