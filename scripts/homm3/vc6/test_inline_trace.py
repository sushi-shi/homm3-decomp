"""A budget trace is evidence only for the selected, unchanged compiler body."""
import os
from pathlib import Path
import struct
import tempfile
import unittest
from unittest.mock import patch

from homm3.vc6 import inline_trace
from homm3.vc6.__main__ import _build_parser

ROOT = '# inline ROOT 22 sym=00bdf500 cb=1530 handle=00003c4a flags=0000052a name=CreateRiver'
SITE = '# inline SITE 22 depth=2 budget=68 remaining=2 sym=00bf36c0 cb=49 handle=000047bc flags=000000e8 name=_Destroy'


class InlineTraceTests(unittest.TestCase):
    def test_records_budget_allowance_without_claiming_final_expansion(self):
        report = inline_trace.parse_trace(ROOT + '\n' + SITE, 'CreateRiver')
        self.assertEqual(report['caller']['initial_budget'], 3060)
        self.assertEqual(report['sites'][0]['cb'], 49)
        self.assertTrue(report['sites'][0]['budget_allows'])
        self.assertNotIn('expanded', report['sites'][0])

    def test_budget_rejection_and_signed_small_callee(self):
        low = SITE.replace('budget=68', 'budget=48')
        self.assertFalse(inline_trace.parse_trace(ROOT + '\n' + low, 'CreateRiver')['sites'][0]['budget_allows'])
        wrapped = ROOT.replace('cb=1530', 'cb=-32768')
        self.assertEqual(inline_trace.parse_trace(wrapped, 'CreateRiver')['caller']['initial_budget'], 1000)

    def test_rejects_missing_wrong_or_multiple_roots(self):
        for text in ('', SITE, ROOT.replace('CreateRiver', 'Other'), ROOT + '\n' + ROOT,
                     ROOT + '\n' + SITE.replace('SITE 22', 'SITE 23')):
            with self.subTest(text=text), self.assertRaises(ValueError):
                inline_trace.parse_trace(text, 'CreateRiver')

    def test_rejects_truncated_and_error_records(self):
        for line in ('# inline ERROR hook bytes changed', SITE[:-12],
                     SITE.replace('remaining=2', 'remaining=0')):
            with self.subTest(line=line), self.assertRaises(ValueError):
                inline_trace.parse_trace(ROOT + '\n' + line, 'CreateRiver')

    def test_canonical_overlay_records(self):
        records = ("sym 0x1 CreateRiver\nsym 0x2 _Destroy\nmain 0x1 cb=1530\n"
                   "site root=0x1 owner=0x1 callee=0x2 cb=49 budget=68 "
                   "depth=2 remain=2 running=1800")
        report = inline_trace.parse_trace(records, "CreateRiver")
        self.assertEqual(report["caller"]["initial_budget"], 3060)
        self.assertEqual(report["sites"][0]["symbol"], "_Destroy")
        self.assertTrue(report["sites"][0]["budget_allows"])
        for bad in (records.replace("root=0x1", "root=0x3"),
                    records.replace("remain=2", "remain=0"),
                    records + "\nmain 0x1 cb=1530"):
            with self.assertRaises(ValueError):
                inline_trace.parse_trace(bad, "CreateRiver")

    def test_pre_budget_state_gate_reports_rejection_without_final_verdict(self):
        prefix = "sym a caller\nsym b helper\nmain a cb=272\n"
        record = "candidate root=a callee=b body_flags=00000000 callee_flags=0000012a"
        candidate = inline_trace.parse_trace(prefix + record, "caller")["candidates"][0]
        self.assertFalse(candidate["state_gate_allows"])
        self.assertNotIn("expanded", candidate)
        for changed in (record.replace("body_flags=00000000", "body_flags=00008000"),
                        record.replace("body_flags=00000000", "body_flags=00010000"),
                        record.replace("callee_flags=0000012a", "callee_flags=0000002a")):
            self.assertTrue(inline_trace.parse_trace(prefix + changed, "caller")
                            ["candidates"][0]["state_gate_allows"])
        for changed in (record.replace("root=a", "root=c"),
                        record.replace("callee=b", "callee=c")):
            with self.assertRaises(ValueError):
                inline_trace.parse_trace(prefix + changed, "caller")

    def test_identity_gate_accepts_only_timestamp_changes(self):
        original = struct.pack('<HHIIIHH', 0x14c, 1, 1, 0, 0, 0, 0) + b'\x55\x8b\xec\xc3'
        stamped = original[:4] + b'\xff' * 4 + original[8:]
        self.assertEqual(inline_trace.verify_identity(original, stamped)['object_bytes'], len(original))
        for changed in (original[:-1] + b'\xcc', original + b'\x00', original[:19]):
            with self.subTest(changed=changed), self.assertRaises(ValueError):
                inline_trace.verify_identity(original, changed)

    def test_trace_keeps_the_shared_selector_forms(self):
        parser = _build_parser()
        for selector in ('0x548df0', '0x148df0', 'type_random_map_generator::CreateRiver',
                         'CreateRiver', 'rmg:0x548df0'):
            args = parser.parse_args(['predict-inline', selector, '--trace', '--json'])
            self.assertEqual(args.src, selector)
            self.assertTrue(args.trace)


@unittest.skipUnless(os.environ.get('HOMM3_TEST_VC6_TRACE') == '1',
                     'requires the pinned VC6 toolchain and Wine')
class LiveInlineTraceTests(unittest.TestCase):
    def test_mutating_shim_is_rejected_then_clean_trace_recovers(self):
        from homm3.vc6.shim import build

        symbol = '?shim_gate_hash@@YIIPBEI@Z'
        with tempfile.TemporaryDirectory(prefix='vc6-inline-trace-') as temporary:
            directory = Path(temporary)
            expected = directory / 'expected.obj'
            build._ensure_wine_env()
            proc = build._cc_wrap(expected, build.SAMPLE, build.GATE_FLAGS)
            self.assertEqual(proc.returncode, 0, build._tail(proc))
            original_compile = build.compile_shim
            try:
                with patch.object(build, 'compile_shim',
                                  side_effect=lambda **kwargs: original_compile(negative=True) if kwargs.get("inlineTrace") else original_compile()):
                    with self.assertRaisesRegex(ValueError, 'changed the captured-IL object'):
                        inline_trace.capture(build.SAMPLE, build.GATE_FLAGS, symbol,
                                             expected, workdir=directory / 'negative')
            finally:
                original_compile(negative=False)
            report = inline_trace.capture(build.SAMPLE, build.GATE_FLAGS, symbol,
                                          expected, workdir=directory / 'restored')
            self.assertEqual(report['caller']['symbol'], symbol)
            self.assertGreater(report['oracle']['matching_function_bytes'], 0)
            self.assertTrue(report['sites'])


if __name__ == '__main__':
    unittest.main()
