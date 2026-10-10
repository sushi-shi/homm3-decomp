"""A shared source read by another image: only its VA_AT claims count."""
import tempfile
import unittest
from pathlib import Path
from unittest import mock

from homm3.core import common
from homm3.retail_labels import source

SHARED = """\
VA(0x0055ee50, 0x76)
VA_AT(h3maped, 0x004b3c4e, 0x61)
MAC_ADDRESS(0x253c60, 0x6c)
TRiverPlacementOp::TRiverPlacementOp(TAbstractMap* newAdapter)
    : TRiverOp(newAdapter)
{
}

VA_COMPGEN(0x0055eed0, 0x21, SCALAR_DELETING_DTOR, TRiverPlacementOp)

VA_AT(h3ccmped, 0x00401000, 0x10)
VA_AT(h3maped, 0x004b3d3c, 0x5b)
TRiverEraseOp::TRiverEraseOp(TAbstractMap* newAdapter)
{
}

DATA(0x0069E5D0)
TRmgLinePatternTable g_rmgRiverPatternTable(13, DATA_COMPGEN(0x00641140, kPatterns, "x"));
"""


class ImageViewTest(unittest.TestCase):
    def test_image_reads_only_its_own_va_at_claims_at_unchanged_offsets(self):
        view = source.image_view(SHARED, "h3maped")
        self.assertEqual(len(view), len(SHARED))
        self.assertEqual(view.count("\n"), SHARED.count("\n"))
        self.assertNotIn("0x0055ee50", view)
        self.assertNotIn("VA_COMPGEN", view)
        self.assertNotIn("DATA", view)
        self.assertNotIn("0x00401000", view)
        heads = [line.split(",")[0].replace(" ", "") for line in view.splitlines()
                 if line.lstrip().startswith("VA(")]
        self.assertEqual(heads, ["VA(0x004b3c4e", "VA(0x004b3d3c"])
        # the game reads the file as written: VA_AT is no VA() site
        self.assertEqual(len(source.macro_invocations(
            source.mask_lexical_noise(SHARED), source.VA_HEAD_RE)), 1)

    def test_scan_binds_each_va_at_claim_to_its_own_declarator(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "rmg_river.cpp"
            path.write_text(SHARED)
            functions = {0x4b3c4e - common.IMAGE_BASE, 0x4b3d3c - common.IMAGE_BASE}
            with mock.patch.object(source, "shared_image_source", return_value=True), \
                    mock.patch("homm3.core.paths.image_key", return_value="h3maped"):
                rows = source.scan_file(path, functions, [])
        self.assertEqual([(r["rva"] + common.IMAGE_BASE, r["size"], r["name"], r["dtor"])
                          for r in rows],
                         [(0x4b3c4e, 0x61, "TRiverPlacementOp_TRiverPlacementOp", False),
                          (0x4b3d3c, 0x5b, "TRiverEraseOp_TRiverEraseOp", False)])

    def test_game_declarator_search_skips_a_va_at_line(self):
        text = "VA(0x00401000, 0x10)\nVA_AT(h3maped, 0x00402000, 0x10)\nvoid draw()\n{\n}\n"
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "widget.cpp"
            path.write_text(text)
            rows = source.scan_file(path, {0x1000}, [])
        self.assertEqual([(r["rva"], r["name"]) for r in rows], [(0x1000, "draw")])

    def test_ir_names_follow_the_selected_image(self):
        ir = ('@.str = private constant [27 x i8] c"va:0x0055ee50 size:0x76\\00"\n'
              '@.str.1 = private constant [36 x i8] c"va_at:h3maped 0x004b3c4e size:0x61\\00"\n'
              '@.str.2 = private constant [37 x i8] c"va_at:h3ccmped 0x00401000 size:0x10\\00"\n'
              '@llvm.global.annotations = appending global [3 x { ptr, ptr, ptr, i32, ptr }] '
              '[{ ptr, ptr, ptr, i32, ptr } { ptr @"?a@@YAXXZ", ptr @.str, ptr null, i32 1, ptr null }, '
              '{ ptr, ptr, ptr, i32, ptr } { ptr @"?a@@YAXXZ", ptr @.str.1, ptr null, i32 1, ptr null }, '
              '{ ptr, ptr, ptr, i32, ptr } { ptr @"?b@@YAXXZ", ptr @.str.2, ptr null, i32 1, ptr null }]\n')
        self.assertEqual(source.ir_va_names(ir), {0x15ee50: "?a@@YAXXZ"})
        self.assertEqual(source.ir_va_names(ir, "h3maped"), {0xb3c4e: "?a@@YAXXZ"})
        self.assertEqual(source.ir_va_names(ir, "h3ccmped"), {0x1000: "?b@@YAXXZ"})


if __name__ == "__main__":
    unittest.main()
