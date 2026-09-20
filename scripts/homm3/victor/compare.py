"""Batch retail/candidate execution with fresh-process repeat controls."""
from pathlib import Path
import json
import os
import shutil
import signal
import subprocess
import time

from homm3.core import inputs
from homm3.core.common import HOMM3_DIR as ROOT
from homm3.core.cc_wrap import find_ci, winepath_w
from homm3.rmg.bootstrap import patch_winmain
from homm3.rmg.__main__ import DLLS, file_digest, write_json
from .runtime import build_driver
from .protocol import decode, verify_pixels


def run_batch(out, inputs, label, libraries, timeout, allocation):
    directory = out / label
    directory.mkdir()
    for path in (out / 'victor-host.exe', out / 'rmg-driver.dll', *libraries):
        (directory / path.name).symlink_to(path)
    # Legacy OpenFile rejects long paths. Use short input aliases in the run
    # directory and translate the output root once.
    output_root = winepath_w(directory)
    job = bytearray()
    for index, path in enumerate(inputs):
        if path.parent != inputs[0].parent:
            raise ValueError('batch inputs must share a directory')
        local_input = f'i{index}.pcx'
        (directory / local_input).symlink_to(path)
        for name in (local_input,
                     output_root + '\\' + str(index) + '.bin'):
            job.extend(name.encode('ascii') + b'\0')
    (directory / 'job.bin').write_bytes(job)
    env = dict(os.environ, VICTOR_JOB=output_root + '\\job.bin',
               VICTOR_ALLOCATION=allocation,
               VICTOR_MODE='candidate' if label.startswith('candidate') else 'retail',
               WINEDLLOVERRIDES='winedbg.exe=d')
    started = time.monotonic()
    with (directory / 'wine.log').open('wb') as log:
        process = subprocess.Popen(['wine', str(directory / 'victor-host.exe')],
                                   cwd=directory, env=env, stdout=log,
                                   stderr=subprocess.STDOUT, start_new_session=True)
        try:
            code = process.wait(timeout=timeout)
            result = dict(status='ok' if code == 0 else 'process-error', exitCode=code)
        except subprocess.TimeoutExpired:
            try:
                os.killpg(process.pid, signal.SIGKILL)
            except ProcessLookupError:
                pass
            process.wait()
            result = dict(status='timeout')
    result['seconds'] = round(time.monotonic() - started, 3)
    if (directory / 'failure.bin').exists():
        result.update(status='crash', failure=(directory / 'failure.bin').read_bytes().hex())
    write_json(directory / 'run.json', result)
    print(f'{label}: {result}', flush=True)
    return result


def compare(corpus: Path, out: Path, limit: int | None, timeout: float, allocation: str = "global"):
    os.environ.setdefault('WINEPREFIX', str(ROOT / 'build/wineprefix'))
    os.environ.setdefault('WINEDEBUG', '-all')
    corpus = corpus.resolve(strict=True)
    manifest = json.loads((corpus / 'inventory.json').read_text())
    if manifest['counts'].get('unknown'):
        raise ValueError('corpus contains unclassified image resources')
    records = manifest['images']
    unique = {row['pcx']: row for row in records}
    selected = list(unique.values())
    if limit is not None:
        selected = selected[:limit]
    if not selected:
        raise ValueError('empty Victor corpus')
    paths = [corpus / row['pcx'] for row in selected]
    for path, row in zip(paths, selected):
        if file_digest(path) != row['pcxSha256']:
            raise ValueError(f'input changed after corpus generation: {path}')
        if file_digest(corpus / row['asset']) != row['sha256']:
            raise ValueError(f'original artwork changed after corpus generation: {row["asset"]}')
    out = out.resolve()
    out.mkdir(parents=True, exist_ok=False)
    retail = inputs.read_verified(inputs.RETAIL, inputs.RETAIL.destination)
    (out / 'victor-host.exe').write_bytes(patch_winmain(retail))
    game = Path(manifest['gameDirectory'])
    libraries = []
    for name in DLLS:
        source = find_ci(game, name)
        if source is None:
            raise ValueError(f'missing game library {name}')
        target = out / source.name
        shutil.copyfile(source, target)
        libraries.append(target)
    provenance = dict(allocation=allocation, timeoutSeconds=timeout,
                      winePrefix=os.environ['WINEPREFIX'],
                      wineVersion=subprocess.check_output(['wine', '--version'], text=True).strip(),
                      unitsManifestSha256=file_digest(ROOT / 'config/units.toml'),
                      patchedHostSha256=file_digest(out / 'victor-host.exe'), corpusSha256=file_digest(corpus / 'inventory.json'),
                      retailSha256=file_digest(inputs.RETAIL.destination),
                      libraries={p.name: file_digest(p) for p in libraries},
                      objects=build_driver(out),
                      driverSha256=file_digest(out / 'rmg-driver.dll'),
                      selected=selected,
                      driverSources={p.name: file_digest(p) for p in Path(__file__).parent.iterdir()
                                     if p.suffix in ('.py', '.cpp')})
    write_json(out / 'provenance.json', provenance)
    labels = ('retail', 'retail-repeat', 'candidate', 'candidate-repeat')
    runs = {label: run_batch(out, paths, label, libraries, timeout, allocation) for label in labels}
    report = dict(runs=runs, cases=[], equal=True, executionErrors=False,
                  resourceOccurrences=len(records), distinctCorpusInputs=len(unique),
                  selectedInputs=len(selected))
    for index, row in enumerate(selected):
        case = dict(input=row['pcx'], equal=False)
        try:
            raw = [(out / label / f'{index}.bin').read_bytes() for label in labels]
            decoded = [decode(data) for data in raw]
            case['retailRepeatability'] = raw[0] == raw[1]
            case['candidateRepeatability'] = raw[2] == raw[3]
            case['comparison'] = raw[0] == raw[2]
            case['successfulImport'] = all(all(data.get(key) == 0 for key in
                  ('infoStatus', 'allocateStatus', 'loadStatus', 'flipStatus')) for data in decoded)
            original = (corpus / row['asset']).read_bytes()
            case['artworkPreserved'] = all(verify_pixels(data, original, row['format']) for data in decoded)
            case['equal'] = all(case[k] for k in ('retailRepeatability', 'candidateRepeatability',
                                                 'comparison', 'successfulImport', 'artworkPreserved'))
            if not case['comparison']:
                case['firstDifference'] = next((i for i, pair in enumerate(zip(raw[0], raw[2]))
                                                if pair[0] != pair[1]), min(len(raw[0]), len(raw[2])))
        except (ValueError, OSError) as error:
            case['error'] = str(error)
            report['executionErrors'] = True
        report['equal'] &= case['equal']
        report['cases'].append(case)
    if any(run['status'] != 'ok' for run in runs.values()):
        report.update(equal=False, executionErrors=True)
    write_json(out / 'report.json', report)
    print(f"{sum(case['equal'] for case in report['cases'])}/{len(selected)} inputs equal; {out / 'report.json'}")
    return 2 if report['executionErrors'] else 0 if report['equal'] else 1
