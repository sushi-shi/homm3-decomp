"""Opt-in, alternating CLI benchmarks; no instrumentation in ordinary commands.

python -m homm3.core.perf --baseline /tmp/base --candidate /tmp/candidate \
    --scenario full --repeat 7 --output /tmp/results [--profile]

Use prepared disposable worktrees: full builds write their normal checkpoints.
Profiling is a separate run, never included in timing distributions. The
generated startup hook loads this file directly so baseline imports still come
from the baseline checkout, even when it predates this measurement tool.
"""
from __future__ import annotations

import argparse
import atexit
from collections import Counter
import cProfile
import hashlib
import json
import os
from pathlib import Path
import platform
import pstats
import shutil
import statistics
import subprocess
import sys
import threading
import time

SCENARIOS = {
    'full': ['build'], 'fast': ['build', '--fast', 'bitmap16'],
    'status': ['status'],
    'sema': ['sema', 'diff', '0x00554400', '--summary'],
    'source': ['sema', 'diff', '0x00554400', '--source'],
    'audit-one': ['dreamcast', 'audit', '0x00554400', '--json'],
    'audit-bitmap16': ['dreamcast', 'audit', '--module', 'bitmap16', '--json'],
    'audit-winmgr': ['dreamcast', 'audit', '--module', 'winmgr', '--json'],
    'source-edit-fast': ['build', '--fast', 'bitmap16'],
    'source-edit-full': ['build'], 'header-edit': ['build'],
    'merge-source': ['build'], 'merge-header': ['build'], 'cold': ['build'],
}


def install_profile():
    """Startup hook for each Python interpreter and its ordinary worker threads."""
    output = Path(os.environ['HOMM3_PERF_PROFILE'])
    if sys.version_info < (3, 13):
        raise RuntimeError('whole-process profiling requires the pinned Python 3.13+')
    thread_ids = {threading.get_ident()}
    launches = []
    original_run = threading.Thread.run
    original_popen = subprocess.Popen

    def worker(self):
        thread_ids.add(threading.get_ident())
        return original_run(self)

    class TrackedPopen(original_popen):
        def __init__(self, args, *a, **kw):
            started = time.monotonic()
            super().__init__(args, *a, **kw)
            self.measurement = {'args': args if isinstance(args, str) else list(map(str, args)),
                                'pid': self.pid, 'started': started}
            launches.append(self.measurement)

        def completed(self):
            if self.returncode is not None and 'seconds' not in self.measurement:
                self.measurement.update(seconds=time.monotonic()-self.measurement['started'],
                                        exit_code=self.returncode)

        def wait(self, *a, **kw):
            result = super().wait(*a, **kw)
            self.completed()
            return result

        def poll(self):
            result = super().poll()
            self.completed()
            return result

    threading.Thread.run = worker
    subprocess.Popen = TrackedPopen
    main = cProfile.Profile()

    def finish():
        main.disable()
        main.dump_stats(str(output / f'{os.getpid()}.prof'))
        (output / f'{os.getpid()}.json').write_text(json.dumps({
            'pid': os.getpid(), 'argv': sys.argv, 'threads': len(thread_ids),
            'launches': launches}, default=str))
    atexit.register(finish)
    main.enable()


def profile_environment(output: Path, env: dict) -> dict:
    output.mkdir(parents=True, exist_ok=False)
    hook = output / 'startup'
    hook.mkdir()
    (hook / 'sitecustomize.py').write_text(
        'import importlib.util\n'
        f'spec=importlib.util.spec_from_file_location("_homm3_perf", {str(Path(__file__).resolve())!r})\n'
        'module=importlib.util.module_from_spec(spec)\n'
        'spec.loader.exec_module(module)\nmodule.install_profile()\n')
    # Ninja recipes explicitly set PYTHONPATH=scripts. Restore our startup hook
    # when they launch python3; use the real executable to avoid recursion.
    binary = output / 'bin'
    binary.mkdir()
    wrapper = binary / 'python3'
    wrapper.write_text(f'#!{sys.executable}\nimport os,sys\n'
        f'os.environ["PYTHONPATH"]={str(hook)!r}+os.pathsep+os.environ.get("PYTHONPATH", "")\n'
        f'os.execv({sys.executable!r}, [{sys.executable!r}, *sys.argv[1:]])\n')
    wrapper.chmod(0o755)
    return dict(env, HOMM3_PERF_PROFILE=str(output),
                PYTHONPATH=str(hook) + os.pathsep + env['PYTHONPATH'],
                PATH=str(binary) + os.pathsep + env['PATH'])


