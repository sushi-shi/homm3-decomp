"""Harness contracts; these tests do not substitute a mock generator for retail."""
import json
from pathlib import Path
import struct
import tempfile
import subprocess
import unittest
from unittest.mock import patch
from unittest.mock import Mock

from homm3.build.canonicalize_data_symbols import CoffObject
from .bindings import absolute_object, prepare_calls, undefined_symbols, DRIVER_BASE, resolve
from .bootstrap import offset, patch_winmain, imports
from .cases import validate_case, job_bytes, decode_result, byte_difference, compare_runs, load_cases
from .__main__ import driver_include_flags, run_case, selected_cases, unsigned


def object_fixture():
    code = b'\xe8\0\0\0\0\xe8\0\0\0\0' + b'\0' * 4
    relocations = b''.join(struct.pack('<IIH', site, symbol, kind)
                           for site, symbol, kind in ((1, 0, 0x14), (6, 1, 0x14), (10, 0, 6)))
    symbols = b''.join(name.ljust(8, b'\0') + struct.pack('<IhHBB', value, section, 0, 2, 0)
                       for name, value, section in ((b'foo', 0, 0), (b'bar', 0, 0),
                                                    (b'common', 4, 0), (b'own', 0, 1)))
    return (struct.pack('<HHIIIHH', 0x14c, 1, 0, 60 + len(code) + len(relocations), 4, 0, 0)
            + struct.pack('<8sIIIIIIHHI', b'.text', 0, 0, len(code), 60,
                          60 + len(code), 0, 3, 0, 0x60000020)
            + code + relocations + symbols + struct.pack('<I', 4))


def executable_fixture():
    data = bytearray(0x1200)
    data[:2] = b'MZ'
    struct.pack_into('<I', data, 0x3c, 0x80)
    data[0x80:0x84] = b'PE\0\0'
    struct.pack_into('<HH', data, 0x84, 0x14c, 1)
    struct.pack_into('<H', data, 0x94, 224)
    struct.pack_into('<H', data, 0x98, 0x10b)
    struct.pack_into('<I', data, 0x98 + 28, 0x400000)
    struct.pack_into('<I', data, 0x98 + 104, 0xf7100)
    struct.pack_into('<8sIIIIIIHHI', data, 0x178, b'.text', 0x1000, 0xf7000,
                     0x1000, 0x200, 0, 0, 0, 0, 0x60000020)
    struct.pack_into('<5I', data, 0x300, 0xf7180, 0, 0, 0, 0xf71a0)
    struct.pack_into('<3I', data, 0x380, 0xf71c0, 0xf71e0, 0)
    data[0x3c2:0x3cf] = b'LoadLibraryA\0'
    data[0x3e2:0x3f1] = b'GetProcAddress\0'
    return bytes(data)


