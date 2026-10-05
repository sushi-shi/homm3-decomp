"""RMG rainbow tables: resumable native reference results for Rust checks.

Precomputed C++ outcomes let a check run only the Rust candidate; no Rust
output ever becomes a reference. See docs/tooling/rmg-rainbow-tables.md.

homm3 rmg rainbow --help   (alias: python -m homm3.rmg.corpus --help)
"""
from __future__ import annotations

import argparse
from collections import Counter
from contextlib import contextmanager
import fcntl
import hashlib
import json
import os
from pathlib import Path
import platform
import random
import re
import select
import shutil
import sqlite3
import struct
import subprocess
import time
import uuid

from .cases import FIELDS, decode_result, job_bytes, load_cases, validate_case

SCHEMA = 1
MODES = ('retail', 'hotfix')
ARCHIVES = ('h3bitmap.lod', 'h3sprite.lod', 'h3ab_bmp.lod', 'h3ab_spr.lod')
# `extend` reuses the request distribution of the imported sample2 campaign
# (build/rmg-oracle/sample_campaign_100k.py, master seed 0x524d4732). Bump the
# version for any change to sample_case or extension_cases: recorded
# extensions must stay reproducible.
SAMPLER = 'sample2-extend-v1'
EXTEND_SEED = 0x524d4733
# The retail driver imported from the sample2 campaign predates batching: it
# runs only the first job and writes map.raw/result.bin. The hotfix driver
# runs several jobs per Wine process.
BATCH_SIZE = {'retail': 1, 'hotfix': 32}


def packed(value):
    return json.dumps(value, sort_keys=True, separators=(',', ':'))


def digest(path):
    with Path(path).open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def assets(data):
    # Native loose resources can override archives. Include them, and detect
    # both additions and removals rather than only checking previously used files.
    files = {}
    for p in data.iterdir():
        if p.is_file() and p.suffix.lower() not in ('.snd', '.vid'):
            key = p.name.lower()
            if key in files:
                raise ValueError(f'ambiguous asset name: {p.name}')
            files[key] = digest(p)
    if not set(ARCHIVES) <= files.keys():
        raise ValueError('missing required installation archives')
    return files


def request_hex(request):
    return struct.pack('<8B18i', *request['isHumanSeat'], *request['townType'],
                       *(request[k] for k in FIELDS)).hex()


def replay(case, mode):
    behavior = 'hotfix'
    if mode == 'retail':
        if case['heapByte'] is None:
            raise ValueError('retail replay requires a controlled allocation fill')
        signed_stack = struct.unpack('<i', struct.pack('<I', case['stackWord']))[0]
        # This is the frozen harness policy, not a general engine default.
        behavior = (f"retail {case['stackWord']} {case['heapByte']} "
                    f"{int(case['heapByte'] != 0)} {signed_stack} none")
    return (f"homm3-rmg-replay 2 seed {case['seed']} behavior {behavior} seats "
            + ' '.join(map(str, case['isHumanSeat'])) + ' towns '
            + ' '.join(map(str, case['townType']))
            + f" shape {case['width']} {case['height']} {case['levels']} players "
            + ' '.join(str(case[k]) for k in FIELDS[3:7]) + ' settings '
            + ' '.join(str(case[k]) for k in FIELDS[7:]) + '\n')


