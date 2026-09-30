import os
from pathlib import Path
from tempfile import TemporaryDirectory
import unittest
from unittest.mock import patch

from homm3.core import compile_receipt as receipt
from homm3.core.cc_wrap import scan_header_deps


class CompileReceiptTests(unittest.TestCase):
    def test_content_not_timestamp_and_flags_controls_freshness(self):
        with TemporaryDirectory() as tmp:
            src, obj = Path(tmp, 'source.cpp'), Path(tmp, 'source.obj')
            src.write_text('source'); obj.write_bytes(b'object')
            stamp = src.stat()
            inputs = receipt.snapshot([src])
            receipt.publish(obj, inputs, ['/O2'])
            self.assertEqual(receipt.current(obj, flags=['/O2'], required=[src]), inputs)
            self.assertIsNone(receipt.current(obj, flags=['/O1'], required=[src]))
            self.assertIsNone(receipt.current(obj, flags=['/O2'], required=[obj]))
            src.write_text('edited')
            os.utime(src, ns=(stamp.st_atime_ns, stamp.st_mtime_ns))
            self.assertIsNone(receipt.current(obj, flags=['/O2'], required=[src]))
            with self.assertRaises(ValueError):
                receipt.publish(obj, inputs, ['/O2'])
            src.write_text('source'); obj.write_bytes(b'changed object')
            self.assertIsNone(receipt.current(obj, flags=['/O2'], required=[src]))

    def test_dependency_scan_reaches_original_uppercase_sdk(self):
        with TemporaryDirectory() as tmp:
            root = Path(tmp)
            src = root / 'source.cpp'
            src.write_text('#include <bitset>\n')
            sdk = root / 'sdk'; sdk.mkdir()
            (sdk / 'BITSET').write_text('#include <xstddef>\n')
            (sdk / 'XSTDDEF').write_text('typedef unsigned int size_t;\n')
            self.assertEqual(set(scan_header_deps(src, sdk)),
                             {str(sdk / 'BITSET'), str(sdk / 'XSTDDEF')})

    def test_shared_dependency_pass_reads_headers_once_and_next_pass_sees_edits(self):
        with TemporaryDirectory() as tmp:
            root = Path(tmp)
            first, second = root/'first.cpp', root/'second.cpp'
            common, added = root/'common.h', root/'added.h'
            for src in (first, second):
                src.write_text('#include "common.h"\n')
            common.write_text('struct Value {};\n')
            added.write_text('struct More {};\n')
            reads, read_text = [], Path.read_text
            def counted(path, *args, **kwargs):
                reads.append(path)
                return read_text(path, *args, **kwargs)
            with patch.object(Path, 'read_text', counted):
                cache = {}
                for src in (first, second):
                    self.assertEqual(scan_header_deps(src, root, cache=cache), [str(common)])
            self.assertEqual(reads.count(common), 1)
            common.write_text('#include "added.h"\n')
            self.assertEqual(set(scan_header_deps(first, root)), {str(common), str(added)})


if __name__ == '__main__':
    unittest.main()
