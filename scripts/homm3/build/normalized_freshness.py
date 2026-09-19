"""Provenance stamps for disposable comparison objects.

Each normalization stage writes ``<output>.stamp.json`` recording the exact
content identity of every input it consumed. A consumer of a normalized
comparison object verifies the whole recorded chain before trusting it, so a
focused raw rebuild can never be silently compared through a stale normalized
copy. A missing stamp is reported as unverifiable rather than silently
accepted; regenerating through ``homm3 build`` writes current stamps.

The stamp is diagnostic provenance for disposable comparison copies only. Raw
compiler and delinker objects remain authoritative for linking and hard gates.
"""

from __future__ import annotations

import hashlib
import json
import os
import tempfile
from pathlib import Path

STAMP_SUFFIX = ".stamp.json"
# 2: text-pad trim; 3: non-code sections dropped (functions-only scope);
# 4: paired base/target padding normalization; 5: source-owned compiler-
# function manifest participates in semantic `$E<n>` canonicalization;
# 6: paired EH handler-owner relocation canonicalization; 7: semantic retail
# unwind-owner names join the equivalent candidate cleanup funclets; 8 was
# the initial equivalent-relocation schema; 9 skips the generated name map's
# provenance preamble; 10 removes proved false-literal rows rather than using
# an objdiff-unsupported ABSOLUTE placeholder; 11 refuses any candidate-side
# relocation at a literal site and requires one unambiguous aggregate anchor;
# 12 removes paired candidate ``__except_list`` relocations that retail proves
# are fixed-base literal-zero operands.
# 13 admits an OWNER-FREE static destructor into the semantic `$E<n>`
# canonicalization - an empty holder whose teardown names only other
# compilands' globals and so relocates no datum of its own object.
# 14 refuses ownerless fallback when an initializer supplies contradictory
# callback-registration evidence.
# 15 canonicalizes reviewed anonymous-namespace paths and tracks their table.
# 16 verifies output bytes and transform implementations; no stat-based memo.
STAMP_SCHEMA = 16


class ValidationContext:
    """One operation's reads; writers forget changed outputs.

    Never retain this across commands, delinking or external input changes.
    """
    def __init__(self):
        self.hashes = {}
        self.paths = {}

    def resolve(self, path: Path) -> Path:
        if path not in self.paths:
            resolved = path.resolve()
            self.paths[path] = resolved
            self.paths[resolved] = resolved
        return self.paths[path]

    def digest(self, path: Path) -> str:
        path = self.resolve(path)
        if path not in self.hashes:
            self.hashes[path] = _sha256(path)
        return self.hashes[path]

    def forget(self, path: Path):
        self.hashes.pop(self.resolve(path), None)


def _sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with open(path, "rb") as stream:
        for block in iter(lambda: stream.read(1 << 20), b""):
            digest.update(block)
    return digest.hexdigest()


def implementation_inputs() -> dict[str, Path]:
    """Explicit transform dependency list, not automatic import discovery.

    Add any new behavior-affecting helper or dependency here. Content changes
    to listed files invalidate existing stamps automatically; bump STAMP_SCHEMA
    when the stamp format or validation contract changes.
    """
    directory = Path(__file__).parent
    paths = {"tool:" + name: directory / name for name in (
        "normalized_freshness.py", "normalize_objs.py", "canonicalize_data_symbols.py")}
    for name in ("project.py", "image.py", "inputs.py"):
        paths["tool:core/" + name] = directory.parent / "core" / name
    return paths


def stamp_path(output: Path) -> Path:
    return output.with_name(output.name + STAMP_SUFFIX)


def _portable_reference(input_path: Path, stamp_directory: Path) -> str:
    """Prefer a stamp-relative spelling so copied build trees stay coherent."""
    resolved = Path(input_path).resolve()
    try:
        return os.path.relpath(resolved, stamp_directory.resolve())
    except ValueError:
        return str(resolved)


