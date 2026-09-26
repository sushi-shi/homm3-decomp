"""One immutable-input Mac build session; no cache survives a command."""
from __future__ import annotations

import hashlib
from pathlib import Path


def snapshot(paths):
    files = set()
    for path in paths:
        if path.is_dir():
            files.update(p for p in path.rglob('*') if p.is_file() and '__pycache__' not in p.parts)
        elif path.is_file():
            files.add(path)
    return {str(p): hashlib.sha256(p.read_bytes()).hexdigest() for p in sorted(files)}


class BuildSession:
    def __init__(self, root: Path, pef, tools_dir: Path):
        from homm3.mac import build, call_report
        self.root, self.tools_dir = root, tools_dir
        self.input_paths = [root / name for name in
                            ('src', 'include', 'vendor', 'config', 'scripts/homm3',
                             'build/mac/sdk', 'build/mac/toolchain')]
        self.inputs = snapshot(self.input_paths)
        self.pairs = build.load_pairs(root)
        self.context = call_report.inspection_context(root, pef, pairs=self.pairs)
        self.wine_version = build._wine_version()
        self.units, self.failures, self.compiled, self.artifacts = {}, {}, {}, {}
        self.prepared = {}
        self.header_inputs = {}

    def headers(self, profile):
        from homm3.mac import profiles
        if profile is None:
            return {}
        key = profile.include_dirs
        if key not in self.header_inputs:
            self.header_inputs[key] = profiles.headers(self.root, profile)
        return self.header_inputs[key]

    def compile(self, pair):
        from homm3.mac import build
        if pair not in self.compiled:
            self.compiled[pair] = build.compile_pair(pair, self.tools_dir,
                                                      sdk_staged=True, session=self)
        return self.compiled[pair]

    def provenance(self, pair):
        from homm3.mac import build
        if pair in self.compiled:
            row = self.compiled[pair]
            return row.source_hash, row.build_hash
        source, headers, fingerprint = self.prepared[build.object_directory(self.root, pair)]
        return build._digest(build.source_identity(pair, source, header_inputs=headers)), fingerprint

    def remember_artifacts(self, work):
        paths = [work / name for name in
                 ('candidate.cpp', 'candidate.o', 'candidate.dis.txt', 'build-stamp.json', '_inputs')]
        self.artifacts[work] = (paths, snapshot(paths))

    def verify(self):
        from homm3.mac.build import MacBuildError
        if snapshot(self.input_paths) != self.inputs:
            raise MacBuildError('Mac build inputs changed during comparison; reports/checkpoint withheld')
        for work, (paths, expected) in self.artifacts.items():
            if snapshot(paths) != expected:
                raise MacBuildError(f'Mac compilation artifacts changed in {work}; reports/checkpoint withheld')
