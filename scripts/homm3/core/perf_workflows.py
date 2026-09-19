"""Disposable merge and cold-worktree preparations for core.perf.

Only detached benchmark HEADs are merged. Source branches are never advanced.
Cold worktrees retain their logs/build outputs for review and explicit cleanup.
"""
from contextlib import contextmanager
from pathlib import Path
import os
import shutil
import subprocess
import tempfile


def git(root, *args, env=None, text=None):
    return subprocess.run(['git', *args], cwd=root, env=env, input=text,
                          text=True, capture_output=True, check=True).stdout.strip()


@contextmanager
def merged(root: Path, header: bool, label: str):
    if git(root, 'status', '--porcelain', '--untracked-files=no'):
        raise ValueError(f'merge benchmark requires a clean disposable worktree: {root}')
    original = git(root, 'rev-parse', 'HEAD')
    branch = subprocess.run(['git', 'symbolic-ref', '--quiet', '--short', 'HEAD'],
                            cwd=root, capture_output=True, text=True).stdout.strip()
    relative = 'include/rmg.h' if header else 'src/bitmap16.cpp'
    source = (root / relative).read_text() + f'\n// Temporary merged performance workload {label}.\n'
    with tempfile.TemporaryDirectory(prefix='homm3-perf-index-') as directory:
        env = dict(os.environ, GIT_INDEX_FILE=str(Path(directory) / 'index'))
        git(root, 'read-tree', original, env=env)
        blob = git(root, 'hash-object', '-w', '--stdin', text=source)
        git(root, 'update-index', '--cacheinfo', f'100644,{blob},{relative}', env=env)
        tree = git(root, 'write-tree', env=env)
        commit = git(root, 'commit-tree', tree, '-p', original,
                     '-m', 'Disposable performance merge')
    git(root, 'switch', '--detach', original)
    try:
        git(root, 'merge', '--ff-only', commit)
        yield root
    finally:
        # No reset/restore: checkout refuses to discard unexpected local work.
        git(root, 'switch', '--detach', original)
        if branch:
            git(root, 'switch', branch)


@contextmanager
def cold(root: Path, destination: Path):
    if git(root, 'status', '--porcelain', '--untracked-files=no'):
        raise ValueError(f'cold benchmark requires committed candidate code: {root}')
    git(root, 'worktree', 'add', '--detach', str(destination), 'HEAD')
    build = destination / 'build'
    build.mkdir()
    # Immutable tools/game inputs only. No Ninja graph/dependencies, compiled
    # objects, generated labels, mirror, debug or ownership cache are copied.
    for name in ('orig', 'homm3-toolchain-vc6-sp3', 'toolchain'):
        existing = root / 'build' / name
        if existing.exists():
            if name == 'orig':
                shutil.copytree(existing, build / name)
            else:
                (build / name).symlink_to(existing.resolve(), target_is_directory=True)
    yield destination