def write_stamp(output: Path, inputs: dict[str, Path], *, context: ValidationContext | None = None) -> Path:
    """Record the content identity of every input consumed for ``output``."""
    path = stamp_path(output)
    context = context or ValidationContext()
    context.forget(output)
    inputs = {**inputs, **implementation_inputs()}
    payload = {
        "schema": STAMP_SCHEMA,
        "output_sha256": context.digest(output),
        "inputs": {
            role: {
                "path": _portable_reference(Path(input_path), path.parent),
                "sha256": context.digest(Path(input_path)),
            }
            for role, input_path in sorted(inputs.items())
        },
    }
    sidecar = output.with_suffix('.symbols.tsv')
    if sidecar.is_file():
        context.forget(sidecar)
        payload['sidecar_sha256'] = context.digest(sidecar)
    # A killed writer must leave an old (invalid) stamp or a complete new one.
    temporary = None
    try:
        with tempfile.NamedTemporaryFile(mode='w', dir=path.parent,
                                         prefix='.stamp-', suffix='.tmp', delete=False) as stream:
            temporary = Path(stream.name)
            stream.write(json.dumps(payload, indent=2, sort_keys=True) + "\n")
        os.replace(temporary, path)
    finally:
        if temporary is not None:
            temporary.unlink(missing_ok=True)
    return path


def freshness_problems(output: Path, _seen: set | None = None, *,
                       required_inputs: dict[str, Path] | None = None,
                       context: ValidationContext | None = None) -> list[str]:
    """Return every provenance problem for one normalized object.

    Each recorded input must exist and hash to its recorded identity. When a
    recorded input has its own stamp, its chain is verified recursively, so a
    rebuilt raw candidate invalidates both the paired and normalized copies
    that were derived from the old bytes.

    A normalization stage can require its complete input roles and paths;
    an earlier raw-only stamp must never stand in for a finished paired pass.
    """
    output = Path(output)
    context = context or ValidationContext()
    seen = _seen if _seen is not None else set()
    key = str(context.resolve(output))
    if key in seen:
        return []
    seen.add(key)
    problems = []
    stamp = stamp_path(output)
    if not stamp.is_file():
        problems.append("%s has no provenance stamp; run `homm3 build`" % output)
        return problems
    try:
        payload = json.loads(stamp.read_text())
    except (json.JSONDecodeError, OSError) as exc:
        problems.append("%s stamp is unreadable (%s); run `homm3 build`" % (output, exc))
        return problems
    if not isinstance(payload, dict) or payload.get("schema") != STAMP_SCHEMA:
        problems.append("%s stamp has unknown schema; run `homm3 build`" % output)
        return problems
    records = payload.get("inputs", {})
    if not isinstance(records, dict) or any(not isinstance(r, dict)
            or not isinstance(r.get('path'), str) for r in records.values()):
        return [f'{output} stamp has invalid input records; run `homm3 build`']
    if not output.is_file() or context.digest(output) != payload.get('output_sha256'):
        problems.append(f'{output} is stale: normalized output changed; run `homm3 build`')
    sidecar = output.with_suffix('.symbols.tsv')
    if 'sidecar_sha256' in payload and (not sidecar.is_file()
            or context.digest(sidecar) != payload['sidecar_sha256']):
        problems.append(f'{output} is stale: symbol sidecar changed; run `homm3 build`')
    for role, required in {**(required_inputs or {}), **implementation_inputs()}.items():
        record = records.get(role)
        if record is None:
            problems.append("%s stamp lacks required %s input" % (output, role))
        elif context.resolve(stamp.parent / record.get("path", "")) != context.resolve(required):
            problems.append("%s stamp has a different %s input path" % (output, role))
    for role, record in sorted(records.items()):
        input_path = Path(record.get("path", ""))
        if not input_path.is_absolute():
            input_path = context.resolve(stamp.parent / input_path)
        if not input_path.is_file():
            problems.append("%s input %s is missing: %s" % (output, role, input_path))
            continue
        if context.digest(input_path) != record.get("sha256"):
            problems.append(
                "%s is stale: %s input changed (%s); run `homm3 build`" %
                (output, role, input_path))
            continue
        if stamp_path(input_path).is_file():
            problems.extend(freshness_problems(input_path, seen, context=context))
    return problems
