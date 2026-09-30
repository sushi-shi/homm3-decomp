"""Keep queue cleanup bound to its native owner, container and allocator."""

import unittest
from unittest.mock import patch

from homm3.compare.canonicalize import DIRECT_SYMBOL_COMPGEN_KINDS
from homm3.retail_labels import source


QUEUE = ("??1?$queue@UTPoint@@V?$list@UTPoint@@"
         "V?$allocator@UTPoint@@@std@@@std@@@std@@QAE@XZ")
LIST = "??1?$list@UTPoint@@V?$allocator@UTPoint@@@std@@@std@@QAE@XZ"


class QueueListDestructorKeyTests(unittest.TestCase):
    def test_queue_and_member_destructors_have_distinct_owners(self):
        self.assertEqual(source._demangle_key(QUEUE), "tpoint@queue_list_dtor")
        self.assertEqual(source._demangle_key(LIST), "tpoint@list_dtor")
        self.assertEqual(source._demangle_key(QUEUE.replace("TPoint", "Other")),
                         "other@queue_list_dtor")

    def test_different_container_allocator_element_and_abi_do_not_join(self):
        for name in (QUEUE.replace("?$list@", "?$deque@"),
                     QUEUE.replace("?$allocator@", "?$OtherAllocator@"),
                     QUEUE.replace("?$list@UTPoint", "?$list@UOther"),
                     QUEUE.replace("?$allocator@UTPoint", "?$allocator@UOther"),
                     QUEUE.replace("QAE@XZ", "QAE@H@Z")):
            with self.subTest(symbol=name):
                self.assertNotEqual(source._demangle_key(name),
                                    "tpoint@queue_list_dtor")

    def test_claim_joins_actual_emitted_queue(self):
        row = dict(rva=0x14c6a0, size=77, channel="src-VA_COMPGEN",
                   name="__h3cg$rmg$queue_list_dtor$TPoint")
        with patch.object(source, "_base_authority_scan", return_value=(
                {"tpoint@queue_list_dtor": [(QUEUE, 77)]}, {})):
            source.join_unit("rmg", [row])
        self.assertEqual(row["joined"], QUEUE)

    def test_equal_size_list_cannot_supply_queue_body(self):
        row = dict(rva=0x14c6a0, size=77, channel="src-VA_COMPGEN",
                   name="__h3cg$rmg$queue_list_dtor$TPoint")
        with patch.object(source, "_base_authority_scan", return_value=(
                {"tpoint@list_dtor": [(LIST, 77)]}, {})):
            source.join_unit("rmg", [row])
        self.assertNotIn("joined", row)

    def test_native_body_is_not_an_anonymous_wrapper(self):
        self.assertIn("QUEUE_LIST_DTOR", source.COMPGEN_KINDS)
        self.assertIn("QUEUE_LIST_DTOR", DIRECT_SYMBOL_COMPGEN_KINDS)
        self.assertNotIn("QUEUE_LIST_DTOR", source.ANONYMOUS_COMPGEN_KINDS)


if __name__ == "__main__":
    unittest.main()
