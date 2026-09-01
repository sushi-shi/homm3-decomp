"""Tests for the compiler-neutral debug/source structure model."""
from __future__ import annotations

import unittest

from homm3.analysis import debug_shape


class DebugShapeTest(unittest.TestCase):
    def test_serialization_keeps_zero_emission_rows_and_call_sites(self):
        call = debug_shape.DebugCall(
            site_address=0x108, target_address=0x9000,
            target_function_address=0x300,
            target_emitted_size=12, name="Widget::Open",
            classification="call")
        statement = debug_shape.DebugStatement(
            address=0x108, emitted_size=0,
            source_file=r"E:\game\unit.cpp", source_line=22,
            calls=(call,))
        line_map = debug_shape.DebugLineMap(
            procedure_line=20, procedure_line_reliable=True, bodyless=False,
            first_body_line=22, first_body_address=0x108,
            gaps=(debug_shape.DebugSourceGap(
                after_line=20, before_line=22,
                first_missing_line=21, last_missing_line=21,
                leading=True),))
        shape = debug_shape.DebugFunctionShape(
            producer="CodeView", target="test target",
            address_space="section offset", name="Run", module="unit.obj",
            linkage="global", address=0x100, emitted_size=32,
            source_file=r"E:\game\unit.cpp", boundary_line=20,
            debug_start=4, debug_end=28, line_map=line_map,
            statements=(statement,))

        payload = shape.to_dict()
        self.assertEqual(payload["schema"], debug_shape.SCHEMA)
        self.assertEqual(payload["statements"][0]["emitted_size"], 0)
        self.assertEqual(
            payload["statements"][0]["calls"][0]["site_address"], 0x108)
        self.assertEqual(payload["line_map"]["leading_gap_lines"], 1)
        self.assertNotIn("candidate", payload)
        self.assertNotIn("retail", payload)

    def test_scope_depth_is_derived_without_target_assumptions(self):
        scopes = debug_shape.scope_ranges([
            (0x100, 0x30), (0x108, 0x10), (0x10c, 0x04),
        ])
        self.assertEqual([scope.depth for scope in scopes], [0, 1, 2])


if __name__ == "__main__":
    unittest.main()
