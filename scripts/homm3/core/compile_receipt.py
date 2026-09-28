"""Content receipts for comparisons that infer owners from compiled source.

Ninja decides when to compile. A receipt independently verifies that a raw
object still belongs to the current inputs, even when timestamps were restored.
"""
from hashlib import sha256
import json
from pathlib import Path


def digest(path):
    return sha256(Path(path).read_bytes()).hexdigest()


def snapshot(paths):
    return {str(Path(p).resolve()): digest(p) for p in sorted(set(map(str, paths)))}


def publish(out, inputs, flags):
    if snapshot(inputs) != inputs:
        raise ValueError('compiler inputs changed during compilation')
    payload = dict(schema=1, object_sha256=digest(out), inputs=inputs, flags=flags)
    path = Path(str(out) + '.inputs.json')
    tmp = path.with_suffix('.tmp')
    tmp.write_text(json.dumps(payload, sort_keys=True) + '\n')
    tmp.replace(path)


def current(out, *, flags, required, hashes=None):
    """Return the checked input hashes, or None for absent/stale evidence."""
    hashes = {} if hashes is None else hashes
    try:
        record = json.loads(Path(str(out) + '.inputs.json').read_text())
        inputs = record['inputs']
        if (record['schema'] != 1 or record['flags'] != flags or not inputs
                or record['object_sha256'] != digest(out)
                or not {str(Path(p).resolve()) for p in required} <= inputs.keys()):
            return None
        for path, expected in inputs.items():
            if path not in hashes:
                hashes[path] = digest(path)
            if hashes[path] != expected:
                return None
        return inputs
    except (OSError, ValueError, KeyError, TypeError):
        return None
