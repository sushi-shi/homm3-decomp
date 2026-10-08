"""homm3.init.vc6_rtm - the VC6 RTM C compiler that built retail's zlib.

Retail's Rich header counts every zlib object under `Utc12_C` build 8168
and has no 8447 C entry, so zlib was compiled by the RTM C compiler
(docs/reference/executable-libraries.md). Units naming
`compiler = "msvc6-rtm"` compile with it.

The compiler is an overlay of the pinned SP3 tree: `bin/` is copied (the CL
driver's LoadLibraryA must find real files beside it), every other entry is
symlinked, and the RTM front and back ends replace `bin/C1.DLL` and
`bin/C2.DLL`. The RTM binaries are never in git: they are staged outside the
repository (config/project.toml `[toolchain.compilers."msvc6-rtm"]`
`source`, provenance in its PROVENANCE.txt) and every one is hash-checked
against that table's `files` before it is installed.

    python3 -m homm3.init.vc6_rtm [--source DIR] [--force]

`homm3 init` builds the overlay whenever the staged binaries are present.
"""
from __future__ import annotations

import argparse
import hashlib
import shutil
import sys
from pathlib import Path

from homm3.core import common

NAME = "msvc6-rtm"


def spec() -> dict:
    from homm3.core.project import Project
    return dict(Project(common.HOMM3_DIR).specification
                .get("toolchain", {}).get("compilers", {}).get(NAME, {}))


def destination() -> Path:
    return common.HOMM3_DIR / spec()["locations"][0]


def source_dir(override: str | Path | None = None) -> Path:
    """`override`, else $HOMM3_VC6_RTM_SOURCE, else the pinned `source`."""
    import os
    override = override or os.environ.get("HOMM3_VC6_RTM_SOURCE")
    if override:
        return Path(override).expanduser().resolve()
    return (common.HOMM3_DIR / spec()["source"]).resolve()


def verify(source: Path) -> dict[str, Path]:
    """{bin name: staged path} after checking every pinned hash."""
    files = spec().get("files", {})
    if not files:
        raise ValueError(f"config/project.toml pins no {NAME} files")
    out = {}
    for name, digest in files.items():
        path = source / name
        if not path.is_file():
            raise ValueError(f"{NAME}: {path} is missing")
        actual = hashlib.sha256(path.read_bytes()).hexdigest()
        if actual != digest:
            raise ValueError(f"{NAME}: {path} sha256 {actual} != pinned {digest}")
        out[name] = path
    return out


def current() -> bool:
    """True when the overlay holds exactly the pinned RTM passes."""
    from homm3.core.cc_wrap import find_ci
    bin_dir = destination() / "bin"
    for name, digest in spec().get("files", {}).items():
        path = find_ci(bin_dir, name)
        if path is None or hashlib.sha256(path.read_bytes()).hexdigest() != digest:
            return False
    return find_ci(bin_dir, "cl.exe") is not None


def build(source: str | Path | None = None, force: bool = False) -> Path:
    """Build (or repair) the overlay; returns its msvc/ root."""
    from homm3.core.cc_wrap import find_ci, msvc_dir
    staged = verify(source_dir(source))
    pinned = msvc_dir()
    if not find_ci(pinned / "bin", "cl.exe"):
        raise ValueError(f"no pinned SP3 toolchain at {pinned}; run `homm3 init`")
    msvc = destination()
    if force and msvc.exists():
        shutil.rmtree(msvc)
    bin_dir = msvc / "bin"
    if not bin_dir.is_dir():
        msvc.mkdir(parents=True, exist_ok=True)
        for entry in sorted(pinned.iterdir()):
            if entry.name.lower() == "bin":
                continue
            link = msvc / entry.name
            if not (link.exists() or link.is_symlink()):
                link.symlink_to(entry.resolve())
        bin_dir.mkdir()
        for path in sorted((pinned / "bin").iterdir()):
            if path.is_file():
                shutil.copy2(path, bin_dir / path.name)
    for name, path in staged.items():
        target = find_ci(bin_dir, name) or bin_dir / name
        shutil.copy2(path, target)
    if not current():
        raise ValueError(f"{NAME}: overlay at {msvc} does not hold the pinned passes")
    return msvc


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--source", help="directory with the staged RTM binaries")
    parser.add_argument("--force", action="store_true", help="rebuild the overlay")
    args = parser.parse_args(argv)
    try:
        msvc = build(args.source, args.force)
    except ValueError as exc:
        print(f"[init] {exc}", file=sys.stderr)
        return 1
    print(f"[init] {NAME} overlay: {msvc}")
    return 0


def logged_main(argv: list[str] | None = None) -> int:
    import shlex
    from homm3.core.usage import append, run_logged
    argv = list(sys.argv[1:] if argv is None else argv)
    command = shlex.join(["python3", "-m", "homm3.init.vc6_rtm", *argv])
    return run_logged(main, argv, lambda rc, **meta: append(
        common.HOMM3_DIR / "build/homm3_usage.log", command, rc, **meta), failure_rc=1)


if __name__ == "__main__":
    sys.exit(logged_main())