def sample_case(rng, index, seed):
    """One request with the sample2 campaign's distribution.

    Map size, levels, map version, water and monster strength cycle with the
    case index (period 480). Players, teams, seats, fixed towns, the stack
    word and the heap byte come from ``rng`` in the original draw order. The
    original iterated small-integer sets, whose CPython order is ascending;
    the explicit sorts keep that order without relying on it.
    """
    size = (36, 72, 108, 144)[index % 4]
    version = (index // 8) % 3
    total = rng.randrange(2, 9)
    humans = rng.randrange(1, total + 1)
    computers = total - humans
    human_seats = set(rng.sample(range(8), humans))
    active = set(rng.sample(range(8), total)) | human_seats
    for seat in sorted(active - human_seats)[:len(active) - total]:
        active.remove(seat)
    towns = [-1] * 8
    for seat in sorted(active):
        if rng.randrange(4) == 0:
            towns[seat] = rng.randrange((8 if version >= 1 else 7) + 1)
    return validate_case({
        'name': f'sample2-{index:06d}', 'seed': seed,
        'width': size, 'height': size, 'levels': 1 + (index // 4) % 2,
        'mapVersion': version, 'waterContent': (index // 24) % 4,
        'monsterStrength': (index // 96) % 5 - 2,
        'humanPlayerCount': humans, 'computerPlayerCount': computers,
        'humanTeamCount': rng.randrange(humans + 1),
        'computerTeamCount': rng.randrange(computers + 1),
        'isHumanSeat': [int(seat in human_seats) for seat in range(8)],
        'townType': towns, 'stackWord': rng.getrandbits(32),
        'heapByte': rng.randrange(256),
    })


def extension_cases(cases, count, master_seed=EXTEND_SEED):
    """Append-only continuation of the sample2 sequence.

    Each case has its own generator keyed by the master seed and its index,
    so one extension by 2N equals two by N. A seed already used by an earlier
    case is redrawn from the same generator, keeping seeds unique.
    """
    for index, case in enumerate(cases):
        if case['name'] != f'sample2-{index:06d}':
            raise ValueError(f'case {index} ({case["name"]}) breaks the sample2 sequence')
    used = {case['seed'] for case in cases}
    result = []
    for index in range(len(cases), len(cases) + count):
        rng = random.Random((master_seed << 32) | index)
        seed = rng.getrandbits(32)
        while seed in used:
            seed = rng.getrandbits(32)
        used.add(seed)
        result.append(sample_case(rng, index, seed))
    return result


def database(root):
    db = sqlite3.connect(root / 'references.sqlite3', timeout=30)
    db.execute('PRAGMA journal_mode=WAL')
    db.executescript('''
        CREATE TABLE IF NOT EXISTS metadata (key TEXT PRIMARY KEY, value TEXT NOT NULL);
        CREATE TABLE IF NOT EXISTS requests (
            ordinal INTEGER PRIMARY KEY, name TEXT UNIQUE NOT NULL,
            input_sha256 TEXT NOT NULL, input_json TEXT NOT NULL);
        CREATE INDEX IF NOT EXISTS request_hash_lookup ON requests(input_sha256);
        CREATE TABLE IF NOT EXISTS results (
            mode TEXT NOT NULL, ordinal INTEGER NOT NULL REFERENCES requests(ordinal),
            outcome TEXT NOT NULL, reference_json TEXT NOT NULL,
            PRIMARY KEY(mode, ordinal));
    ''')
    return db


@contextmanager
def writer(root):
    root.mkdir(parents=True, exist_ok=True)
    with (root / 'writer.lock').open('a') as lock:
        fcntl.flock(lock, fcntl.LOCK_EX | fcntl.LOCK_NB)
        yield


def put(db, mode, ordinal, reference):
    """Never silently replace a different reference, including native faults."""
    payload = packed(reference)
    old = db.execute('SELECT reference_json FROM results WHERE mode=? AND ordinal=?',
                     (mode, ordinal)).fetchone()
    if old is not None and old[0] != payload:
        raise ValueError(f'conflicting {mode} reference at case {ordinal}')
    db.execute('INSERT OR IGNORE INTO results VALUES (?,?,?,?)',
               (mode, ordinal, reference['status'], payload))


def native_result(state, size, sha):
    return dict(status='generated' if state['returnCode'] == 0 else 'rejected',
                code=state['returnCode'], rng=state['rngState'],
                request=request_hex(state['request']), size=size, sha256=sha,
                x87=[state['x87ControlWord'], state['x87FinalControlWord']])


def retail_reference(report):
    run = report['runs']['retail']
    if run['status'] != 'ok':
        return {k: v for k, v in run.items() if k not in ('seconds', 'state')}
    state = run['state']
    for comparison in (report.get('retailRepeatability'), report.get('comparison')):
        if not comparison:
            continue
        for side in ('right', 'left'):
            if comparison.get(side + 'State') == state and comparison.get('map'):
                output = comparison['map']
                return native_result(state, output[side + 'Size'], output[side + 'Sha256'])
    raise ValueError('retail success lacks a map digest tied to its recorded state')


def verify_files(directory, expected):
    for name, sha in expected.items():
        if digest(directory / name) != sha:
            raise ValueError(f'provenance mismatch: {directory / name}')


def manifest(root):
    value = json.loads((root / 'manifest.json').read_text())
    if value['schema'] != SCHEMA:
        raise ValueError('unsupported corpus schema')
    return value


def write_atomic(path, text):
    temporary = path.with_name(path.name + '.tmp')
    temporary.write_text(text)
    temporary.replace(path)


def recorded_cases(root, info):
    path = root / 'cases.json'
    if digest(path) != info['cases_sha256']:
        raise ValueError('cases.json differs from manifest.json (interrupted extend?)')
    cases = load_cases(path)
    if len(cases) != info['cases']:
        raise ValueError('cases.json and manifest.json disagree on the request count')
    return cases


def sync_requests(db, root, info):
    """Store requests that cases.json records but the table lacks.

    ``extend`` writes cases.json and the manifest before the table, so this
    also completes an extension interrupted after its files were written.
    """
    count, top = db.execute('SELECT count(*), max(ordinal) FROM requests').fetchone()
    if count != (0 if top is None else top + 1) or count > info['cases']:
        raise ValueError('stored requests are not a prefix of manifest.json')
    if count == info['cases']:
        return 0
    cases = recorded_cases(root, info)
    if count:
        last = db.execute('SELECT name,input_json FROM requests WHERE ordinal=?', (count - 1,)).fetchone()
        if last != (cases[count - 1]['name'], packed(cases[count - 1])):
            raise ValueError('stored requests differ from cases.json')
    for i in range(count, len(cases)):
        db.execute('INSERT INTO requests VALUES (?,?,?,?)', (i, cases[i]['name'],
                   hashlib.sha256(job_bytes(cases[i])).hexdigest(), packed(cases[i])))
    db.commit()
    return len(cases) - count


def validate_environment(root, data):
    info = manifest(root)
    if assets(data) != info['assets']:
        raise ValueError('installation assets differ from this corpus')
    return info


def initialize(args):
    from . import __main__ as oracle
    with writer(args.out):
        if (args.out / 'manifest.json').exists():
            raise ValueError('corpus already initialized; use capture to resume')
        cases = load_cases(args.retail_corpus / 'cases.json')[:args.limit]
        if len(cases) != args.limit:
            raise ValueError('not enough requests in the source corpus')
        retail = json.loads((args.retail_corpus / 'provenance.json').read_text())
        hotfix = json.loads((args.hotfix_oracle / 'provenance.json').read_text())
        current_assets = assets(args.data)
        if current_assets != {k.lower(): v for k, v in retail['data'].items()
                              if Path(k).suffix.lower() not in ('.snd', '.vid')}:
            raise ValueError('assets do not match the imported retail captures')
        if hotfix['retailSha256'] != retail['retailSha256']:
            raise ValueError('oracles use different retail executables')
        verify_files(args.retail_corpus, {'rmg-host.exe': retail['patchedHostSha256'],
                                         'rmg-driver.dll': retail['driverSha256']})
        verify_files(args.hotfix_oracle, hotfix['objects'])
        sources = {}
        for mode, source, provenance in [('retail', args.retail_corpus, retail),
                                          ('hotfix', args.hotfix_oracle, hotfix)]:
            target = args.out / mode
            target.mkdir(exist_ok=True)
            names = ['rmg-host.exe', 'rmg-driver.dll']
            if mode == 'hotfix':
                names += list(hotfix['objects'])
            for name in names:
                shutil.copy2(source / name, target / name)
            for name in oracle.DLLS:
                library = oracle.find_ci(args.data.parent, name)
                if digest(library) != retail['libraries'][name]:
                    raise ValueError(f'library mismatch: {name}')
                shutil.copy2(library, target / name)
            sources[mode] = dict(provenance=provenance,
                                files={p.name: digest(p) for p in target.iterdir() if p.is_file()})
        info = dict(schema=SCHEMA, cases=len(cases), assets=current_assets, sources=sources,
                    request_source_sha256=digest(args.retail_corpus / 'cases.json'),
                    imports={'retail': str(args.retail_corpus.resolve())},
                    replay_policy='v2; water towns from heap; water alignment heap!=0; tent signed stack')
        (args.out / 'cases.json').write_text(packed(cases) + '\n')
        info['cases_sha256'] = digest(args.out / 'cases.json')
        with database(args.out) as db:
            db.execute('INSERT OR REPLACE INTO metadata VALUES (?,?)', ('schema', str(SCHEMA)))
            for i, case in enumerate(cases):
                db.execute('INSERT INTO requests VALUES (?,?,?,?)',
                           (i, case['name'], hashlib.sha256(job_bytes(case)).hexdigest(), packed(case)))
                report = json.loads((args.retail_corpus / (case['name'] + '.json')).read_text())
                if validate_case(report['case']) != case or report['index'] != i:
                    raise ValueError(f'imported request mismatch at {i}')
                put(db, 'retail', i, retail_reference(report))
                if i % 10000 == 0:
                    print(f'imported retail {i}/{len(cases)}', flush=True)
            if args.hotfix_captures:
                source = args.hotfix_captures
                verify_files(source, {name: sources['hotfix']['files'][name]
                                      for name in ('rmg-host.exe', 'rmg-driver.dll')})
                if json.loads((source / 'provenance.json').read_text()) != hotfix:
                    raise ValueError('hotfix capture provenance differs from the frozen oracle')
                for line in (source / 'cases.jsonl').read_text().splitlines():
                    row = json.loads(line)
                    i = row['index']
                    if i >= len(cases):
                        continue
                    if row['name'] != cases[i]['name']:
                        raise ValueError('hotfix request name mismatch')
                    n = row['native']
                    if n['status'] != 'ok':
                        continue  # Retry old infrastructure failures in the frozen harness.
                    # Check archived native inputs and state, independently of Rust verdicts.
                    slot = re.fullmatch(r'map-(\d+)\.raw', row['slot'])
                    ordinal = int(slot[1]) if slot else 0
                    batch = source / row['batch']
                    job = (batch / 'job.bin').read_bytes()[ordinal * 92:(ordinal + 1) * 92]
                    if job != job_bytes(cases[i]):
                        raise ValueError('archived hotfix job differs from requested input')
                    result_name = f'result-{ordinal}.bin' if slot else 'result.bin'
                    state = decode_result((batch / result_name).read_bytes())
                    reference = native_result(state, n['size'], n['sha256'])
                    if any(reference[k] != n[k] for k in ('code', 'rng', 'request', 'x87')):
                        raise ValueError('archived hotfix state differs from capture log')
                    put(db, 'hotfix', i, reference)
                info['imports']['hotfix'] = dict(path=str(source.resolve()),
                                                log_sha256=digest(source / 'cases.jsonl'))
        (args.out / 'manifest.json').write_text(json.dumps(info, indent=2) + '\n')
    status(args)


def extend(args):
    """Append deterministic sample2 requests; existing requests never change."""
    with writer(args.out):
        info = manifest(args.out)
        with database(args.out) as db:
            sync_requests(db, args.out, info)
        old = (args.out / 'cases.json').read_text()
        cases = recorded_cases(args.out, info)
        first = len(cases)
        cases += extension_cases(cases, args.count, args.seed)
        text = packed(cases) + '\n'
        if not (old.endswith(']\n') and text.startswith(old[:-2] + ',')):
            raise ValueError('re-serializing cases.json would change existing requests')
        write_atomic(args.out / 'cases.json', text)
        info['cases'] = len(cases)
        info['cases_sha256'] = digest(args.out / 'cases.json')
        info.setdefault('extensions', []).append(dict(
            sampler=SAMPLER, master_seed=args.seed, first=first, count=args.count,
            python=platform.python_version(), cases_sha256=info['cases_sha256']))
        write_atomic(args.out / 'manifest.json', json.dumps(info, indent=2) + '\n')
        with database(args.out) as db:
            sync_requests(db, args.out, info)
    status(args)


def status(args):
    info = manifest(args.out)
    extensions = info.get('extensions', [])
    segments = [dict(first=0, count=info['cases'] - sum(e['count'] for e in extensions),
                     source='import')]
    segments += [dict(first=e['first'], count=e['count'],
                      source=f"{e['sampler']} seed {e['master_seed']:#x}") for e in extensions]
    with database(args.out) as db:
        db.execute('BEGIN')  # one snapshot while a capture commits batches
        stored = db.execute('SELECT count(*) FROM requests').fetchone()[0]
        counts = {mode: dict(db.execute('SELECT outcome,count(*) FROM results WHERE mode=? '
                                       'GROUP BY outcome', (mode,))) for mode in MODES}
        for segment in segments:
            done = dict(db.execute('SELECT mode,count(*) FROM results WHERE ordinal>=? AND '
                                   'ordinal<? GROUP BY mode',
                                   (segment['first'], segment['first'] + segment['count'])))
            segment['missing'] = {mode: segment['count'] - done.get(mode, 0) for mode in MODES}
    value = dict(requests_per_mode=info['cases'], stored_requests=stored, modes=counts,
                 missing={mode: info['cases'] - sum(counts[mode].values()) for mode in MODES},
                 segments=segments)
    print(json.dumps(value, indent=2), flush=True)
    return value


def check_batch_protocol(path, entries):
    """Refuse harness failures that would otherwise become stored outcomes."""
    if len(entries) > 1 and (path / 'result.bin').exists():
        raise ValueError(f'{path}: the driver ignored the batch; use --batch-size 1')
    for entry in entries:
        if entry['status'] == 'process-error' and entry.get('exitCode') == 0:
            raise ValueError(f'{path}: clean exit without the job outputs')


def capture(args):
    from . import __main__ as oracle
    with writer(args.out):
        info = validate_environment(args.out, args.data)
        directory = args.out / args.mode
        verify_files(directory, info['sources'][args.mode]['files'])
        with database(args.out) as db:
            sync_requests(db, args.out, info)
        libraries = [directory / name for name in oracle.DLLS]
        run_info = dict(mode=args.mode, pid=os.getpid(), started=time.time(),
                        cpu_affinity=sorted(os.sched_getaffinity(0)),
                        wine=subprocess.check_output(['wine', '--version'], text=True).strip(),
                        wine_prefix=os.environ.get('WINEPREFIX'),
                        tool_sha256=digest(Path(__file__)),
                        harness_sha256=digest(Path(oracle.__file__)))
        (args.out / ('run-' + uuid.uuid4().hex + '.json')).write_text(
            json.dumps(run_info, indent=2) + '\n')
        size = args.batch_size or BATCH_SIZE[args.mode]
        with database(args.out) as db:
            pending = db.execute('SELECT ordinal,input_json FROM requests WHERE ordinal NOT IN '
                                 '(SELECT ordinal FROM results WHERE mode=?) ORDER BY ordinal',
                                 (args.mode,)).fetchall()
            began = time.monotonic()
            completed = 0
            while pending:
                batch, pending = pending[:size], pending[size:]
                name = 'capture-' + uuid.uuid4().hex
                entries = oracle.run_batch(directory, name,
                    'retail' if args.mode == 'retail' else 'candidate',
                    [job_bytes(json.loads(case)) for _, case in batch],
                    args.data, libraries, args.timeout)
                check_batch_protocol(directory / name, entries)
                retry = []
                keep = False
                for (i, case), entry in zip(batch, entries):
                    if entry['status'] == 'not-run':
                        retry.append((i, case))
                        continue
                    if entry['status'] == 'ok':
                        raw = entry['map'].read_bytes()
                        reference = native_result(entry['state'], len(raw), hashlib.sha256(raw).hexdigest())
                    else:
                        keep = True
                        reference = {k: v for k, v in entry.items() if k not in ('map', 'result', 'state')}
                        reference['artifacts'] = str((directory / name).relative_to(args.out))
                    put(db, args.mode, i, reference)
                    completed += 1
                db.commit()  # Atomic batch: interruption resumes only missing cases.
                if not keep:
                    shutil.rmtree(directory / name)
                pending = retry + pending
                progress = dict(mode=args.mode, pid=os.getpid(), added=completed,
                                remaining=len(pending), seconds=time.monotonic() - began)
                temporary = args.out / 'progress.tmp'
                temporary.write_text(json.dumps(progress, indent=2) + '\n')
                temporary.replace(args.out / 'progress.json')
                print(packed(progress), flush=True)
    status(args)


class Runner:
    def __init__(self, executable, data, log, timeout):
        self.process = subprocess.Popen([str(executable), str(data)], stdin=subprocess.PIPE,
            stdout=subprocess.PIPE, stderr=log, bufsize=0)
        self.pending = bytearray()
        self.timeout = timeout

    def read(self, count=None):
        deadline = time.monotonic() + self.timeout
        while (len(self.pending) < count if count is not None else b'\n' not in self.pending):
            wait = deadline - time.monotonic()
            if wait <= 0 or not select.select([self.process.stdout], [], [], wait)[0]:
                raise TimeoutError('Rust replay runner timed out')
            chunk = os.read(self.process.stdout.fileno(), 65536)
            if not chunk:
                raise ValueError('Rust replay runner exited unexpectedly')
            self.pending.extend(chunk)
        end = count if count is not None else self.pending.index(b'\n') + 1
        result = bytes(self.pending[:end])
        del self.pending[:end]
        return result

    def run(self, case, mode):
        self.process.stdin.write(replay(case, mode).encode())
        fields = self.read().decode().strip().split(' ', 3)
        if fields[0] == 'ok':
            count = int(fields[2])
            if not 0 <= count <= 64 * 1024 * 1024:
                raise ValueError('invalid runner output size')
            raw = self.read(count)
            return dict(status='ok', rng=int(fields[1]), size=len(raw), request=fields[3],
                        sha256=hashlib.sha256(raw).hexdigest())
        if fields[0] != 'error':
            raise ValueError('invalid runner response')
        request, message = fields[3].split(' ', 1)
        return dict(status='error', rng=None if fields[1] == 'none' else int(fields[1]),
                    stage=fields[2], request=request, message=message)

    def close(self):
        self.process.stdin.close()
        try:
            self.process.wait(timeout=3)
        except subprocess.TimeoutExpired:
            self.process.kill()
            self.process.wait()


def verdict(case, native, rust, mode):
    if native['status'] not in ('generated', 'rejected'):
        return 'native-fault'
    if rust['status'] == 'ok':
        return 'equal' if native['status'] == 'generated' and all(
            rust[k] == native[k] for k in ('rng', 'size', 'request', 'sha256')) else 'mismatch'
    if (mode == 'hotfix' and native['code'] == 3 and rust['stage'] == 'Towns'
        and rust['message'].endswith('hotfix requires owned starting towns for all requested players')
        and all(rust[k] == native[k] for k in ('rng', 'request'))):
        return 'matching-rejection'
    if mode == 'retail':
        if rust['stage'] == 'Selection' and re.search(
            r'rmg.txt row (323|329): unassigned player zone writes before retail player-slot arrays',
            rust['message']):
            return 'typed-unassigned-player-zone'
        match = re.search(r'x: (-?\d+), y: (-?\d+).*level: (Surface|Underground)', rust['message'])
        if (rust['stage'] == 'Rivers' and 'placement accesses missing cell' in rust['message'] and match
            and int(match[1]) == case['width'] and int(match[2]) == case['height'] - 1
            and match[3] == ('Surface' if case['levels'] == 1 else 'Underground')):
            return 'typed-coast-one-past'
    return 'mismatch'


def check(args):
    validate_environment(args.out, args.data)
    counts = Counter()
    args.report.parent.mkdir(parents=True, exist_ok=True)
    with database(args.out) as db, args.report.open('x') as report, \
         args.report.with_suffix('.log').open('x') as log:
        runner = Runner(args.runner.resolve(), args.data, log, args.timeout)
        try:
            rows = db.execute('SELECT name,input_json,reference_json FROM requests JOIN results '
                              'USING(ordinal) WHERE mode=? AND ordinal>=? ORDER BY ordinal LIMIT ?',
                              (args.mode, args.start, -1 if args.limit is None else args.limit))
            for name, case, native in rows:
                if args.name and name not in args.name:
                    continue
                case, native = json.loads(case), json.loads(native)
                rust = runner.run(case, args.mode) if native['status'] in ('generated', 'rejected') else None
                result = verdict(case, native, rust, args.mode)
                counts[result] += 1
                report.write(packed(dict(name=name, verdict=result, native=native, rust=rust)) + '\n')
                report.flush()
                if sum(counts.values()) % 100 == 0 or result == 'mismatch':
                    print(packed(counts), flush=True)
                if result == 'mismatch' and not args.keep_going:
                    break
        finally:
            runner.close()
    if not counts:
        raise ValueError('no captured references matched the selection')
    summary = dict(mode=args.mode, counts=counts, runner_sha256=digest(args.runner),
                   corpus_manifest_sha256=digest(args.out / 'manifest.json'))
    args.report.with_suffix('.summary.json').write_text(json.dumps(summary, indent=2) + '\n')
    print(json.dumps(summary, indent=2))
    return int(bool(counts['mismatch']))


def main(argv=None, prog=None):
    parser = argparse.ArgumentParser(prog=prog, description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('--out', type=Path, required=True, help='local rainbow-table directory')
    commands = parser.add_subparsers(dest='command', required=True)
    init = commands.add_parser('init', help='freeze oracles and import existing native references')
    init.add_argument('--retail-corpus', type=Path, required=True)
    init.add_argument('--hotfix-oracle', type=Path, required=True)
    init.add_argument('--hotfix-captures', type=Path)
    init.add_argument('--limit', type=int, default=100000)
    init.add_argument('--data', type=Path, required=True)
    cap = commands.add_parser('capture', help='fill missing native references with one worker')
    cap.add_argument('--mode', choices=MODES, required=True)
    cap.add_argument('--data', type=Path, required=True)
    cap.add_argument('--batch-size', type=int,
                     help='jobs per Wine process (default: retail 1, hotfix 32)')
    cap.add_argument('--timeout', type=float, default=45)
    grow = commands.add_parser('extend', help='append deterministic sample2 requests')
    grow.add_argument('--count', type=int, required=True)
    grow.add_argument('--seed', type=lambda value: int(value, 0), default=EXTEND_SEED,
                      help=f'extension master seed (default {EXTEND_SEED:#x})')
    commands.add_parser('status')
    run = commands.add_parser('check', help='compare Rust with stored references without Wine')
    run.add_argument('--mode', choices=MODES, required=True)
    run.add_argument('--data', type=Path, required=True)
    run.add_argument('--runner', type=Path, required=True)
    run.add_argument('--report', type=Path, required=True)
    run.add_argument('--start', type=int, default=0)
    run.add_argument('--limit', type=int, help='maximum stored references to check (default: all)')
    run.add_argument('--name', action='append')
    run.add_argument('--timeout', type=float, default=90)
    run.add_argument('--keep-going', action='store_true')
    args = parser.parse_args(argv)
    args.out = args.out.resolve()
    if hasattr(args, 'data'):
        args.data = args.data.resolve()
    for field in ('limit', 'batch_size', 'timeout', 'count'):
        if getattr(args, field, None) is not None and getattr(args, field) <= 0:
            parser.error(f'{field} must be positive')
    if getattr(args, 'start', 0) < 0:
        parser.error('start must be nonnegative')
    if not 0 <= getattr(args, 'seed', 0) <= 0xffffffff:
        parser.error('seed must be a 32-bit value')
    result = {'init': initialize, 'capture': capture, 'extend': extend, 'status': status,
              'check': check}[args.command](args)
    return result if type(result) is int else 0


if __name__ == '__main__':
    raise SystemExit(main())
