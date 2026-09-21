"""Run the recovered RMG beside the pinned retail image in fresh Wine processes."""
from __future__ import annotations

import argparse
from datetime import datetime, timezone
import json
import hashlib
import math
import os
from pathlib import Path
import shutil
import signal
import struct
import subprocess
import sys
import time
import uuid

from homm3.core import inputs
from homm3.core.cc_wrap import find_ci, msvc_dir, winepath_w
from homm3.core.common import HOMM3_DIR as ROOT
from homm3.core.project import Project
from .bindings import DRIVER_BASE, prepare_calls, resolve, absolute_object
from .bootstrap import patch_winmain
from .cases import (load_cases, validate_case, job_bytes, digest, compare_runs,
                    decode_result)

UNITS = ('rmg', 'rmg_support', 'rmg_terrain')
DLLS = ('BINKW32.DLL', 'MSS32.DLL', 'SMACKW32.DLL', 'IFC20.dll')


def unsigned(value: str, maximum: int, option: str) -> int:
    try:
        result = int(value, 0)
    except ValueError:
        raise argparse.ArgumentTypeError(f'{option} must be an integer')
    if not 0 <= result <= maximum:
        raise argparse.ArgumentTypeError(f'{option} must be in [0, {maximum}]')
    return result


def selected_cases(path: Path | None, seed: int | None,
                   stack_word: int | None, heap_byte: int | None) -> list[dict]:
    if path is not None:
        if stack_word is not None or heap_byte is not None:
            raise ValueError('--stack-word and --heap-byte require --seed')
        return load_cases(path)
    return [validate_case({'name': f'seed-{seed:08x}', 'seed': seed,
                           'stackWord': stack_word or 0,
                           'heapByte': heap_byte or 0})]


def write_json(path: Path, value) -> None:
    path.write_text(json.dumps(value, indent=2, sort_keys=True) + '\n')


def file_digest(path: Path) -> str:
    with path.open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def run_command(command: list[str], log: Path) -> None:
    with log.open('ab') as stream:
        stream.write((json.dumps(command) + '\n').encode())
        stream.flush()
        subprocess.run(command, cwd=ROOT, stdout=stream, stderr=subprocess.STDOUT, check=True)


def driver_include_flags() -> list[str]:
    """Spell include roots explicitly for the standalone oracle driver.

    Some Wine/VC6 environments preserve only the first semicolon-delimited
    INCLUDE entry. Normal game TUs deliberately retain their established
    compiler command line; only this new standalone source needs the fallback.
    """
    includes = [msvc_dir() / 'include',
                *(path for path in Project(ROOT).includes if path.is_dir())]
    return ['/I' + winepath_w(path) for path in includes]


def build_driver(out: Path, retail: bytes) -> dict:
    # Ninja owns per-TU compiler profiles and header dependency freshness.
    run_command([sys.executable, '-m', 'homm3.build.configure'], out / 'build.log')
    objects = [ROOT / 'build/objdiff/base' / f'{unit}.obj' for unit in UNITS]
    run_command(['ninja', *(str(path.relative_to(ROOT)) for path in objects)], out / 'build.log')
    run_command([sys.executable, '-m', 'homm3.model'], out / 'build.log')
    driver = out / 'driver.obj'
    run_command([sys.executable, '-m', 'homm3.core.cc_wrap', '--src',
                 str(Path(__file__).with_name('driver.cpp')), '--out', str(driver),
                 '--', '/nologo', '/c', '/O2', '/MT', *driver_include_flags()],
                out / 'build.log')
    # Freeze link inputs before resolving names, hashing, or relocating them.
    # A concurrent build cannot silently change the recorded candidate.
    archived = []
    for path in objects:
        copy = out / path.name
        shutil.copyfile(path, copy)
        archived.append(copy)
    originals = [driver, *archived]
    bindings = resolve(ROOT, originals, retail)
    write_json(out / 'bindings.json', {name: f'0x{va:08x}' for name, va in bindings.items()})
    bridge = out / 'retail-bridge.obj'
    bridge.write_bytes(absolute_object(bindings))
    copies = []
    for path in originals:
        copy = out / ('link-' + path.name)
        copy.write_bytes(prepare_calls(path.read_bytes(), bindings))
        copies.append(copy)
    toolchain = msvc_dir()
    run_command(['wine', str(find_ci(toolchain / 'bin', 'link.exe')), '/NOLOGO',
                 '/DLL', '/NOENTRY', '/NODEFAULTLIB', '/INCREMENTAL:NO', '/FIXED',
                 f'/BASE:0x{DRIVER_BASE:x}', '/EXPORT:run=_run@0',
                 '/MAP:' + winepath_w(out / 'driver.map'),
                 '/OUT:' + winepath_w(out / 'rmg-driver.dll'),
                 *(winepath_w(path) for path in copies), winepath_w(bridge),
                 winepath_w(find_ci(toolchain / 'lib', 'kernel32.lib'))], out / 'build.log')
    return {path.name: digest(path.read_bytes()) for path in originals}


