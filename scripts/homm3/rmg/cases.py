"""Validated whole-map requests and byte-exact output comparisons."""
from __future__ import annotations

import hashlib
import json
import re
import struct
from pathlib import Path

FIELDS = ('width', 'height', 'levels', 'humanPlayerCount', 'humanTeamCount',
          'computerPlayerCount', 'computerTeamCount', 'waterContent',
          'monsterStrength', 'mapVersion')
DEFAULTS = dict(zip(FIELDS, (36, 36, 1, 2, 2, 0, 8, 3, 0, 2)))


def integer(value, lo: int, hi: int, name: str) -> int:
    if type(value) is not int or not lo <= value <= hi:
        raise ValueError(f'{name} must be an integer in [{lo}, {hi}]')
    return value


def validate_case(case: dict) -> dict:
    if not isinstance(case, dict):
        raise ValueError('each case must be an object')
    unknown = set(case) - set(FIELDS) - {'name', 'seed', 'stackWord', 'heapByte', 'isHumanSeat', 'townType'}
    if unknown:
        raise ValueError(f'unknown case fields: {sorted(unknown)}')
    name = case.get('name')
    if not isinstance(name, str) or not re.fullmatch(r'[A-Za-z0-9][A-Za-z0-9_-]{0,63}', name):
        raise ValueError('case name must be a safe, unique filename (1..64 characters)')
    result = dict(DEFAULTS, **case)
    integer(result.get('seed'), 0, 0xffffffff, 'seed')
    integer(result.setdefault('stackWord', 0), 0, 0xffffffff, 'stackWord')
    if result.setdefault('heapByte', 0) is not None:
        integer(result['heapByte'], 0, 255, 'heapByte')
    for field in ('width', 'height'):
        if integer(result[field], 36, 144, field) not in (36, 72, 108, 144):
            raise ValueError(f'{field} must be 36, 72, 108 or 144')
    if result['width'] != result['height']:
        raise ValueError('the whole-map harness currently accepts square maps only')
    for field, lo, hi in (('levels', 1, 2), ('humanPlayerCount', 1, 8),
                          ('humanTeamCount', 0, 8), ('computerPlayerCount', 0, 7),
                          ('computerTeamCount', 0, 8), ('waterContent', 0, 3),
                          ('monsterStrength', -2, 2), ('mapVersion', 0, 2)):
        integer(result[field], lo, hi, field)
    if not 2 <= result['humanPlayerCount'] + result['computerPlayerCount'] <= 8:
        raise ValueError('total player count must be 2..8')
    for field, default, lo, hi in (('isHumanSeat', [0] * 8, 0, 1),
                                  ('townType', [-1] * 8, -1, 8)):
        values = result.setdefault(field, default)
        if not isinstance(values, list) or len(values) != 8:
            raise ValueError(f'{field} must contain exactly eight values')
        for value in values:
            integer(value, lo, hi, field)
    return result


def load_cases(path: Path) -> list[dict]:
    cases = json.loads(path.read_text())
    if not isinstance(cases, list) or not cases:
        raise ValueError('cases file must be a nonempty JSON array')
    result = [validate_case(case) for case in cases]
    if len({case['name'] for case in result}) != len(result):
        raise ValueError('duplicate case names')
    return result


def job_bytes(case: dict) -> bytes:
    heap_byte = 0xffffffff if case['heapByte'] is None else case['heapByte']
    return struct.pack('<III8B18i', case['seed'], case['stackWord'], heap_byte, *case['isHumanSeat'],
                       *case['townType'], *(case[field] for field in FIELDS))


def digest(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def byte_difference(left: bytes, right: bytes) -> dict:
    first = next((i for i, pair in enumerate(zip(left, right)) if pair[0] != pair[1]), None)
    if first is None and len(left) != len(right):
        first = min(len(left), len(right))
    result = {'equal': first is None, 'leftSize': len(left), 'rightSize': len(right),
              'leftSha256': digest(left), 'rightSha256': digest(right), 'firstDifference': first}
    if first is not None:
        start = max(0, first - 16)
        result.update(contextOffset=start, leftContext=left[start:first + 17].hex(),
                      rightContext=right[start:first + 17].hex())
    return result


def decode_result(data: bytes) -> dict:
    if len(data) != 96 or data[:4] != b'RMG1':
        raise ValueError('missing or malformed driver result')
    code, rng, control = struct.unpack_from('<iII', data, 4)
    values = struct.unpack_from('<8B18i', data, 16)
    return {'returnCode': code, 'rngState': rng, 'x87ControlWord': control & 0xffff,
            'x87FinalControlWord': control >> 16,
            'request': dict(zip(FIELDS, values[16:]),
                            isHumanSeat=list(values[:8]), townType=list(values[8:16]))}


def compare_runs(left: Path, right: Path) -> dict:
    """Compare every observable, preserving raw differences without masking."""
    first = decode_result((left / 'result.bin').read_bytes())
    second = decode_result((right / 'result.bin').read_bytes())
    output = byte_difference((left / 'map.raw').read_bytes(), (right / 'map.raw').read_bytes())
    return {'equal': first == second and output['equal'], 'map': output,
            'stateEqual': first == second, 'leftState': first, 'rightState': second}
