"""The catch-scope census must read a real FuncInfo and reject a fake one.

The negative control is the whole point of the record scan: 0x19930520 is an
ordinary dword that can appear anywhere in .rdata, so the walker is only
trustworthy if a lookalike whose pointers do not resolve is REJECTED.
"""
import struct
import unittest

from homm3.core.image import Section
from homm3.vc6 import tryblocks


BASE = 0x400000
TEXT_RVA, RDATA_RVA = 0x1000, 0x2000


class _Image:
    """The three attributes tryblocks reads: sections, data, image_base."""

    def __init__(self, text: bytes, rdata: bytes):
        self.image_base = BASE
        self.data = bytes(0x400) + text.ljust(0x1000, b"\0") + rdata
        self.sections = [
            Section(".text", TEXT_RVA, 0x1000, 0x1000, 0x400, True),
            Section(".rdata", RDATA_RVA, len(rdata), len(rdata), 0x1400, False),
        ]


def _build(*, valid=True):
    """One function at 0x1000 with a `catch (TThrown)` over states 0..1."""
    type_rva = RDATA_RVA + 0x100
    handler_rva = RDATA_RVA + 0x140
    tryblock_rva = RDATA_RVA + 0x160
    unwind_rva = RDATA_RVA + 0x180
    funclet_rva = TEXT_RVA + 0x80
    info_rva = RDATA_RVA + 0x1c0

    rdata = bytearray(0x200)

    def put(rva, blob):
        rdata[rva - RDATA_RVA:rva - RDATA_RVA + len(blob)] = blob

    put(type_rva, struct.pack("<II", 0, 0) + b".?AVTThrown@@\0")
    put(handler_rva, struct.pack("<IIiI", 0, type_rva + BASE, 0, funclet_rva + BASE))
    put(tryblock_rva, struct.pack("<iiiiI", 0, 1, 2, 1, handler_rva + BASE))
    put(unwind_rva, struct.pack("<iIiI", -1, 0, 0, 0))
    # magic, maxState, pUnwindMap, nTryBlocks, pTryBlockMap, nIPMap, pIPMap
    bad = 0 if valid else 0xDEAD0000
    put(info_rva, struct.pack("<IiIIIII", 0x19930520, 2,
                              (unwind_rva + BASE) if valid else bad,
                              1, (tryblock_rva + BASE) if valid else bad, 0, 0))

    text = bytearray(0x100)
    # the __ehhandler stub: mov eax,<FuncInfo> ; jmp __CxxFrameHandler
    stub = 0xC0
    text[stub:stub + 10] = b"\xb8" + struct.pack("<I", info_rva + BASE) + \
        b"\xe9" + struct.pack("<i", 0)
    # the guarded body's prologue push of that stub
    text[0x04:0x09] = b"\x68" + struct.pack("<I", TEXT_RVA + stub + BASE)
    return _Image(bytes(text), bytes(rdata))


class TryBlockCensusTest(unittest.TestCase):
    def test_reads_extent_type_and_funclet(self):
        infos = [i for i in tryblocks.func_infos(_build()) if i["n_try"]]
        self.assertEqual(len(infos), 1)
        (low, high, catch_high, handlers), = infos[0]["tries"]
        self.assertEqual((low, high, catch_high), (0, 1, 2))
        _, name, disp, addr = handlers[0]
        self.assertEqual((name, disp, addr), (".?AVTThrown@@", 0, TEXT_RVA + 0x80))

    def test_catch_all_has_no_type_descriptor(self):
        image = _build()
        # blank the HandlerType's pType: a NULL descriptor is `catch (...)`
        data = bytearray(image.data)
        off = 0x1400 + 0x140 + 4
        data[off:off + 4] = b"\0\0\0\0"
        image.data = bytes(data)
        infos = [i for i in tryblocks.func_infos(image) if i["n_try"]]
        self.assertEqual(infos[0]["tries"][0][3][0][1], "...")

    def test_negative_control_lookalike_magic_is_rejected(self):
        # same magic, unresolvable map pointers - must not become a record
        self.assertEqual(
            [i for i in tryblocks.func_infos(_build(valid=False)) if i["n_try"]], [])

    def test_stub_and_push_site_resolve_the_guarded_body(self):
        image = _build()
        infos = tryblocks.func_infos(image)
        stubs = tryblocks.handler_stubs(image, infos)
        self.assertEqual(list(stubs.values()), [TEXT_RVA + 0xC0])
        sites = tryblocks.push_sites(image, stubs.values())
        self.assertEqual(sites[TEXT_RVA + 0xC0], [TEXT_RVA + 0x04])


if __name__ == "__main__":
    unittest.main()
