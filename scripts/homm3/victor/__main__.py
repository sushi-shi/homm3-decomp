"""Prepare an exhaustive installed-bitmap corpus for the Victor oracle."""
import argparse
import json
import math
import subprocess
import sys
from pathlib import Path

from .corpus import inventory, bitmap_to_pcx, sha256


def main():
    parser = argparse.ArgumentParser(prog='homm3 victor')
    sub = parser.add_subparsers(dest='command', required=True)
    command = sub.add_parser('corpus', help='inventory and encode installed bitmap resources')
    command.add_argument('--game-dir', type=Path, required=True)
    command.add_argument('--out', type=Path, required=True)
    command = sub.add_parser('compare', help='run retail and candidate against a prepared corpus')
    command.add_argument('--corpus', type=Path, required=True)
    command.add_argument('--out', type=Path, required=True)
    command.add_argument('--limit', type=int)
    command.add_argument('--allocation', choices=('global', 'dib'), default='global')
    command.add_argument('--timeout', type=float, default=600)
    args = parser.parse_args()
    if args.command == 'compare':
        from .compare import compare
        if args.limit is not None and args.limit < 1:
            parser.error('--limit must be positive')
        if not math.isfinite(args.timeout) or args.timeout <= 0:
            parser.error('--timeout must be positive and finite')
        return compare(args.corpus, args.out, args.limit, args.timeout, args.allocation)
    game = args.game_dir.resolve(strict=True)
    if not game.is_dir():
        parser.error('--game-dir must be a directory')
    args.out.mkdir(parents=True, exist_ok=False)
    report = inventory(game, args.out)
    encoded = args.out / 'pcx'
    encoded.mkdir()
    for resource in report['images']:
        kind = resource['format']
        if kind == 'unknown':
            continue
        path = encoded / (resource['sha256'] + '.pcx')
        if not path.exists():
            original = (args.out / resource['asset']).read_bytes()
            path.write_bytes(original if kind == 'pcx' else bitmap_to_pcx(original))
        resource['pcx'] = str(path.relative_to(args.out))
        resource['pcxSha256'] = sha256(path.read_bytes())
        resource['inputOrigin'] = 'original' if kind == 'pcx' else 'lossless-transcode'
    (args.out / 'inventory.json').write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(report['counts'], sort_keys=True))
    print(f"{len(report['images'])} resource occurrences; {len(list(encoded.iterdir()))} distinct inputs")
    return int(bool(report['counts'].get('unknown')))


if __name__ == '__main__':
    try:
        raise SystemExit(main())
    except (ValueError, OSError, subprocess.SubprocessError) as error:
        print(f'[victor] ERROR: {error}', file=sys.stderr)
        raise SystemExit(2)
