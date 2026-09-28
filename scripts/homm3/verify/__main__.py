"""Gruntz data-verification commands, using HoMM3's one model."""
import argparse
import importlib
import sys

COMMANDS = {
    'data-relocs': 'data_relocs',
    'data-access': 'data_access',
    'data-coverage': 'data_coverage',
    'data-tu-order': 'data_tu_order',
    'library-data-refs': 'library_data_refs',
    'library-code': 'library_code',
}


def main(argv=None):
    argv = list(sys.argv[1:] if argv is None else argv)
    if argv and argv[0] in COMMANDS:
        return importlib.import_module('homm3.verify.' + COMMANDS[argv[0]]).main(argv[1:])
    parser = argparse.ArgumentParser(prog='homm3 verify', description=__doc__)
    parser.add_argument('command', choices=COMMANDS)
    parser.parse_args(argv)
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
