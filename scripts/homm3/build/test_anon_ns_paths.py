"""Controls for portable anonymous-namespace names and cache invalidation."""
from pathlib import Path
import os
import tempfile
import unittest
from unittest.mock import patch

from homm3.build import canonicalize_data_symbols as canon, normalize_objs
from homm3.build.normalized_freshness import freshness_problems
from homm3.build.test_equivalent_relocation_normalization import _base, _target
from homm3.build.test_eh_handler_normalization import FixtureSection, _coff, _symbol
from homm3.core import common


CANONICAL = r"C:\Dev\Heroes 3 Exp 2\Game\ForceFeedback.cpp210603558"


def initializer(scope):
    return "??0t_initializer@?%" + scope + "@@QAE@PAX0@Z"


class AnonymousNamespacePathsTest(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        (self.root / "config").mkdir()
        self.table = self.root / "config/retail-anon-ns-paths.tsv"
        self.write_table(CANONICAL)
        self.root_patch = patch.object(common, "HOMM3_DIR", self.root)
        self.root_patch.start()
        self.addCleanup(self.root_patch.stop)

    def write_table(self, scope, unit="forcefeedback"):
        self.table.write_text("unit\tsource_basename\tcanonical_scope\n"
                              + unit + "\tforcefeedback.h\t" + scope + "\n")

    def test_two_worktrees_share_a_canonical_name(self):
        for scope in (r"Z:\checkout-a\include\forcefeedback.h126885183",
                      r"Z:\checkout-b\include\FORCEFEEDBACK.H1168229266"):
            self.assertEqual(canon.normalize_anon_ns_name(initializer(scope), "forcefeedback"),
                             initializer(CANONICAL))
        canonical = initializer(CANONICAL)
        self.assertEqual(canon.normalize_anon_ns_name(canonical, "forcefeedback"), canonical)

    def test_unreviewed_namespace_and_ordinary_symbols_are_unchanged(self):
        for name in (initializer(r"Z:\other.h123"), "?ordinary@@YAXXZ"):
            self.assertEqual(canon.normalize_anon_ns_name(name, "forcefeedback"), name)
        self.table.unlink()
        name = initializer(r"Z:\forcefeedback.h123")
        self.assertEqual(canon.normalize_anon_ns_name(name, "forcefeedback"), name)

    def test_repeated_scopes_in_catchable_type_names_are_all_normalized(self):
        scope = r"Z:\checkout\forcefeedback.h123"
        name = "__CT??_R0?AVt_initialize_failure@t_initializer@?%" + scope
        name += "@@@8" + initializer(scope) + "28"
        self.assertEqual(canon.normalize_anon_ns_name(name, "forcefeedback"),
                         name.replace(scope, CANONICAL))

    def test_embedded_rtti_payload_is_not_rewritten(self):
        name = initializer(r"Z:\checkout\forcefeedback.h123")
        raw = name.encode() + b"\0"
        payload = _coff((FixtureSection(".rdata", raw, ()),),
                        (_symbol("rtti", 0, 1, 0, 2),))
        payload = canon._rewrite_names(canon.CoffObject(payload), {0: name})
        result = canon.canonicalize_coff(payload, unit="forcefeedback")
        after = canon.CoffObject(result.data)
        self.assertEqual(after.symbols[0].name, initializer(CANONICAL))
        self.assertEqual(after.section_bytes(after.sections[0]), raw)

    def test_coff_rewrite_preserves_code_and_relocation_identity(self):
        old = initializer(r"Z:\checkout\forcefeedback.h123")
        payload = canon._rewrite_names(canon.CoffObject(_base()), {0: old})
        before = canon.CoffObject(payload)
        result = canon.canonicalize_coff(payload, unit="forcefeedback")
        after = canon.CoffObject(result.data)
        self.assertEqual(after.symbols[0].name, initializer(CANONICAL))
        self.assertEqual(len(result.rows), 1)
        row = result.rows[0]
        self.assertEqual(row.family, "anonymous-namespace")
        self.assertEqual(row.original_name, old)
        self.assertEqual(row.canonical_name, initializer(CANONICAL))
        self.assertIn("unit=forcefeedback", row.proof)
        self.assertEqual(before.relocations, after.relocations)
        self.assertEqual([before.section_bytes(s) for s in before.sections],
                         [after.section_bytes(s) for s in after.sections])
        self.assertEqual(before.symbols[0].value, after.symbols[0].value)
        self.assertEqual(before.symbols[0].section, after.symbols[0].section)

    def test_same_basename_in_another_unit_is_not_aliased(self):
        name = initializer(r"Z:\other\forcefeedback.h123")
        self.assertEqual(canon.normalize_anon_ns_name(name, "other"), name)
        self.assertEqual(canon.normalize_anon_ns_name(name), name)
        payload = canon._rewrite_names(canon.CoffObject(_base()), {0: name})
        for unit in (None, "other"):
            result = canon.canonicalize_coff(payload, unit=unit)
            self.assertEqual(canon.CoffObject(result.data).symbols[0].name, name)
            self.assertFalse(result.rows)

    def test_sweep_uses_the_same_unit_scoped_normalization(self):
        from homm3.vc6 import tu_state_sweep
        name = initializer(r"Z:\checkout\forcefeedback.h123")
        payload = canon._rewrite_names(canon.CoffObject(_base()), {0: name})
        with patch.object(tu_state_sweep, "_claims", return_value=((), frozenset())):
            result = tu_state_sweep._first_pass("forcefeedback", payload)
            self.assertEqual(canon.CoffObject(result).symbols[0].name,
                             initializer(CANONICAL))
            result = tu_state_sweep._first_pass("other", payload)
            self.assertEqual(canon.CoffObject(result).symbols[0].name, name)

    def test_colliding_namespace_identities_are_rejected(self):
        payload = canon._rewrite_names(canon.CoffObject(_base()), {
            0: initializer(r"Z:\one\forcefeedback.h123"),
            1: initializer(r"Z:\two\forcefeedback.h456"),
        })
        with self.assertRaisesRegex(ValueError, "collision"):
            canon.canonicalize_coff(payload, unit="forcefeedback")

    def test_collision_with_an_existing_canonical_name_is_rejected(self):
        payload = canon._rewrite_names(canon.CoffObject(_base()), {
            0: initializer(r"Z:\one\forcefeedback.h123"),
            1: initializer(CANONICAL),
        })
        with self.assertRaisesRegex(ValueError, "collision"):
            canon.canonicalize_coff(payload, unit="forcefeedback")

    def test_manifest_rejects_malformed_or_duplicate_rows(self):
        valid = self.table.read_text()
        for contents in ("wrong header\n", valid + "short row\n",
                         valid + valid.splitlines()[-1] + "\n",
                         valid.replace("forcefeedback.h", ""),
                         valid.replace(CANONICAL, "?%invalid@scope")):
            with self.subTest(contents=contents):
                self.table.write_text(contents)
                with self.assertRaises(ValueError):
                    canon.normalize_anon_ns_name(
                        initializer(r"Z:\forcefeedback.h123"), "forcefeedback")

    def test_same_size_table_edit_with_preserved_timestamp_is_seen(self):
        old = initializer(r"Z:\forcefeedback.h123")
        self.assertEqual(canon.normalize_anon_ns_name(old, "forcefeedback"),
                         initializer(CANONICAL))
        stat = self.table.stat()
        changed = CANONICAL[:-1] + "9"
        self.write_table(changed)
        os.utime(self.table, ns=(stat.st_atime_ns, stat.st_mtime_ns))
        self.assertEqual(canon.normalize_anon_ns_name(old, "forcefeedback"),
                         initializer(changed))

    def test_table_change_invalidates_both_paired_copies(self):
        self.write_table(CANONICAL, unit="probe")
        objdiff = self.root / "objdiff"
        old = initializer(r"Z:\checkout\forcefeedback.h123")
        for side, filename, payload in (("base", "probe.obj", _base()),
                                        ("target", "probe.c.obj", _target())):
            (objdiff / side).mkdir(parents=True)
            (objdiff / side / filename).write_bytes(
                canon._rewrite_names(canon.CoffObject(payload), {0: old}))
        names = self.root / "symbol_names.csv"
        names.write_text("rva,name,unit,size,kind,provenance\n"
                         "0x1020,owner,probe,0x10,data,dc\n")
        with patch.multiple(normalize_objs, OBJDIFF=objdiff, SYMBOL_NAMES=names,
                            COMPGEN_MANIFEST=self.root / "absent.tsv"):
            self.assertEqual(normalize_objs.normalize_unit("probe")["wrote"], 2)
            self.assertEqual(normalize_objs.normalize_unit("probe")["wrote"], 0)
            outputs = [objdiff / "normalized/base/probe.obj",
                       objdiff / "normalized/target/probe.c.obj"]
            for path in outputs:
                self.assertFalse(freshness_problems(path))
            changed = CANONICAL + "0"
            self.write_table(changed, unit="probe")
            for path in outputs:
                self.assertTrue(freshness_problems(path))
            self.assertEqual(normalize_objs.normalize_unit("probe")["wrote"], 2)
            for path in outputs:
                self.assertFalse(freshness_problems(path))
                self.assertEqual(canon.CoffObject(path.read_bytes()).symbols[0].name,
                                 initializer(changed))


if __name__ == "__main__":
    unittest.main()
