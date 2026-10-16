"""A data pointer to the code after a catch funclet's rethrow call starts a
function; one after a call in a parent body does not."""
import unittest
from types import SimpleNamespace

from homm3.census import functions


class RethrowSeedsTest(unittest.TestCase):
    def census(self):
        c = SimpleNamespace(starts={0x1000: "call", 0x1100: "funclet:catch"})
        return c

    def test_pointer_after_a_funclet_call_is_a_seed(self):
        c = self.census()
        covered = {0x1105: 0x1100, 0x110a: 0x1100}
        ends = {0x110a: 0x1105}
        self.assertEqual(functions.rethrow_seeds(c, [(0x110a, 0x2000)], covered, ends),
                         [(0x110a, 0x2000)])

    def test_pointer_after_a_parent_call_is_not(self):
        c = self.census()
        covered = {0x1005: 0x1000, 0x100a: 0x1000}
        ends = {0x100a: 0x1005}
        self.assertEqual(functions.rethrow_seeds(c, [(0x100a, 0x2000)], covered, ends), [])

    def test_pointer_not_after_a_call_is_not(self):
        c = self.census()
        covered = {0x110a: 0x1100}
        self.assertEqual(functions.rethrow_seeds(c, [(0x110a, 0x2000)], covered, {}), [])


class ThrowSeedsTest(unittest.TestCase):
    """A data pointer to the code after a call of the throw helper (the
    callee a catch funclet's rethrow names) starts a function."""
    def census(self, target, padding=()):
        call = SimpleNamespace(mnemonic="call", operands=[
            SimpleNamespace(type=functions.x86.X86_OP_IMM, imm=0x400000 + target)])
        return SimpleNamespace(insn=lambda rva: call,
                               byte=lambda rva: 0x90 if rva in padding else 0x6a,
                               image=SimpleNamespace(image_base=0x400000))

    def test_pointer_after_a_throw_call_is_a_seed(self):
        c = self.census(0x5000)
        covered = {0x1005: 0x1000, 0x100a: 0x1000}
        ends = {0x100a: 0x1005}
        self.assertEqual(functions.throw_seeds(c, [(0x100a, 0x2000)], covered, ends, {0x5000}),
                         [(0x100a, 0x2000)])

    def test_pointer_after_another_call_is_not(self):
        c = self.census(0x6000)
        covered = {0x1005: 0x1000, 0x100a: 0x1000}
        ends = {0x100a: 0x1005}
        self.assertEqual(functions.throw_seeds(c, [(0x100a, 0x2000)], covered, ends, {0x5000}), [])

    def test_vtable_pointer_after_padded_throw_starts_a_function(self):
        c = self.census(0x5000, range(0x1005, 0x1010))
        covered = {0x1000: 0x1000, 0x1010: 0x1000}
        ends = {0x1005: 0x1000}
        self.assertEqual(functions.throw_seeds(c, [(0x1010, 0x2000)], covered, ends, {0x5000}),
                         [(0x1010, 0x2000)])

    def test_padded_ordinary_call_does_not_prove_a_boundary(self):
        c = self.census(0x6000, range(0x1005, 0x1010))
        covered = {0x1000: 0x1000, 0x1010: 0x1000}
        self.assertEqual(functions.throw_seeds(c, [(0x1010, 0x2000)], covered,
                                             {0x1005: 0x1000}, {0x5000}), [])

    def test_padded_throw_does_not_claim_another_functions_bytes(self):
        c = self.census(0x5000, range(0x1005, 0x1010))
        covered = {0x1000: 0x1000, 0x1010: 0x1008}
        self.assertEqual(functions.throw_seeds(c, [(0x1010, 0x2000)], covered,
                                             {0x1005: 0x1000}, {0x5000}), [])

    def test_the_helper_is_a_catch_funclet_s_last_call(self):
        call = SimpleNamespace(mnemonic="call", operands=[
            SimpleNamespace(type=functions.x86.X86_OP_IMM, imm=0x405000)])
        ret = SimpleNamespace(mnemonic="ret", operands=[])
        insns = {0x1100: ret, 0x1105: call, 0x1200: call, 0x1205: ret}
        c = SimpleNamespace(starts={0x1100: "funclet:catch", 0x1200: "call"},
                            reached={0x1100: {0x1100, 0x1105}, 0x1200: {0x1200, 0x1205}},
                            insn=insns.get, image=SimpleNamespace(image_base=0x400000))
        self.assertEqual(functions.throw_helpers(c), {0x5000})


