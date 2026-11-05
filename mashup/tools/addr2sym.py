#!/usr/bin/env python3
"""Map Heroes3.exe addresses to functions using the VC6 link map.

    addr2sym.py ADDRESS... [--map build/exe/HEROES3.candidate.map]
"""
import argparse
import bisect
import re

ap = argparse.ArgumentParser()
ap.add_argument('addresses', nargs='+')
ap.add_argument('--map', default='build/exe/HEROES3.candidate.map')
args = ap.parse_args()
rows = []
for line in open(args.map, errors='replace'):
    if not line.startswith(' 0001:'):
        continue
    parts = line.split()
    va = next((p for p in parts[2:] if re.fullmatch(r'00[0-9a-f]{6}', p)), None)
    if va:
        rows.append((int(va, 16), parts[1], parts[-1]))
rows.sort()
keys = [r[0] for r in rows]
for text in args.addresses:
    address = int(text, 16)
    i = bisect.bisect_right(keys, address) - 1
    va, name, obj = rows[i]
    print(f'{address:08x} = {name}+0x{address - va:x} ({obj})')
