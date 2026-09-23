"""Content provenance for raw VC6 objects consumed by data binding.

Ninja's timestamps schedule compilation; they do not prove that an object
belongs to today's source. The data binder requires this content stamp.
"""
from __future__ import annotations

import hashlib
import json
import os
from pathlib import Path
import tempfile

SCHEMA = 1


def digest(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def tree_digest(root):
    """Include names and contents, including additions/deletions and .inc files."""
    value = hashlib.sha256()
    if not root.is_dir():
        return None
    for path in sorted(p for p in root.rglob('*') if p.is_file()):
        value.update(str(path.relative_to(root)).encode())
        value.update(b'\0')
        value.update(bytes.fromhex(digest(path)))
    return value.hexdigest()


def snapshot(root, source, flags, includes, toolchain):
    roots = {source.parent.resolve(), *(Path(p).resolve() for p in includes),
             (toolchain / 'include').resolve(), (toolchain / 'bin').resolve()}
    # Remove nested roots without weakening the fingerprint.
    roots = {p for p in roots if not any(p != q and p.is_relative_to(q) for q in roots)}
    files = [source.resolve(), root/'config/units.toml', root/'config/project.toml',
             root/'scripts/homm3/core/cc_wrap.py', root/'scripts/homm3/core/project.py',
             root/'scripts/homm3/build/compiled_freshness.py']
    return dict(schema=SCHEMA, source=str(source.resolve()), flags=list(flags),
                includes=[str(Path(p).resolve()) for p in includes], toolchain=str(toolchain.resolve()),
                files={str(p): digest(p) for p in files},
                trees={str(p): tree_digest(p) for p in sorted(roots)})


def stamp_path(output):
    return output.with_name(output.name + '.compile.json')


def write(output, before, after):
    if before != after:
        raise ValueError('compiler inputs changed during compilation; raw object has no valid provenance')
    record = dict(before, object_sha256=digest(output))
    output.parent.mkdir(parents=True, exist_ok=True)
    fd, temporary = tempfile.mkstemp(dir=output.parent, prefix='.compile-')
    try:
        with os.fdopen(fd, 'w') as stream:
            json.dump(record, stream, indent=2)
            stream.write('\n')
        os.replace(temporary, stamp_path(output))
    finally:
        Path(temporary).unlink(missing_ok=True)


def validate(output, expected):
    try:
        record = json.loads(stamp_path(output).read_text())
    except (OSError, ValueError) as exc:
        raise ValueError(f'{output.name}: missing/invalid raw compiler provenance; rebuild this TU') from exc
    object_digest = record.pop('object_sha256', None)
    if record != expected:
        raise ValueError(f'{output.name}: stale raw compiler inputs; rebuild this TU')
    if not output.is_file() or object_digest != digest(output):
        raise ValueError(f'{output.name}: raw object bytes differ from compiler provenance; rebuild this TU')
    return object_digest
