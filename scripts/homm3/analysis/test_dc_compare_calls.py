"""Hermetic tests for `homm3 dreamcast compare-calls`."""
from __future__ import annotations

from types import SimpleNamespace
import unittest

from homm3.analysis import dc_compare_calls as compare_calls, dreamcast
from homm3.mac.relocations import Address


def _bl(origin: int, target: int) -> bytes:
    return ((18 << 26) | ((target - origin) & 0x3fffffc) | 1).to_bytes(4, "big")


class CompareTest(unittest.TestCase):
    def test_normalised_names_and_inline_traces(self):
        dc = ["army::Is", "army::Is", "town::HasBuilding", "searchArray::limit_was_reached",
              "__modls", "std::vector<int,std::allocator<int> >::size"]
        mac = ["army::is", "type_widget::sendMessage", "searchArray::limitWasReached",
               ".__nw__FUl", "textWidget::textWidget"]
        traces = {"sendmessage": [{"source": "widget.h", "definition_line": 4, "lines": [4],
                                   "dc_start": 0, "dc_end": 2, "confidence": "positive",
                                   "definitions": ["type_widget::SendMessage"]}]}
        result = compare_calls.compare(dc, mac, traces)
        self.assertEqual([(i["key"], i["mac"], i["dreamcast"]) for i in result["mac_only"]],
                         [("sendmessage", 1, 0), ("textwidget", 1, 0)])
        self.assertEqual(result["mac_only"][0]["dc_inline"], traces["sendmessage"])
        self.assertEqual([(i["key"], i["mac"], i["dreamcast"]) for i in result["dreamcast_only"]],
                         [("hasbuilding", 0, 1), ("is", 1, 2)])

    def test_aggregate_counts_functions_absences_and_sites(self):
        functions = [
            {"mac_only": [{"key": "send", "callee": "send", "mac": 2, "dreamcast": 0,
                           "dc_inline": [{}]}], "dreamcast_only": []},
            {"mac_only": [{"key": "send", "callee": "send", "mac": 2, "dreamcast": 1,
                           "dc_inline": []}],
             "dreamcast_only": [{"key": "is", "callee": "army::Is", "mac": 0, "dreamcast": 3}]},
        ]
        totals = compare_calls.aggregate(functions)
        self.assertEqual(totals["mac_only"], [{
            "key": "send", "callee": "send", "functions": 2, "absent_functions": 1,
            "mac_sites": 4, "dreamcast_sites": 1, "dc_inline_functions": 1}])
        self.assertEqual(totals["dreamcast_only"][0]["dreamcast_sites"], 3)

    def test_mac_sites_split_named_plumbing_and_unnamed(self):
        origin = 0x100
        code = (_bl(origin, 0x200) + _bl(origin + 4, 0x300) + _bl(origin + 8, 0x400)
                + bytes.fromhex("4e800020"))
        mac = object.__new__(compare_calls.MacCalls)
        mac.section = 0
        mac.pef = SimpleNamespace(code=lambda section, offset, size: code[:size])
        mac.pairs = {0x401000: SimpleNamespace(
            label="hero::run", mac=SimpleNamespace(offset=origin, size=len(code)))}
        mac.symbols = {"game:200": Address(0, 0x200), "plumbing:300": Address(0, 0x300)}
        mac.labels = {"game:200": "army::isAlive", "plumbing:300": ".__copy"}
        sites = mac.calls(0x401000)
        self.assertEqual(sites["named"], ["army::isAlive"])
        self.assertEqual((sites["plumbing_sites"], sites["unnamed_sites"]), (1, 1))
        self.assertIsNone(mac.calls(0x402000))

    def test_vendored_zlib_procedures_are_plumbing(self):
        corpus = SimpleNamespace(by_offset={
            0x10: [{"file": r"E:\gamedcs\zlib113\inflate.cpp"}],
            0x20: [{"file": r"E:\gamedcs\game.cpp"}]})
        self.assertTrue(compare_calls._vendor(corpus, 0x10))
        self.assertFalse(compare_calls._vendor(corpus, 0x20))
        self.assertFalse(compare_calls._vendor(corpus, 0x30))

    def test_command_is_registered(self):
        args = dreamcast._build_parser().parse_args(["compare-calls", "--all", "--json"])
        self.assertTrue(args.all and args.json)
        self.assertEqual(args.limit, 25)
        self.assertIn("compare-calls", dreamcast.COMMANDS)


if __name__ == "__main__":
    unittest.main()
