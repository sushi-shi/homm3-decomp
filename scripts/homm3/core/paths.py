"""Project paths shared by the Gruntz-derived model and data pipeline.

``RETAIL`` and ``BUILD`` name the SELECTED IMAGE's admitted tables and
generated-state root (homm3.core.images): config/retail/ and build/ for the
game, config/retail/<image>/ and build/<image>/ for another image. The
toolchain, staged executables (build/orig), the Wine prefix and the
Mac/Dreamcast evidence stay shared under ``SHARED_BUILD``.
"""
from pathlib import Path

from homm3.core import images as _images
from homm3.core.common import HOMM3_DIR

ROOT = HOMM3_DIR
SHARED_BUILD = ROOT / "build"
CONFIG = ROOT / "config"
RETAIL_ROOT = CONFIG / "retail"
SRC = ROOT / "src"
INCLUDE = ROOT / "include"
REPO = ROOT

DEFAULT_IMAGE = _images.DEFAULT_IMAGE
IMAGE_ENV = _images.IMAGE_ENV


def image_key() -> str:
    return _images.selected(ROOT)


def images() -> list[str]:
    return _images.images(ROOT)


def is_game(image: str | None = None) -> bool:
    return (image or image_key()) == DEFAULT_IMAGE


def retail_dir(image: str | None = None) -> Path:
    """config/retail for the game; config/retail/<image> for another image."""
    key = image or image_key()
    return RETAIL_ROOT if key == DEFAULT_IMAGE else RETAIL_ROOT / key


def image_build(image: str | None = None) -> Path:
    """build/ for the game; build/<image>/ for another image."""
    key = image or image_key()
    return SHARED_BUILD if key == DEFAULT_IMAGE else SHARED_BUILD / key


def image_build_rel(image: str | None = None) -> str:
    """`build` or `build/<image>`, for generated Ninja and objdiff files."""
    return image_build(image).relative_to(ROOT).as_posix()


def baseline(image: str | None = None) -> Path:
    """config/match_baseline.tsv for the game; config/match_baseline.<image>.tsv
    for another image."""
    key = image or image_key()
    return CONFIG / ("match_baseline.tsv" if key == DEFAULT_IMAGE
                     else f"match_baseline.{key}.tsv")


def manifest(image: str | None = None) -> Path:
    """config/units.toml for the game; config/units.<image>.toml for another
    image (its own unit list and profiles; shared sources are named there)."""
    key = image or image_key()
    return CONFIG / ("units.toml" if key == DEFAULT_IMAGE else f"units.{key}.toml")


RETAIL = retail_dir()
BUILD = image_build()


def retail_exe():
    from homm3.core import inputs
    return inputs.stage_executable(inputs.RETAIL)


def msvc_dir():
    from homm3.core.project import Project
    return Project(ROOT).toolchain