def summarize_profiles(directory: Path) -> dict:
    functions = {}
    for path in directory.glob('*.prof'):
        for (file, line, name), (primitive, calls, own, cumulative, _) in pstats.Stats(str(path)).stats.items():
            key = (file, line, name)
            row = functions.setdefault(key, [0, 0, 0., 0.])
            for index, value in enumerate((primitive, calls, own, cumulative)):
                row[index] += value
    rows = [dict(file=f, line=l, function=n, primitive=v[0], calls=v[1],
                 self_seconds=v[2], cumulative_seconds=v[3])
            for (f, l, n), v in functions.items()]
    processes = [json.loads(p.read_text()) for p in directory.glob('*.json')
                 if p.name != 'summary.json']
    launches = Counter()
    for process in processes:
        for launch in process['launches']:
            args = launch['args']
            launches[Path(args[0]).name if isinstance(args, list) else args] += 1
    result = dict(python_calls=sum(r['calls'] for r in rows if r['file'] != '~'),
                  python_primitive=sum(r['primitive'] for r in rows if r['file'] != '~'),
                  c_calls=sum(r['calls'] for r in rows if r['file'] == '~'),
                  processes=processes, launches=dict(launches),
                  coverage='CPython 3.13 monitoring covers Python threads; startup hook covers Python children. Interpreter/profiler setup and native child internals excluded. Inclusive profile times overlap; CLI time is measured separately.',
                  functions=sorted(rows, key=lambda r: r['cumulative_seconds'], reverse=True))
    (directory / 'summary.json').write_text(json.dumps(result, indent=2))
    return {key: value for key, value in result.items() if key != 'functions'}


def run(root: Path, scenario: str, output: Path, profile=False) -> dict:
    from homm3.core.perf_workflows import merged, cold
    from contextlib import nullcontext
    preparation = (merged(root, scenario=='merge-header', output.stem.rsplit('-', 1)[-1])
                   if scenario.startswith('merge-') else
                   cold(root, output.with_suffix('.worktree')) if scenario=='cold' else nullcontext(root))
    start = time.perf_counter()
    with preparation as prepared:
        preparation_seconds = time.perf_counter() - start
        result = _run(prepared, scenario, output, profile)
        result['preparation_seconds'] = preparation_seconds
        return result


def _run(root: Path, scenario: str, output: Path, profile=False) -> dict:
    env = dict(os.environ, HOMM3_DIR=str(root), PYTHONPATH=str(root / 'scripts'),
               HOMM3_USAGE_TEST='perf', PYTHONUNBUFFERED='1')
    if profile:
        env = profile_environment(output.with_suffix('.profiles'), env)
    edit = ('include/rmg.h' if scenario == 'header-edit' else
            'src/bitmap16.cpp' if scenario.startswith('source-edit-') else None)
    original = (root / edit).read_bytes() if edit else None
    try:
        if edit:
            with (root / edit).open('ab') as stream:
                # Equal edits for each paired baseline/candidate measurement,
                # distinct from the previous iteration's ownership cache key.
                label = output.stem.rsplit('-', 1)[-1]
                stream.write(f'\n// Temporary performance workload {label}.\n'.encode())
        start = time.perf_counter_ns()
        with output.open('wb') as log:
            with subprocess.Popen([sys.executable, '-m', 'homm3', *SCENARIOS[scenario]],
                                  cwd=root, env=env, stdout=log, stderr=subprocess.STDOUT) as process:
                _, status, usage = os.wait4(process.pid, 0)
                process.returncode = os.waitstatus_to_exitcode(status)
        elapsed = (time.perf_counter_ns() - start) / 1e9
        result = dict(root=str(root), scenario=scenario, seconds=elapsed,
                      exit_code=process.returncode, log=str(output),
                      user_seconds=usage.ru_utime, system_seconds=usage.ru_stime,
                      children_maxrss_kb=usage.ru_maxrss,
                      memory_note='wait4 command-tree maximum RSS, not aggregate concurrent RSS')
        allowed = {0, 1, 2} if scenario.startswith('audit-') else {0, 1} if scenario in ('sema', 'source') else {0}
        if process.returncode not in allowed:
            raise RuntimeError(f'{scenario} failed with exit {process.returncode}; see {output}')
        if profile:
            result['profile'] = summarize_profiles(output.with_suffix('.profiles'))
        return result
    finally:
        if edit:
            (root / edit).write_bytes(original)


