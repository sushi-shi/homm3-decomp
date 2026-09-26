"""Reviewed binary operations for targets without a source identity.

These are investigation evidence, never compiler symbols, source mappings or
caller-closure dispositions. Source declarations remain the only source owners.
"""
from __future__ import annotations

import hashlib
import re
from pathlib import Path

from homm3.core import tsv
from homm3.mac import tables

PATH = 'config/mac/target-observations.tsv'
FIELDS = ['offset', 'size', 'sha256', 'category', 'operation', 'evidence']
CATEGORIES = frozenset({'platform', 'library', 'game_variant', 'shared_candidate'})


def read(root: Path, pef, spans: dict[int, int]) -> dict[int, dict]:
    path = root / PATH
    if not path.exists():
        return {}
    _, fields, rows = tsv.read(path)
    if fields != FIELDS:
        raise ValueError(f'{PATH}: expected columns {FIELDS}')
    result = {}
    for row in rows:
        offset = tables._hex(row['offset'], PATH)
        size = tables._hex(row['size'], PATH)
        if offset in result:
            raise ValueError(f'{PATH}: duplicate target {offset:#x}')
        if size <= 0 or offset % 4 or size % 4 or spans.get(offset) != size:
            raise ValueError(f'{PATH}: unverified span {offset:#x}+{size:#x}')
        if (row['category'] not in CATEGORIES or not row['operation'].strip()
                or not row['evidence'].strip()
                or not re.fullmatch(r'[0-9a-f]{64}', row['sha256'])):
            raise ValueError(f'{PATH}: invalid observation at {offset:#x}')
        if hashlib.sha256(pef.read(0, offset, size)).hexdigest() != row['sha256']:
            raise ValueError(f'{PATH}: byte hash mismatch at {offset:#x}')
        result[offset] = {k: row[k] for k in ('category', 'operation', 'evidence')}
    return result
