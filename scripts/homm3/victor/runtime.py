"""Link the recovered Victor objects without retail Victor fallbacks."""
from pathlib import Path
import shutil
import sys

from homm3.core.cc_wrap import find_ci, msvc_dir, winepath_w
from homm3.core.common import HOMM3_DIR as ROOT
from homm3.rmg.__main__ import run_command, write_json, file_digest
from homm3.rmg.bindings import absolute_object, prepare_calls, undefined_symbols, DRIVER_BASE

UNITS = ('victor', 'victor_flip', 'victor_loadpcx', 'victor_pcx_kernels')


def resolve(paths):
    # Only CRT memory services and SEH are external. No Victor code or data
    # is admitted. Memory addresses come from config/retail-runtime-map.tsv;
    # SEH handler is the same CRT entry documented in the RMG oracle.
    runtime = {'_malloc': 0x61a405, '_calloc': 0x61a491, '_free': 0x6195e0,
               '__except_handler3': 0x61a528, '__except_list': 0}
    result = {}
    for name in sorted(undefined_symbols(paths)):
        if name.startswith('__imp__'):
            continue
        if name not in runtime:
            raise ValueError(f'unresolved dependency (no retail Victor fallback): {name}')
        result[name] = runtime[name]
    return result


def build_driver(out: Path):
    log = out / 'build.log'
    run_command([sys.executable, '-m', 'homm3.build.configure'], log)
    objects = [ROOT / 'build/objdiff/base' / (unit + '.obj') for unit in UNITS]
    run_command(['ninja', *(str(p.relative_to(ROOT)) for p in objects)], log)
    driver = out / 'driver.obj'
    run_command([sys.executable, '-m', 'homm3.core.cc_wrap', '--src',
                 str(Path(__file__).with_name('driver.cpp')), '--out', str(driver),
                 '--', '/nologo', '/c', '/O2', '/MT'], log)
    archived = [driver]
    for path in objects:
        copy = out / path.name
        shutil.copyfile(path, copy)
        archived.append(copy)
    bindings = resolve(archived)
    write_json(out / 'bindings.json', {name: hex(address) for name, address in bindings.items()})
    bridge = out / 'bridge.obj'
    bridge.write_bytes(absolute_object(bindings))
    copies = []
    for path in archived:
        copy = out / ('link-' + path.name)
        copy.write_bytes(prepare_calls(path.read_bytes(), bindings))
        copies.append(copy)
    toolchain = msvc_dir()
    run_command(['wine', str(find_ci(toolchain / 'bin', 'link.exe')), '/NOLOGO',
                 '/DLL', '/NOENTRY', '/NODEFAULTLIB', '/INCREMENTAL:NO', '/FIXED',
                 f'/BASE:0x{DRIVER_BASE:x}', '/EXPORT:run=_run@0',
                 '/MAP:' + winepath_w(out / 'driver.map'),
                 '/OUT:' + winepath_w(out / 'rmg-driver.dll'),
                 *(winepath_w(p) for p in copies), winepath_w(bridge),
                 *(winepath_w(find_ci(toolchain / 'lib', name + '.lib'))
                   for name in ('kernel32', 'user32', 'gdi32'))], log)
    return {p.name: file_digest(p) for p in archived}
