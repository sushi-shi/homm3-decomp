"""Verifier inputs must come from the selected image, without game fallback."""
import os
import unittest
from contextlib import ExitStack
from pathlib import Path
from tempfile import TemporaryDirectory
from types import SimpleNamespace
from unittest.mock import patch

from homm3.build.test_eh_handler_normalization import FixtureSection, _coff, _symbol
from homm3.verify import (shared_initializers, source_initializers,
                          source_padding, startup_bodies)


class ImageObjectTests(unittest.TestCase):
    modules = (source_padding, startup_bodies, shared_initializers, source_initializers)

    def check_loader(self, module, image_key, *, missing=False):
        raw = _coff((FixtureSection('.text', b'\xc3', ()),),
                    (_symbol('probe', 0, 1, 0x20, 2),))
        with TemporaryDirectory() as tmp, ExitStack() as stack:
            root = Path(tmp)
            (root / 'tools/bin').mkdir(parents=True)
            game = root / 'build/objdiff/base/probe.obj'
            chosen = (game if image_key == 'game' else
                      root / f'build/{image_key}/objdiff/base/probe.obj')
            chosen.parent.mkdir(parents=True, exist_ok=True)
            if not missing:
                chosen.write_bytes(raw)
            if chosen != game:
                game.parent.mkdir(parents=True, exist_ok=True)
                # Selecting the unrelated game object must fail visibly.
                game.write_bytes(b'not the selected image object')
            project = SimpleNamespace(root=root, toolchain=root / 'tools', includes=[],
                manifest={'unit': [dict(unit='probe', source='probe.cpp', flags='f')],
                          'flags': {'f': ['/O2']}})
            model = SimpleNamespace(functions=[], data=[])
            pe = SimpleNamespace(image_base=0x400000, read=lambda a, n: b'\xc3'[:n])
            image = SimpleNamespace(pe=pe, image_base=pe.image_base,
                                    relocs_in=lambda a, b: [], u32=lambda a: 0)
            stack.enter_context(patch.dict(os.environ, {'HOMM3_IMAGE': image_key}))
            stack.enter_context(patch('homm3.core.cc_wrap.scan_header_deps', return_value=[]))
            stack.enter_context(patch('homm3.core.compile_receipt.current',
                side_effect=lambda path, **kw: {'source': 'hash'} if path.is_file() else None))
            stack.enter_context(patch('homm3.delink.image.Image', return_value=image))
            stack.enter_context(patch('homm3.retail_labels.censuses.functions',
                                     return_value=[dict(rva=0x100, size=1)]))
            parsed = stack.enter_context(patch.object(module, 'Obj', wraps=module.Obj))
            if module is source_padding:
                loaded = module.Objects(project).get('probe')
                self.assertEqual(loaded is None, missing)
            else:
                stack.enter_context(patch.object(module, 'verified_roots', return_value=[0x100]))
                stack.enter_context(patch.object(module, 'read', return_value=([], [], [])))
                if module is source_initializers:
                    stack.enter_context(patch.object(module, 'declarations', return_value=({}, [])))
                    module.compare(project, pe, model, witness='probe')
                elif module is startup_bodies:
                    module.compare(project, pe, model, [])
                else:
                    module.compare(project, pe, model)
            if missing:
                parsed.assert_not_called()
            else:
                parsed.assert_called_once_with(chosen)

    def test_game_and_each_editor_read_their_own_objects(self):
        for module in self.modules:
            for image in ('game', 'h3maped', 'h3ccmped'):
                with self.subTest(module=module.__name__, image=image):
                    self.check_loader(module, image)

    def test_missing_editor_object_never_falls_back_to_game(self):
        for module in self.modules:
            for image in ('h3maped', 'h3ccmped'):
                with self.subTest(module=module.__name__, image=image):
                    self.check_loader(module, image, missing=True)


if __name__ == '__main__':
    unittest.main()