def metadata(root):
    def command(*args):
        return subprocess.run(args, cwd=root, text=True, capture_output=True).stdout.strip()
    versions = {name: command(name, '--version').splitlines()[:1]
                for name in ('clang', 'ninja', 'wine', 'git', 'objdiff-cli') if shutil.which(name)}
    return dict(commit=command('git', 'rev-parse', 'HEAD'),
                runner_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
                diff_sha256=hashlib.sha256(command('git', 'diff', 'HEAD').encode()).hexdigest(),
                python=sys.version, platform=platform.platform(), cpu_count=os.cpu_count(),
                workers={'labels': min(16, os.cpu_count() or 4), 'ownership': 4,
                         'ninja': 'default'},
                versions=versions)


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--baseline', type=Path, required=True)
    parser.add_argument('--candidate', type=Path, required=True)
    parser.add_argument('--scenario', choices=SCENARIOS, action='append', required=True)
    parser.add_argument('--repeat', type=int, default=7)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--profile', action='store_true')
    args = parser.parse_args(argv)
    if args.repeat < 2:
        parser.error('--repeat must be at least 2')
    roots = [args.baseline.resolve(), args.candidate.resolve()]
    if roots[0] == roots[1] or any(not (root / '.git').is_file() for root in roots):
        parser.error('use two distinct disposable Git worktrees')
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=False)
    payload = dict(schema=1, metadata=[metadata(root) for root in roots],
                   preparation='one unmeasured warm-up per scenario/root; compiler/toolchain already provisioned',
                   runs=[], summary={})
    def save():
        (output / 'results.json').write_text(json.dumps(payload, indent=2))
    save()
    for scenario in args.scenario:
        for side, root in ([] if scenario == 'cold' else enumerate(roots)):
            warm = run(root, scenario, output / f'{scenario}-{side}-warm.log')
            payload['runs'].append(dict(warm, warmup=True))
            save()
        for index in range(3 if scenario == 'cold' else args.repeat):
            for side in ([0, 1] if index % 2 == 0 else [1, 0]):
                result = run(roots[side], scenario, output / f'{scenario}-{side}-{index}.log')
                payload['runs'].append(dict(result, side=side, iteration=index, warmup=False))
                save()
                print(scenario, side, index, f"{result['seconds']:.3f}s", 'rc', result['exit_code'], flush=True)
        values = [[r['seconds'] for r in payload['runs'] if r['scenario']==scenario
                   and not r['warmup'] and r['side']==side] for side in (0,1)]
        payload['summary'][scenario] = {
            'median_seconds': list(map(statistics.median, values)),
            'quartiles_seconds': [statistics.quantiles(v, n=4, method='inclusive') for v in values],
            'paired_reduction_percent': statistics.median([(a-b)/a*100 for a,b in zip(*values)])}
        if args.profile:
            for side, root in enumerate(roots):
                payload.setdefault('profiles', []).append(run(root, scenario, output / f'{scenario}-{side}-profile.log', True))
        save()
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
