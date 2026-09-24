"""Paths and admitted inputs for one operation; no process-wide project cache."""
from __future__ import annotations

from dataclasses import dataclass
from functools import cached_property
from pathlib import Path
import tomllib


@dataclass(frozen=True)
class Project:
    root: Path

    def __post_init__(self):
        object.__setattr__(self, 'root', self.root.resolve())

    @cached_property
    def specification(self) -> dict:
        with (self.root / 'config/project.toml').open('rb') as stream:
            return tomllib.load(stream)

    @cached_property
    def manifest(self) -> dict:
        from homm3 import manifest
        return manifest.load(self.root / 'config/units.toml')

    @property
    def includes(self) -> list[Path]:
        return [self.root / p for p in self.manifest['build'].get('includes', ['include'])]

    @cached_property
    def toolchain(self) -> Path:
        from homm3.core.cc_wrap import msvc_dir
        return msvc_dir(self.root)

    @property
    def fragments(self) -> Path:
        return self.root / 'build/gen/claims'

    def executable(self, key):
        from homm3.core.inputs import Executable
        row = self.specification['inputs'][key]
        return Executable(row['name'], self.root / row['path'], row['size'],
                          row['sha256'], row['env'], row['option'])

    @cached_property
    def image(self):
        return self.input_image('retail')

    def input_image(self, key):
        """Load a named, verified input without replacing the game context."""
        from homm3.core.inputs import stage_executable
        from homm3.core.image import Image
        image = Image(stage_executable(self.executable(key)))
        expected = self.specification['inputs'][key].get('image_base')
        if expected is not None and image.image_base != expected:
            raise ValueError(f'image base {image.image_base:#x} != admitted {expected:#x}')
        return image
