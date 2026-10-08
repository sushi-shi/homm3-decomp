"""homm3.core.images - the linked programs this repository reconstructs.

The game (`HEROES3.EXE`, config/project.toml `[inputs.retail]`) is the default
image `game`. Every other `[inputs.<key>]` row carrying `image = true` is a
further image keyed by `<key>`; the GOG Complete map editor is `h3maped`.

The selected image (`homm3 --image KEY`, exported to child processes as
`$HOMM3_IMAGE`) decides which retail executable, which config/retail table
set and which generated trees a command reads. The game keeps config/retail/
and build/; another image keeps config/retail/<image>/ and build/<image>/.
This module only reads the pins; homm3.core.paths spells the directories.
"""
from __future__ import annotations

import os
import tomllib
from pathlib import Path

#: Environment variable carrying the selected image to every child process.
IMAGE_ENV = "HOMM3_IMAGE"
DEFAULT_IMAGE = "game"
#: The game's pin in config/project.toml.
GAME_INPUT = "retail"


def pins(root: Path) -> dict:
    with (Path(root) / "config/project.toml").open("rb") as stream:
        return tomllib.load(stream).get("inputs", {})


def images(root: Path) -> list[str]:
    """The pinned image keys: the game first, then each `image = true` pin."""
    return [DEFAULT_IMAGE] + [key for key, row in pins(root).items()
                              if row.get("image") is True]


def selected(root: Path) -> str:
    """The selected image; an unknown key is an error, never a fallback."""
    key = os.environ.get(IMAGE_ENV) or DEFAULT_IMAGE
    if key == DEFAULT_IMAGE:
        return key
    known = images(root)
    if key not in known:
        raise RuntimeError(f"${IMAGE_ENV}={key!r} is not a pinned image "
                           f"(config/project.toml: {known})")
    return key


def input_key(image: str) -> str:
    """The `[inputs.*]` row that pins the image's executable."""
    return GAME_INPUT if image == DEFAULT_IMAGE else image


_PREFIXES = ("build/gen", "build/objdiff", "build/delink", "build/pdb",
             "build/exe", "config/retail")


def path(rel: str, image: str | None = None) -> str:
    """Spell a game-relative per-image path for the selected image.

    `build/{gen,objdiff,delink,pdb,exe}...` gain the image directory
    (`build/<image>/gen...`), `config/retail...` its table directory
    (`config/retail/<image>...`), and the ledger and unit manifest their
    image suffix (`config/match_baseline.<image>.tsv`, `config/units.<image>.toml`).
    The game's spellings are returned unchanged."""
    key = image or os.environ.get(IMAGE_ENV) or DEFAULT_IMAGE
    if key == DEFAULT_IMAGE:
        return rel
    for prefix in _PREFIXES:
        if rel == prefix or rel.startswith(prefix + "/"):
            head, _, tail = prefix.partition("/")
            return f"{head}/{tail}/{key}{rel[len(prefix):]}" if head == "config" \
                else f"build/{key}/{tail}{rel[len(prefix):]}"
    if rel == "config/match_baseline.tsv":
        return f"config/match_baseline.{key}.tsv"
    if rel == "config/units.toml":
        return f"config/units.{key}.toml"
    if rel == "build.ninja":
        return f"build/{key}/build.ninja"
    return rel
