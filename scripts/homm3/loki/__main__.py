"""homm3 loki: the pinned GCC 2.95.2 toolchain of Loki's Linux builds.

  toolchain [--debs DIR] [--sgi-stl DIR] [--binutils DIR] [--gcc DIR] [--gtk DIR]
        stage or verify the toolchain (config/loki/toolchain.toml) under
        build/loki/toolchain/ and print the driver version
"""
from __future__ import annotations

import argparse
import sys


def main(argv=None) -> int:
    parser = argparse.ArgumentParser(prog="homm3 loki", description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = parser.add_subparsers(dest="command", required=True)
    p = sub.add_parser("toolchain", help="stage or verify the GCC 2.95.2 toolchain")
    p.add_argument("--debs", help="directory of the pinned Debian potato packages")
    p.add_argument("--sgi-stl", help="directory holding SGI STL 3.2 stl32.tar.gz")
    p.add_argument("--binutils", help="directory holding Slackware 7.1 binutils.tgz (as 2.9.1.0.25)")
    p.add_argument("--gcc", help="directory holding Slackware 7.1 contrib gcc.tgz (vanilla 2.95.2)")
    p.add_argument("--gtk", help="directory holding Slackware 7.1 gtkglib.tgz (GTK+/GLib 1.2.8 headers)")
    args = parser.parse_args(argv)
    from homm3.loki import toolchain
    try:
        toolchain.stage(args.debs, args.sgi_stl, args.binutils, args.gcc, args.gtk)
        print(f"[loki] g++ {toolchain.version()} staged at {toolchain.DESTINATION}")
    except (toolchain.ToolchainError, ValueError, OSError) as exc:
        print(f"[loki] ERROR: {exc}", file=sys.stderr)
        return 2
    return 0


def logged_main(argv=None) -> int:
    from homm3.core import paths
    from homm3.core.usage import append, run_logged
    import shlex
    argv = list(sys.argv[1:] if argv is None else argv)
    cmd = shlex.join(["homm3", "loki", *argv])
    return run_logged(main, argv,
                      lambda rc, **meta: append(paths.SHARED_BUILD / "homm3_usage.log",
                                                cmd, rc, **meta))


if __name__ == "__main__":
    raise SystemExit(logged_main())
