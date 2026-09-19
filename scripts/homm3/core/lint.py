"""Run Ruff and Pyright on the reviewed tooling modules.

Run inside `nix develop .#build`: python -m homm3.core.lint
The explicit scope can grow as other modules become lint/type clean.
"""
from __future__ import annotations

import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile

from homm3.core import common

MODULES = (
    'analysis/source_facts.py',
    'build/build.py',
    'build/delink.py',
    'build/normalize_objs.py',
    'build/normalized_freshness.py',
    'core/lint.py',
    'match/banked_rows.py',
    'match/source_ownership.py',
)
TESTS = ('analysis/test_source_facts.py', 'build/test_build.py', 'core/test_lint.py')


def main() -> int:
    root = common.HOMM3_DIR.resolve()
    sources = [root / 'scripts/homm3' / name for name in MODULES]
    tests = [root / 'scripts/homm3' / name for name in TESTS]
    lint = subprocess.run(['ruff', 'check', *map(str, sources + tests)], cwd=root)
    with tempfile.TemporaryDirectory(prefix='homm3-pyright-') as directory:
        config = Path(directory) / 'pyrightconfig.json'
        config.write_text(json.dumps({
            'include': [os.path.relpath(path, directory) for path in sources],
            # Nix's wrapped Python is not always discoverable by Pyright.
            # Use this interpreter's actual dependency paths, not a pinned
            # store path or another worktree's PYTHONPATH.
            'extraPaths': [str(root / 'scripts'),
                           *[p for p in sys.path if p.endswith('site-packages')]],
            'pythonVersion': f'{sys.version_info.major}.{sys.version_info.minor}',
        }))
        types = subprocess.run(['pyright', '--project', str(config)], cwd=root)
    return int(bool(lint.returncode or types.returncode))


if __name__ == '__main__':
    raise SystemExit(main())