class AdjustingThunkTest(unittest.TestCase):
    """A this-adjusting thunk is `add/sub ecx, n` and its jump."""
    def census(self, mnemonic, reg):
        first = SimpleNamespace(mnemonic=mnemonic, operands=[
            SimpleNamespace(type=functions.x86.X86_OP_REG, reg=reg)])
        return SimpleNamespace(insn=lambda rva: first)

    def test_add_ecx_then_jump_is_a_thunk(self):
        c = self.census("add", functions.x86.X86_REG_ECX)
        self.assertTrue(functions.adjusting_thunk(c, 0x1000, {0x1000, 0x1003}))

    def test_a_longer_body_is_not(self):
        c = self.census("add", functions.x86.X86_REG_ECX)
        self.assertFalse(functions.adjusting_thunk(c, 0x1000, {0x1000, 0x1003, 0x1008}))

    def test_another_register_is_not(self):
        c = self.census("add", functions.x86.X86_REG_EAX)
        self.assertFalse(functions.adjusting_thunk(c, 0x1000, {0x1000, 0x1003}))


class TableSeedsAfterCallsTest(unittest.TestCase):
    def test_vtable_slot_after_a_call_is_a_seed(self):
        covered = {0x1005: 0x1000, 0x100a: 0x1000}
        ends = {0x100a: 0x1005}
        vtable = {0x3000 + 4 * i for i in range(10)}
        self.assertEqual(functions.table_seeds_after_calls([(0x100a, 0x3000)], covered, ends, vtable),
                         [(0x100a, 0x3000)])

    def test_short_eh_table_slot_is_not(self):
        covered = {0x1005: 0x1000, 0x100a: 0x1000}
        ends = {0x100a: 0x1005}
        scope = {0x3000, 0x3004}
        self.assertEqual(functions.table_seeds_after_calls([(0x100a, 0x3000)], covered, ends, scope), [])

    def test_slot_not_after_a_call_is_not(self):
        covered = {0x100a: 0x1000}
        self.assertEqual(functions.table_seeds_after_calls([(0x100a, 0x3000)], covered, {}, {0x3000}), [])


class PaddedCallEndsTest(unittest.TestCase):
    def census(self, pad):
        data = {0x1005 + i: 0x90 for i in range(pad)}
        return SimpleNamespace(byte=lambda rva: data.get(rva, 0x6a))

    def test_padding_after_a_call_ends_on_the_next_function(self):
        self.assertEqual(functions.padded_call_ends(self.census(11), {0x1005: 0x1000}), {0x1010: 0x1000})

    def test_no_padding_is_no_end(self):
        self.assertEqual(functions.padded_call_ends(self.census(0), {0x1005: 0x1000}), {})

    def test_padding_longer_than_an_alignment_is_no_end(self):
        self.assertEqual(functions.padded_call_ends(self.census(20), {0x1005: 0x1000}), {})


    def test_data_pointer_after_padding_seeds_the_next_function(self):
        ends = {0x1005: 0x1000}
        covered = {0x1010: 0x1000}
        self.assertEqual(functions.padded_data_seeds(self.census(11), [(0x1010, 0x3000)], covered, ends),
                         [(0x1010, 0x3000)])

    def test_data_pointer_into_the_padding_is_not(self):
        ends = {0x1005: 0x1000}
        covered = {0x1008: 0x1000}
        self.assertEqual(functions.padded_data_seeds(self.census(11), [(0x1008, 0x3000)], covered, ends), [])


if __name__ == '__main__':
    unittest.main()
