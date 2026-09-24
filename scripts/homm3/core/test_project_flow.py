"""Boundary controls: different projects, profiles, libraries and image layouts."""
from pathlib import Path
import hashlib
import json
import os
import struct
import subprocess
import tempfile
import unittest
from unittest.mock import patch

from homm3.core import clang
from homm3.core.compiler_profile import Profiles, arguments
from homm3.core.project import Project


def fixture(image_base: int = 0x10000) -> bytes:
    """A minimal one-section PE image; no game bytes are required."""
    image = bytearray(0x400)
    image[:2] = b"MZ"
    struct.pack_into("<I", image, 0x3c, 0x80)
    image[0x80:0x84] = b"PE\0\0"
    struct.pack_into("<HH", image, 0x84, 0x1a6, 1)
    struct.pack_into("<H", image, 0x94, 0xe0)
    optional = 0x98
    struct.pack_into("<H", image, optional, 0x10b)
    struct.pack_into("<I", image, optional + 28, image_base)
    struct.pack_into("<8s4I", image, optional + 0xe0, b".text", 0x200, 0x1000, 0x200, 0x200)
    return bytes(image)


class ProjectFlowTest(unittest.TestCase):
    def setUp(self):
        self.root = Path(self.enterContext(tempfile.TemporaryDirectory()))
        (self.root / 'config').mkdir()
        self.compiler = clang.clang_bin()
        self.enterContext(patch.dict(os.environ, {}, clear=True))

    def executable(self, root, data, base):
        (root / 'config').mkdir(exist_ok=True)
        (root / 'input.exe').write_bytes(data)
        entry = (f'name="fixture"\npath="input.exe"\nsize={len(data)}\n'
                 f'sha256="{hashlib.sha256(data).hexdigest()}"\n'
                 'env="TEST_EXE"\noption="--exe"\n')
        (root / 'config/project.toml').write_text(
            '[inputs.retail]\n' + entry + f'image_base={base}\n')
        return Project(root)

    def test_context_selects_its_executable_and_layout_without_global_input(self):
        first = self.executable(self.root, fixture(), 0x10000)
        second_root = self.root / 'second'
        second_root.mkdir()
        second = self.executable(second_root, fixture(0x80000), 0x80000)
        self.assertEqual(first.image.image_base, 0x10000)
        self.assertEqual(second.image.image_base, 0x80000)

    def test_project_snapshot_does_not_survive_a_later_operation(self):
        project = self.executable(self.root, fixture(), 0x10000)
        self.assertEqual(project.specification['inputs']['retail']['image_base'], 0x10000)
        path = self.root / 'config/project.toml'
        path.write_text(path.read_text().replace('image_base=65536', 'image_base=524288'))
        self.assertEqual(Project(self.root).specification['inputs']['retail']['image_base'], 0x80000)
        with self.assertRaisesRegex(ValueError, 'image base'):
            _ = Project(self.root).image

    def test_named_editor_image_does_not_replace_game_input(self):
        project = self.executable(self.root, fixture(), 0x10000)
        editor = fixture(0x80000)
        (self.root / 'editor.exe').write_bytes(editor)
        with (self.root / 'config/project.toml').open('a') as stream:
            stream.write('[inputs.h3maped]\nname="editor"\npath="editor.exe"\n'
                         f'size={len(editor)}\nsha256="{hashlib.sha256(editor).hexdigest()}"\n'
                         'env="EDITOR_EXE"\noption="--map-editor-exe"\nimage_base=524288\n')
        self.assertEqual(project.input_image('h3maped').image_base, 0x80000)
        self.assertEqual(project.image.image_base, 0x10000)
        (self.root / 'editor.exe').write_bytes(b'bad')
        with self.assertRaisesRegex(ValueError, 'size'):
            project.input_image('h3maped')

    def test_ownership_claims_come_from_requested_root(self):
        from homm3.match import source_ownership as ownership
        from homm3.retail_labels import fragments
        project = self.executable(self.root, fixture(), 0x10000)
        project.fragments.mkdir(parents=True)
        (project.fragments / 'probe.tsv').write_text(
            '\t'.join(fragments.HEADER) + '\n'
            '0x1000\t0x10\tprobe\tfunc\tsrc-VA\tprobe\t0\t\t\n')
        with patch.object(ownership, 'collect', return_value=([], [], [])), \
                patch.object(ownership, 'claim_identity', return_value=[]) as identity, \
                patch.object(fragments, 'FRAGMENTS', self.root / 'wrong'):
            ownership.audit(self.root)
        self.assertEqual(identity.call_args.args[1][0].unit, 'probe')
        self.assertEqual(identity.call_args.kwargs['image_base'], 0x10000)

    @unittest.skipUnless(clang.clang_bin(), 'requires Clang')
    def test_profiles_change_mangling_and_preprocessor_branches_together(self):
        source = self.root / 'probe.cpp'
        source.write_text('int selected(int x) { return x; }\n'
                          '#ifdef PROFILE_SWITCH\nint enabled() { return 1; }\n#endif\n'
                          '#ifdef _MT\nint threaded() { return 1; }\n#endif\n')
        for flags, symbol, enabled, threaded in (
                (['/Gr', '/ML'], '?selected@@YIHH@Z', False, False),
                (['/Gd', '/MT', '/DPROFILE_SWITCH'], '?selected@@YAHH@Z', True, True)):
            command = [self.compiler, *arguments(flags, []),
                       '-Xclang', '-emit-llvm', '-o', str(self.root / 'probe.ll'), '-c', str(source)]
            subprocess.run(command, capture_output=True, text=True, check=True)
            ir = (self.root / 'probe.ll').read_text()
            self.assertIn(symbol, ir)
            self.assertEqual('enabled@@' in ir, enabled)
            self.assertEqual('threaded@@' in ir, threaded)

    def test_profile_selection_and_header_policy_share_translation(self):
        (self.root / 'config/units.toml').write_text(
            '[build]\nincludes=["sdk"]\nanalysis_profile="header"\n'
            '[flags]\nheader=["/Gr", "/GX"]\ntu=["/Gd", "/DUNIT"]\n'
            '[[unit]]\nunit="probe"\nsource="probe.cpp"\nflags="tu"\n')
        with patch.object(clang, 'mirror', return_value=None):
            profiles = Profiles(Project(self.root))
        self.assertIn('/Gd', profiles.for_source(self.root / 'probe.cpp'))
        self.assertNotIn('/Gr', profiles.for_source(self.root / 'probe.cpp'))
        self.assertIn('/Gr', profiles.for_source(self.root / 'orphan.h'))
        self.assertIn('/TP', profiles.for_source(self.root / 'orphan.h'))
        self.assertIn(str(self.root / 'sdk'), profiles.for_source(self.root / 'probe.cpp'))

    def test_library_reader_has_no_cross_toolchain_or_replacement_cache(self):
        from homm3.retail_labels.iat import implib_decorations
        def archive(directory, name):
            directory.mkdir(exist_ok=True)
            symbol = name.encode() + b'\0'
            body = struct.pack('>II', 1, 0) + symbol
            header = b'/' + bytes(47) + str(len(body)).encode().ljust(10) + b'`\n'
            (directory / 'IMPORT.LIB').write_bytes(b'!<arch>\n' + header + body)
        a, b = self.root / 'a', self.root / 'b'
        archive(a, '__imp__Function@4')
        archive(b, '__imp__Function@8')
        self.assertEqual(implib_decorations(a)['Function'], '__imp__Function@4')
        self.assertEqual(implib_decorations(b)['Function'], '__imp__Function@8')
        archive(a, '__imp__Function@12')
        self.assertEqual(implib_decorations(a)['Function'], '__imp__Function@12')

    def test_toolchain_override_supplies_the_library_directory(self):
        project = self.executable(self.root, fixture(), 0x10000)
        toolchain = self.root / 'alternate-msvc'
        (toolchain / 'bin').mkdir(parents=True)
        (toolchain / 'bin/cl.exe').touch()
        with patch.dict(os.environ, {'MSVC_DIR': str(toolchain)}):
            self.assertEqual(project.toolchain / 'lib', toolchain / 'lib')

    def test_carrier_policy_keeps_all_inline_bindings(self):
        from types import SimpleNamespace
        from homm3.model import carrier_policy
        policy = carrier_policy({('a', 'one'): SimpleNamespace(rva=0x1000),
                                 ('b', 'two'): SimpleNamespace(rva=0x1000)})
        self.assertEqual(set(policy.bindings), {('a', 0x1000), ('b', 0x1000)})
        self.assertEqual(policy.choose(0x1000, set(), policy.banked, []), 'b')
        self.assertEqual(policy.for_units({'a'}), {0x1000: 'a'})

    def test_report_read_is_pure_and_production_failure_is_visible(self):
        from homm3.build.report import generate
        from homm3.match.status import load_report
        report = self.root / 'report.json'
        report.write_text(json.dumps({'units': []}))
        with patch('subprocess.run', side_effect=AssertionError('reader ran a tool')):
            self.assertEqual(load_report(report), {'units': []})
        def failed(*args, **kwargs):
            return subprocess.CompletedProcess(args, 1, '', 'broken report')
        with self.assertRaisesRegex(RuntimeError, 'broken report'):
            generate(self.root, run=failed)
