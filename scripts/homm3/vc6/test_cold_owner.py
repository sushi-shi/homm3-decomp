"""Contract tests for homm3.vc6.cold_owner (synthetic bytes; no DLL needed)."""
import struct
import unittest

from homm3.vc6 import cold_owner


def ice(line: int, va: int) -> bytes:
    return b"\xba" + struct.pack("<I", line) + b"\xb9" + struct.pack("<I", va) + b"\xe8\0\0\0\0"


class ColdOwnerTest(unittest.TestCase):
    PATHS = {0x1000: "E:\\8447\\vc98\\p2\\src\\P2\\globdf.c",
             0x2000: "E:\\8447\\vc98\\p2\\src\\P2\\globopt.c",
             0x3000: "not a compiler path"}

    def sites(self):
        text = b"\x90" * 16 + ice(10, 0x1000) + b"\x90" * 16 + ice(20, 0x1000) \
            + b"\x90" * 16 + ice(30, 0x2000) + ice(40, 0x3000)
        return cold_owner.ice_sites(text, 0x100, self.PATHS.get)

    def test_sites_keep_only_compiler_source_paths(self):
        self.assertEqual([(s[1], s[2]) for s in self.sites()],
                         [("globdf.c", 10), ("globdf.c", 20), ("globopt.c", 30)])

    def test_between_same_tu_is_that_tu_and_boundary_is_ambiguous(self):
        sites = self.sites()
        inside = (sites[0][0] + sites[1][0]) // 2
        boundary = (sites[1][0] + sites[2][0]) // 2
        self.assertEqual(cold_owner.tu_between(sites, inside), "globdf.c")
        self.assertEqual(cold_owner.tu_between(sites, boundary), "globdf.c|globopt.c")
        self.assertEqual(cold_owner.tu_between(sites, 0), "?|globdf.c")

    def test_only_jumps_into_the_upper_region_count(self):
        entry = 0x5000
        # jz rel32 to 0x70000 (upper) ; jmp rel32 to 0x5100 (hot) ; ret
        jz = b"\x0f\x84" + struct.pack("<i", 0x70000 - (entry + 6))
        jmp = b"\xe9" + struct.pack("<i", 0x5100 - (entry + 6 + 5))
        targets = cold_owner.cold_targets(jz + jmp + b"\xc3", entry)
        self.assertEqual(targets, [0x70000])


if __name__ == "__main__":
    unittest.main()
