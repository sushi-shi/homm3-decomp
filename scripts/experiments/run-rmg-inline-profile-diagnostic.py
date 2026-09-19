#!/usr/bin/env python3
"""Disposable /Ob1 versus /Ob2 diagnostic; never changes the live profile.

The marker is experiment metadata in a source comment, not a compiler pragma.
Every unit in a candidate uses the same explicit inline profile. Actual argv
is recorded beside each raw object; all other admitted flags are preserved.
"""
import hashlib
import json
import os
import subprocess
import sys
from pathlib import Path

from homm3.vc6 import source_families

MARKER = '// HOMM3_ISOLATED_PROFILE_DIAGNOSTIC: /Ob1\n'


def compile_candidate(candidate_root, unit, output):
    source = source_families.source_for_unit(unit)
    flags = list(source_families.flags_for_unit(unit))
    assert flags.count('/Ob2') == 1 and '/Ob1' not in flags
    marked = (candidate_root / 'src/rmg.cpp').read_text().count(MARKER)
    assert marked in (0, 1)
    if marked:
        flags[flags.index('/Ob2')] = '/Ob1'
    output.mkdir(parents=True, exist_ok=True)
    obj = output / 'candidate.obj'
    root = source_families.common.HOMM3_DIR
    env = dict(os.environ, HOMM3_DIR=str(candidate_root), PYTHONPATH=str(root / 'scripts'))
    command = [sys.executable, '-m', 'homm3.core.cc_wrap', '--out', str(obj),
               '--src', str(candidate_root / source.relative_to(root)), '--', *flags]
    (output / 'diagnostic-argv.json').write_text(json.dumps(command, indent=2) + '\n')
    proc = subprocess.run(command, capture_output=True, text=True, env=env)
    log = proc.stdout + proc.stderr
    (output / 'compile.log').write_text(log)
    if proc.returncode or not obj.is_file():
        raise RuntimeError(log[-6000:])
    return obj


def main():
    payload = json.loads(Path(sys.argv[1]).read_text())
    assert payload['diagnostic_runner_sha256'] == hashlib.sha256(Path(__file__).read_bytes()).hexdigest()
    assert payload['units'] == ['rmg', 'rmg_support', 'rmg_terrain']
    source_families.compile_candidate = compile_candidate
    return source_families.main()


if __name__ == '__main__':
    raise SystemExit(main())