def run_case(out: Path, name: str, mode: str, job: bytes, data: Path,
             libraries: list[Path], timeout: float) -> dict:
    directory = out / name
    directory.mkdir()
    for path in (out / 'rmg-host.exe', out / 'rmg-driver.dll', *libraries):
        (directory / path.name).symlink_to(path)
    (directory / 'job.bin').write_bytes(job)
    env = dict(os.environ, RMG_JOB=winepath_w(directory / 'job.bin'),
               RMG_DATA=winepath_w(data) + '\\', RMG_MODE=mode,
               # Disable Wine's interactive debugger if a fault escapes before
               # the driver's SEH record is written.
               WINEDLLOVERRIDES='winedbg.exe=d')
    started = time.monotonic()
    with (directory / 'wine.log').open('wb') as stream:
        process = subprocess.Popen(['wine', str(directory / 'rmg-host.exe')],
                                   cwd=directory, env=env, stdout=stream,
                                   stderr=subprocess.STDOUT, start_new_session=True)
        try:
            code = process.wait(timeout=timeout)
            result = {'status': 'ok' if code == 0 else 'process-error', 'exitCode': code}
        except subprocess.TimeoutExpired:
            try:
                os.killpg(process.pid, signal.SIGKILL)
            except ProcessLookupError:
                pass
            process.wait()
            result = {'status': 'timeout'}
    result['seconds'] = round(time.monotonic() - started, 3)
    failure = directory / 'failure.bin'
    if failure.exists():
        raw = failure.read_bytes()
        if len(raw) == 12:
            code, address, stage = struct.unpack('<III', raw)
            result.update(status='crash', exception=f'0x{code:08x}', address=f'0x{address:08x}', stage=stage)
    if result['status'] == 'ok':
        try:
            state = decode_result((directory / 'result.bin').read_bytes())
            if not (directory / 'map.raw').is_file():
                raise ValueError('missing map.raw')
            result['state'] = state
        except (ValueError, OSError) as error:
            result.update(status='invalid-output', error=str(error))
    write_json(directory / 'run.json', result)
    print(f'[rmg] {name}: {result["status"]} ({result["seconds"]}s)', flush=True)
    return result


