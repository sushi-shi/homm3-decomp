"""Select the checkout a homm3 process operates on.

The current directory decides. A shell or agent that entered the main
checkout's dev shell exports ``HOMM3_DIR`` there; a later command run from a
linked worktree must not silently compile, compare or bank the main checkout.
When the inherited ``HOMM3_DIR`` names a different checkout than the one
containing the current directory, the current checkout wins and one warning
goes to stderr. ``HOMM3_DIR_FORCE=1`` keeps ``HOMM3_DIR`` for an intentional
cross-tree command. Outside any checkout, ``HOMM3_DIR`` (or the checkout that
supplies this code) is used as before.

``scripts/project-root.sh --select`` applies the same rule for the installed
``homm3`` wrapper, which then exports the selected root to this process.
A ``HOMM3_DIR`` that exists but is not inside any checkout (a test fixture) is
honored as spelled; only a different checkout is overridden.
"""
from __future__ import annotations

import os
from pathlib import Path
import sys

MARKER_FILES = ("flake.nix", "config/project.toml", "config/units.toml")
FORCE_VARIABLE = "HOMM3_DIR_FORCE"


def project_root(start: str | os.PathLike | None) -> Path | None:
    """The nearest directory at or above ``start`` carrying the project markers."""
    if not start:
        return None
    try:
        path = Path(start).resolve()
    except (OSError, RuntimeError):
        return None
    for candidate in (path, *path.parents):
        if (all((candidate / name).is_file() for name in MARKER_FILES)
                and (candidate / "scripts/homm3").is_dir()):
            return candidate
    return None


def select(environ=None, cwd: str | os.PathLike | None = None,
           fallback: Path | None = None) -> tuple[Path | None, str | None]:
    """Return ``(root, warning)`` under the current-directory-first policy."""
    environ = os.environ if environ is None else environ
    if cwd is None:
        try:
            cwd = os.getcwd()
        except OSError:
            cwd = None
    here = project_root(cwd)
    requested = environ.get("HOMM3_DIR") or ""
    if not requested:
        return here or fallback, None
    explicit = Path(requested)
    if here is None or environ.get(FORCE_VARIABLE) == "1":
        return explicit, None
    if not explicit.is_dir():
        return here, (f"homm3: HOMM3_DIR={requested} does not exist; using the "
                      f"current checkout {here}")
    named = project_root(explicit)
    if named is None or named == here:
        return explicit, None
    return here, (f"homm3: HOMM3_DIR={requested} is a different checkout than the "
                  f"current directory's; using {here} "
                  f"(set {FORCE_VARIABLE}=1 to keep HOMM3_DIR)")


_SELECTED: Path | None = None


def process_root(fallback: Path) -> Path:
    """The root for this process; warn once and export an overriding choice.

    Exporting the override keeps child processes (Ninja rules, nested
    ``python -m homm3...`` stages, tools run from another directory) on the
    same checkout without repeating the warning.
    """
    global _SELECTED
    if _SELECTED is None:
        root, warning = select(fallback=fallback)
        root = root or fallback
        if warning:
            print(warning, file=sys.stderr)
            os.environ["HOMM3_DIR"] = str(root)
        _SELECTED = root
    return _SELECTED


def code_root(module_file: str) -> Path | None:
    """The checkout that supplies a module, by its flake marker."""
    here = Path(module_file).resolve()
    return next((parent for parent in here.parents if (parent / "flake.nix").exists()), None)