class BindingTests(unittest.TestCase):
    def test_absolute_symbols_roundtrip(self):
        values = {'short': 0, '?aLongMangledName@@': 0x617e5c}
        obj = CoffObject(absolute_object(values))
        self.assertEqual({s.name: s.value for s in obj.symbols.values()}, values)
        self.assertTrue(all(s.section == -1 for s in obj.symbols.values()))

    def test_relative_absolute_calls_only(self):
        original = object_fixture()
        relocated = prepare_calls(original, {'foo': 0x617e5c})
        self.assertEqual(struct.unpack_from('<I', relocated, 61)[0], (-DRIVER_BASE) & 0xffffffff)
        self.assertEqual(relocated[:61], original[:61])
        self.assertEqual(relocated[65:], original[65:])
        # LINK subtracts site RVA + 4, then the CPU adds its actual VA + 4.
        for site_rva in (0x1001, 0xabcdef):
            displacement = (0x617e5c - DRIVER_BASE - site_rva - 4) & 0xffffffff
            self.assertEqual((DRIVER_BASE + site_rva + 4 + displacement) & 0xffffffff, 0x617e5c)

    def test_common_symbols_are_definitions(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / 'fixture.obj'
            path.write_bytes(object_fixture())
            self.assertEqual(undefined_symbols([path]), {'foo', 'bar'})

    def test_missing_generator_symbol_cannot_fall_back(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            (root / 'build/gen').mkdir(parents=True)
            (root / 'build/gen/symbol_names.csv').write_text('rva,name,unit\n0x1000,foo,rmg\n')
            path = root / 'fixture.obj'
            path.write_bytes(object_fixture())
            with patch('homm3.rmg.bindings.data_addresses', return_value={}):
                with self.assertRaisesRegex(ValueError, 'no retail fallback'):
                    resolve(root, [path], b'')


class BootstrapTests(unittest.TestCase):
    def test_bootstrap_keeps_image_layout_and_entrypoint(self):
        original = executable_fixture()
        result = patch_winmain(original)
        start = offset(original, 0x4f7a30)
        self.assertEqual(len(result), len(original))
        self.assertEqual(result[:start], original[:start])
        self.assertEqual(result[start + 0x1cf:], original[start + 0x1cf:])
        self.assertEqual(imports(result), {'LoadLibraryA': 0x4f71a0, 'GetProcAddress': 0x4f71a4})
        for branch in (14, 30):
            target = start + branch + 1 + result[start + branch]
            self.assertEqual(result[target:target + 5], b'\xb8\x7e\0\0\0')
        self.assertEqual(result[start + 0x80:start + 0x8f], b'rmg-driver.dll\0')


class CaseTests(unittest.TestCase):
    def test_direct_seed_case(self):
        case, = selected_cases(None, 0x12345678, 0xffffffff, 0xff)
        self.assertEqual(case['name'], 'seed-12345678')
        self.assertEqual((case['seed'], case['stackWord'], case['heapByte']),
                         (0x12345678, 0xffffffff, 0xff))
        self.assertEqual(unsigned('0xffffffff', 0xffffffff, '--seed'), 0xffffffff)
        with self.assertRaises(ValueError):
            selected_cases(Path('cases.json'), None, 1, None)

    def test_request_layout_and_result_signed_code(self):
        case = validate_case({'name': 'small', 'seed': 0xffffffff})
        job = job_bytes(case)
        self.assertEqual(len(job), 92)
        self.assertEqual(struct.unpack_from('<III', job), (0xffffffff, 0, 0))
        self.assertEqual(struct.unpack_from('<3i', job, 52), (36, 36, 1))
        raw = b'RMG1' + struct.pack('<iII', -10, 123, 0x27f) + job[12:]
        state = decode_result(raw)
        self.assertEqual(state['returnCode'], -10)
        self.assertEqual(state['request']['townType'], [-1] * 8)

    def test_memory_inputs_and_native_heap_sentinel(self):
        for value in (0, 85, 255, None):
            case = validate_case({'name': 'memory', 'seed': 1,
                                  'stackWord': 0x12345678, 'heapByte': value})
            self.assertEqual(struct.unpack_from('<III', job_bytes(case)),
                             (1, 0x12345678, 0xffffffff if value is None else value))

    def test_bad_cases_fail_before_execution(self):
        for fields in ({'name': '../escape'}, {'seed': -1}, {'seed': True},
                       {'stackWord': -1}, {'stackWord': 0x100000000},
                       {'heapByte': -1}, {'heapByte': 256}, {'heapByte': True},
                       {'width': 35}, {'levels': 3}, {'typo': 1}, {'isHumanSeat': [0]},
                       {'humanPlayerCount': 8, 'computerPlayerCount': 7}):
            with self.subTest(fields=fields), self.assertRaises(ValueError):
                validate_case(dict({'name': 'valid', 'seed': 0}, **fields))
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / 'cases.json'
            path.write_text(json.dumps([{'name': 'x', 'seed': 1}] * 2))
            with self.assertRaisesRegex(ValueError, 'duplicate'):
                load_cases(path)

    def test_difference_including_length_and_empty_maps(self):
        self.assertTrue(byte_difference(b'', b'')['equal'])
        self.assertEqual(byte_difference(b'ab', b'abc')['firstDifference'], 2)
        self.assertEqual(byte_difference(b'abc', b'axc')['firstDifference'], 1)

    def test_positive_and_mutation_controls(self):
        with tempfile.TemporaryDirectory() as tmp:
            left, right = Path(tmp) / 'left', Path(tmp) / 'right'
            state = b'RMG1' + struct.pack('<iII', 0, 123, 0x27f) + job_bytes(
                validate_case({'name': 'small', 'seed': 1}))[12:]
            for directory in (left, right):
                directory.mkdir()
                (directory / 'result.bin').write_bytes(state)
                (directory / 'map.raw').write_bytes(b'original map')
            self.assertTrue(compare_runs(left, right)['equal'])
            (right / 'map.raw').write_bytes(b'original Map')
            self.assertFalse(compare_runs(left, right)['equal'])
            (right / 'map.raw').write_bytes(b'original map')
            mutated = bytearray(state)
            mutated[8] ^= 1
            (right / 'result.bin').write_bytes(mutated)
            self.assertFalse(compare_runs(left, right)['equal'])
            (right / 'result.bin').write_bytes(b'RMG1')
            with self.assertRaises(ValueError):
                compare_runs(left, right)


class ProcessTests(unittest.TestCase):
    def test_crashes_and_timeouts_are_not_map_differences(self):
        with tempfile.TemporaryDirectory() as tmp:
            out = Path(tmp)
            process = Mock(pid=123)

            def crashing_child(*args, **kwargs):
                (kwargs['cwd'] / 'failure.bin').write_bytes(struct.pack('<III', 0xc0000005, 0x12345678, 8))
                process.wait.return_value = 131
                return process

            with patch('homm3.rmg.__main__.winepath_w', side_effect=str), \
                 patch('homm3.rmg.__main__.subprocess.Popen', side_effect=crashing_child):
                result = run_case(out, 'crash', 'candidate', b'', out, [], 1)
            self.assertEqual(result['status'], 'crash')
            self.assertEqual(result['address'], '0x12345678')
            process.wait.side_effect = [subprocess.TimeoutExpired('wine', 1), -9]
            with patch('homm3.rmg.__main__.winepath_w', side_effect=str), \
                 patch('homm3.rmg.__main__.subprocess.Popen', return_value=process), \
                 patch('homm3.rmg.__main__.os.killpg') as kill:
                result = run_case(out, 'timeout', 'candidate', b'', out, [], 1)
            self.assertEqual(result['status'], 'timeout')
            kill.assert_called_once()
            self.assertEqual(kill.call_args.args[0], 123)

    def test_successful_exit_without_output_is_invalid(self):
        with tempfile.TemporaryDirectory() as tmp:
            out = Path(tmp)
            process = Mock()
            process.wait.return_value = 0
            with patch('homm3.rmg.__main__.winepath_w', side_effect=str), \
                 patch('homm3.rmg.__main__.subprocess.Popen', return_value=process):
                result = run_case(out, 'missing', 'candidate', b'', out, [], 1)
            self.assertEqual(result['status'], 'invalid-output')

    def test_launcher_disables_interactive_wine_debugger(self):
        with tempfile.TemporaryDirectory() as tmp:
            out = Path(tmp)
            process = Mock()
            process.wait.return_value = 1
            with patch('homm3.rmg.__main__.winepath_w', side_effect=str), \
                 patch('homm3.rmg.__main__.subprocess.Popen', return_value=process) as popen:
                run_case(out, 'failure', 'retail', b'', out, [], 1)
            self.assertEqual(popen.call_args.kwargs['env']['WINEDLLOVERRIDES'],
                             'winedbg.exe=d')


class BuildTests(unittest.TestCase):
    def test_driver_include_fallback_is_local_and_ordered(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            toolchain = root / 'msvc'
            includes = [root / 'first', root / 'second']
            for directory in includes:
                directory.mkdir()
            with patch('homm3.rmg.__main__.msvc_dir', return_value=toolchain), \
                 patch('homm3.rmg.__main__.Project',
                       return_value=Mock(includes=includes)), \
                 patch('homm3.rmg.__main__.winepath_w', side_effect=lambda path: 'Z:' + str(path)):
                self.assertEqual(driver_include_flags(),
                                 ['/IZ:' + str(path) for path in
                                  [toolchain / 'include', *includes]])


if __name__ == '__main__':
    unittest.main()
