"""homm3.core.images: image selection and per-image path spelling."""
import os
import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch

from homm3.core import images

PROJECT = """
[inputs.retail]
name = "retail executable"
path = "build/orig/HEROES3.EXE"

[inputs.dreamcast]
name = "Dreamcast executable"
path = "build/orig/dreamcast/H3.EXE"

[inputs.h3maped]
name = "GOG Complete map editor"
image = true
path = "build/orig/h3maped.exe"
sources = "editor"

[inputs.h3ccmped]
name = "GOG Complete campaign editor"
image = true
path = "build/orig/h3ccmped.exe"
sources = "campaign_editor"
"""


class ImagesTest(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.root = Path(self.tmp.name)
        (self.root / "config").mkdir()
        (self.root / "config/project.toml").write_text(PROJECT)

    def test_each_image_reads_only_its_own_source_trees(self):
        editor = [self.root / "src/editor/GameMap.cpp", self.root / "include/editor/GameMap.h"]
        shared = [self.root / "src/rmg.cpp", self.root / "include/rmg.h"]
        with patch.dict(os.environ, {images.IMAGE_ENV: "game"}):
            self.assertEqual(images.source_dir("h3maped", self.root), "editor")
            self.assertTrue(all(images.foreign(p, self.root) for p in editor))
            self.assertFalse(any(images.foreign(p, self.root) for p in shared))
        with patch.dict(os.environ, {images.IMAGE_ENV: "h3maped"}):
            self.assertFalse(any(images.foreign(p, self.root) for p in editor + shared))

    def test_a_third_image_shares_another_images_tree(self):
        editor = [self.root / "src/editor/Player.cpp", self.root / "include/editor/Player.h"]
        campaign = [self.root / "src/campaign_editor/CampaignDoc.cpp"]
        with patch.dict(os.environ, {images.IMAGE_ENV: "h3ccmped"}):
            self.assertTrue(all(images.foreign(p, self.root) for p in editor))
            self.assertFalse(any(images.foreign(p, self.root) for p in campaign))
        with patch.dict(os.environ, {images.IMAGE_ENV: "h3maped"}):
            self.assertTrue(all(images.foreign(p, self.root) for p in campaign))

    def test_only_image_pins_are_images(self):
        self.assertEqual(images.images(self.root), ["game", "h3maped", "h3ccmped"])

    def test_selection_defaults_to_the_game_and_rejects_unknown_keys(self):
        with patch.dict(os.environ, {}, clear=False):
            os.environ.pop(images.IMAGE_ENV, None)
            self.assertEqual(images.selected(self.root), "game")
        with patch.dict(os.environ, {images.IMAGE_ENV: "h3maped"}):
            self.assertEqual(images.selected(self.root), "h3maped")
        with patch.dict(os.environ, {images.IMAGE_ENV: "dreamcast"}):
            with self.assertRaises(RuntimeError):
                images.selected(self.root)

    def test_game_paths_are_unchanged(self):
        for rel in ("build/gen/claims", "build/objdiff/base/x.obj", "config/retail/relocs.tsv",
                    "config/units.toml", "config/match_baseline.tsv", "build.ninja",
                    "build/wineprefix"):
            self.assertEqual(images.path(rel, "game"), rel)

    def test_image_paths(self):
        expected = {
            "build/gen/claims": "build/h3maped/gen/claims",
            "build/objdiff": "build/h3maped/objdiff",
            "build/delink": "build/h3maped/delink",
            "config/retail": "config/retail/h3maped",
            "config/retail/relocs.tsv": "config/retail/h3maped/relocs.tsv",
            "config/units.toml": "config/units.h3maped.toml",
            "config/match_baseline.tsv": "config/match_baseline.h3maped.tsv",
            "build.ninja": "build/h3maped/build.ninja",
            # shared state keeps its spelling
            "build/wineprefix": "build/wineprefix",
            "build/orig/h3maped.exe": "build/orig/h3maped.exe",
            "build/generated": "build/generated",
        }
        for rel, want in expected.items():
            self.assertEqual(images.path(rel, "h3maped"), want, rel)

    def test_input_key(self):
        self.assertEqual(images.input_key("game"), "retail")
        self.assertEqual(images.input_key("h3maped"), "h3maped")


if __name__ == "__main__":
    unittest.main()