def compare(args) -> int:
    cases = selected_cases(args.cases, args.seed, args.stack_word, args.heap_byte)
    if args.game_dir is None:
        raise ValueError('pass --game-dir or set HOMM3_GAME_DIR')
    game = args.game_dir.resolve(strict=True)
    data = find_ci(game, 'data')
    if data is None or not data.is_dir():
        raise ValueError('game directory has no Data directory')
    libraries = [find_ci(game, name) for name in DLLS]
    if any(path is None for path in libraries):
        raise ValueError(f'game directory must contain {", ".join(DLLS)}')
    out = (args.out or ROOT / 'build/rmg-runs' /
           (datetime.now(timezone.utc).strftime('%Y%m%dT%H%M%SZ') + '-' + uuid.uuid4().hex[:8])).resolve()
    out.mkdir(parents=True, exist_ok=False)
    print(f'[rmg] artifacts: {out}', flush=True)
    os.environ.setdefault('WINEPREFIX', str(ROOT / 'build/wineprefix'))
    os.environ.setdefault('WINEDEBUG', '-all')
    retail = inputs.read_verified(inputs.RETAIL, inputs.RETAIL.destination)
    host = patch_winmain(retail)
    (out / 'rmg-host.exe').write_bytes(host)
    frozen_libraries = []
    for path in libraries:
        copy = out / path.name
        shutil.copyfile(path, copy)
        frozen_libraries.append(copy)
    libraries = frozen_libraries
    write_json(out / 'cases.json', cases)
    provenance = {'retailSha256': digest(retail), 'patchedHostSha256': digest(host),
                  'gameDirectory': str(game),
                  'winePrefix': os.environ['WINEPREFIX'], 'timeoutSeconds': args.timeout,
                  'wineVersion': subprocess.check_output(['wine', '--version'], text=True).strip(),
                  'data': {str(path.relative_to(data)): file_digest(path)
                           for path in sorted(data.rglob('*')) if path.is_file()},
                  'libraries': {path.name: file_digest(path) for path in libraries},
                  'driverSources': {path.name: digest(path.read_bytes())
                                    for path in Path(__file__).parent.iterdir()
                                    if path.suffix in ('.cpp', '.py')},
                  'unitsManifestSha256': digest((ROOT / 'config/units.toml').read_bytes())}
    write_json(out / 'provenance.json', provenance)
    provenance['objects'] = build_driver(out, retail)
    provenance['driverSha256'] = digest((out / 'rmg-driver.dll').read_bytes())
    write_json(out / 'provenance.json', provenance)
    report = {'cases': [], 'equal': True, 'executionErrors': False}
    for case in cases:
        name = case['name']
        entry = {'name': name, 'runs': {}}
        for label, mode in (('retail', 'retail'), ('retail-repeat', 'retail'),
                            ('candidate', 'candidate'), ('candidate-repeat', 'candidate')):
            entry['runs'][label] = run_case(out, f'{name}-{label}', mode,
                                           job_bytes(case), data, libraries, args.timeout)
        okay = all(run['status'] == 'ok' for run in entry['runs'].values())
        if okay:
            for label, left, right in (('retailRepeatability', 'retail', 'retail-repeat'),
                                       ('candidateRepeatability', 'candidate', 'candidate-repeat'),
                                       ('comparison', 'retail', 'candidate')):
                entry[label] = compare_runs(out / f'{name}-{left}', out / f'{name}-{right}')
            entry['equal'] = all(entry[label]['equal'] for label in
                                 ('retailRepeatability', 'candidateRepeatability', 'comparison'))
        else:
            entry['equal'] = False
            report['executionErrors'] = True
        report['equal'] &= entry['equal']
        report['cases'].append(entry)
        write_json(out / 'report.json', report)
        print(f'[rmg] {name}: {"equal" if entry["equal"] else "differs or failed"}', flush=True)
    print(f'[rmg] report: {out / "report.json"}', flush=True)
    return 2 if report['executionErrors'] else (0 if report['equal'] else 1)


def main(argv=None) -> int:
    parser = argparse.ArgumentParser(prog='homm3 rmg')
    sub = parser.add_subparsers(dest='command', required=True)
    command = sub.add_parser('compare', help='build and run fresh retail/candidate whole-map comparisons')
    game_default = Path(os.environ['HOMM3_GAME_DIR']) if os.environ.get('HOMM3_GAME_DIR') else None
    command.add_argument('--game-dir', type=Path, default=game_default,
                         help='installed game directory (or set HOMM3_GAME_DIR)')
    inputs = command.add_mutually_exclusive_group(required=True)
    inputs.add_argument('--cases', type=Path, help='JSON array of named map requests')
    inputs.add_argument('--seed', type=lambda value: unsigned(value, 0xffffffff, '--seed'),
                        help='run one default 36x36 request; accepts decimal or 0xHEX')
    command.add_argument('--stack-word', type=lambda value: unsigned(value, 0xffffffff, '--stack-word'),
                         help='initial stack word for --seed (default: 0)')
    command.add_argument('--heap-byte', type=lambda value: unsigned(value, 255, '--heap-byte'),
                         help='allocation fill byte for --seed (default: 0)')
    command.add_argument('--out', type=Path, help='new artifact directory (default: build/rmg-runs/TIMESTAMP)')
    command.add_argument('--timeout', type=float, default=300, help='seconds per child process (default: 300)')
    args = parser.parse_args(argv)
    if not math.isfinite(args.timeout) or args.timeout <= 0:
        parser.error('--timeout must be positive')
    try:
        return compare(args)
    except (ValueError, OSError, subprocess.SubprocessError) as error:
        print(f'[rmg] ERROR: {error}', file=sys.stderr)
        return 2


if __name__ == '__main__':
    sys.exit(main())
