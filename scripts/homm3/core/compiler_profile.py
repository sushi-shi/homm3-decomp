"""Translate manifest MSVC profiles for Clang's name/type observations.

Code-generation switches are deliberately excluded: VC6 remains the byte oracle.
Every consumer adds its own action (IR, AST, editor), not its own ABI defaults.
"""
from __future__ import annotations

from homm3.core import clang


def arguments(flags, includes, *, root=None):
    args = ['--driver-mode=cl', f'--target={clang.TARGET}',
            '-fms-compatibility', '-fms-extensions',
            f'-fms-compatibility-version={clang.MSC_VER}', '-Wno-everything']
    for flag in flags:
        if root is not None and flag.startswith(('/I', '/FI')):
            prefix = '/FI' if flag.startswith('/FI') else '/I'
            args.append(prefix + str(root / flag[len(prefix):]))
        elif flag == '/GX':
            args.append('/EHsc')
        elif flag in ('/ML', '/MLd'):
            # Clang has no single-threaded CRT switch and defaults to _MT.
            args.extend(['/U_MT', '/U_DLL'])
            if flag == '/MLd':
                args.append('/D_DEBUG')
        elif flag in ('/Gr', '/Gd', '/Gz', '/TC', '/TP', '/MT', '/MD', '/MTd', '/MDd') \
                or flag.startswith(('/D', '/U', '/I', '/FI', '/Zp', '/EH')):
            args.append(flag)
    for include in includes:
        args.extend(['-imsvc', str(include)])
    return args


class Profiles:
    """One manifest/toolchain snapshot, safe to share with parsing workers."""
    def __init__(self, project):
        self.project = project
        self.data = project.manifest
        self.mirror = clang.mirror(project.root, project.toolchain)
        self.includes = ([self.mirror] if self.mirror else []) + project.includes
        self.by_source = {u['source']: u for u in self.data.get('unit', [])}

    def for_source(self, path):
        relative = path.resolve().relative_to(self.project.root).as_posix()
        unit = self.by_source.get(relative)
        # Unadmitted reference sources and standalone headers have no TU.
        # Their parsing profile is explicit project policy in the manifest.
        profile = unit['flags'] if unit else self.data['build']['analysis_profile']
        flags = self.data['flags'][profile]
        if not unit:
            flags = [*flags, '/TP']
        return arguments(flags, self.includes, root=self.project.root)
