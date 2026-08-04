#!/usr/bin/env python3
"""homm3.core.image - the PE32 reader the pipeline reasons with.

Semantics mirror the vendored `find_relocs.Image` byte for byte (that file
is vendored VERBATIM from vostok and retires with the carve package;
this reader is ours and stays): `size` is the readable extent, `mapped`
the virtual one - they differ on a zero-filled tail, and containment
tests use `mapped`.
"""
from __future__ import annotations

import struct
import sys
from collections import namedtuple

IMAGE_SCN_MEM_EXECUTE = 0x20000000
IMAGE_SCN_CNT_UNINITIALIZED_DATA = 0x00000080
PE32_MAGIC = 0x10B

Section = namedtuple("Section", "name rva size mapped raw_offset executable")


def _die(message):
    print(f"[homm3] ERROR: {message}", file=sys.stderr)
    raise SystemExit(1)


class Image:
    """The parts of a PE32 file the pipeline reasons about."""

    def __init__(self, path):
        self.path = path
        self.data = open(path, "rb").read()
        data = self.data
        if data[:2] != b"MZ":
            _die(f"{path}: not a PE image")
        pe = struct.unpack_from("<I", data, 0x3C)[0]
        if data[pe:pe + 4] != b"PE\0\0":
            _die(f"{path}: no PE signature")
        machine = struct.unpack_from("<H", data, pe + 4)[0]
        section_count = struct.unpack_from("<H", data, pe + 6)[0]
        optional_size = struct.unpack_from("<H", data, pe + 20)[0]
        optional = pe + 24
        if struct.unpack_from("<H", data, optional)[0] != PE32_MAGIC:
            _die(f"{path}: not PE32 (machine 0x{machine:x}); "
                 "only 32-bit HIGHLOW images are supported")
        self.image_base = struct.unpack_from("<I", data, optional + 28)[0]
        self.image_size = struct.unpack_from("<I", data, optional + 56)[0]
        self.image_end = self.image_base + self.image_size

        self.sections = []
        for index in range(section_count):
            entry = optional + optional_size + index * 40
            name = data[entry:entry + 8].rstrip(b"\0").decode("latin-1")
            virtual_size, rva, raw_size, raw_offset = struct.unpack_from(
                "<4I", data, entry + 8)
            flags = struct.unpack_from("<I", data, entry + 36)[0]
            if flags & IMAGE_SCN_CNT_UNINITIALIZED_DATA or raw_size == 0:
                continue
            self.sections.append(Section(
                name, rva, min(virtual_size, raw_size) or raw_size,
                max(virtual_size, raw_size), raw_offset,
                bool(flags & IMAGE_SCN_MEM_EXECUTE)))

    def section_of(self, rva):
        for section in self.sections:
            if section.rva <= rva < section.rva + section.mapped:
                return section
        return None

    def blob(self, section):
        return self.data[section.raw_offset:section.raw_offset + section.size]

    def in_image(self, value):
        return self.image_base <= value < self.image_end

    def rva_of(self, value):
        return value - self.image_base
