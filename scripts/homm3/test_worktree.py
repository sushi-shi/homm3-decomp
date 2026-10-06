"""`homm3 worktree`: receipt-verified seeding and safe removal."""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import tempfile
import unittest
from unittest.mock import patch

from homm3 import worktree
from homm3.build import normalized_freshness
from homm3.core import root


def _digest(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


class SeedBaseObjectTests(unittest.TestCase):
    def setUp(self):
        self._tmp = tempfile.TemporaryDirectory(prefix="homm3 worktree ")
        self.root = Path(self._tmp.name).resolve()
        self.origin, self.tree = self.root / "origin", self.root / "tree"
        toolchain = self.origin / "build/homm3-toolchain-vc6-sp3"
        toolchain.mkdir(parents=True)
        (toolchain / "CL.EXE").write_bytes(b"compiler")
        for checkout in (self.origin, self.tree):
            (checkout / "src").mkdir(parents=True, exist_ok=True)
            (checkout / "include").mkdir(exist_ok=True)
            (checkout / "include/shared.h").write_text("shared")
            (checkout / "src/same.cpp").write_text("same")
            (checkout / "src/wrapper.py").write_text("wrapper")
        (self.origin / "src/edited.cpp").write_text("old")
        (self.tree / "src/edited.cpp").write_text("new")
        # A seeded tree links the shared toolchain, as `worktree new` does.
        (self.tree / "build").mkdir()
        (self.tree / "build/homm3-toolchain-vc6-sp3").symlink_to(toolchain)
        self.base = self.origin / "build/objdiff/base"
        self.base.mkdir(parents=True)

    def tearDown(self):
        self._tmp.cleanup()

    def _compiled(self, origin: Path, unit: str, source: str, obj: bytes = b"obj") -> None:
        base = origin / "build/objdiff/base"
        base.mkdir(parents=True, exist_ok=True)
        inputs = {}
        for path in (origin / "src" / source, origin / "include/shared.h",
                     self.origin / "build/homm3-toolchain-vc6-sp3/CL.EXE",
                     origin / "src/wrapper.py"):
            inputs[str(path.resolve())] = _digest(path.read_bytes())
        (base / f"{unit}.obj").write_bytes(obj + unit.encode())
        (base / f"{unit}.obj.inputs.json").write_text(json.dumps({
            "schema": 1, "flags": ["/O2"], "inputs": inputs,
            "object_sha256": _digest(obj + unit.encode())}))

    def test_only_byte_identical_inputs_are_adopted_under_new_paths(self):
        self._compiled(self.origin, "same", "same.cpp")
        self._compiled(self.origin, "edited", "edited.cpp")
        seeded = worktree.seed_base_objects(self.tree, [self.origin], ["same", "edited", "absent"])
        self.assertEqual(seeded, {"same": self.origin})
        base = self.tree / "build/objdiff/base"
        self.assertEqual(sorted(p.name for p in base.iterdir()),
                         ["same.obj", "same.obj.inputs.json"])
        receipt = json.loads((base / "same.obj.inputs.json").read_text())
        self.assertIn(str(self.tree / "src/same.cpp"), receipt["inputs"])
        # The linked toolchain resolves to the one shared real directory.
        self.assertIn(str(self.origin / "build/homm3-toolchain-vc6-sp3/CL.EXE"),
                      receipt["inputs"])
        self.assertFalse(any(key.startswith(str(self.origin / "src"))
                             for key in receipt["inputs"]))

    def test_later_checkout_supplies_units_the_first_cannot(self):
        other = self.root / "other"
        for name in ("src", "include"):
            (other / name).mkdir(parents=True)
        (other / "include/shared.h").write_text("shared")
        (other / "src/edited.cpp").write_text("new")
        (other / "src/wrapper.py").write_text("wrapper")
        self._compiled(self.origin, "edited", "edited.cpp")
        self._compiled(other, "edited", "edited.cpp")
        seeded = worktree.seed_base_objects(self.tree, [self.origin, other], ["edited"])
        self.assertEqual(seeded, {"edited": other})

    def test_wrapper_mismatch_rejects_the_origin_and_tampered_objects_are_skipped(self):
        self._compiled(self.origin, "same", "same.cpp")
        (self.tree / "src/wrapper.py").write_text("changed wrapper")
        self.assertEqual(worktree.seed_base_objects(self.tree, [self.origin], ["same"]), {})
        (self.tree / "src/wrapper.py").write_text("wrapper")
        (self.base / "same.obj").write_bytes(b"tampered")
        self.assertEqual(worktree.seed_base_objects(self.tree, [self.origin], ["same"]), {})

    def test_adopted_units_take_their_origins_normalized_copies(self):
        seed = self.root / "seed"
        names = worktree._normalized_names("same")
        for name in names:
            path = self.origin / "build/objdiff/normalized" / name
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_text(name)
        copied = worktree.adopt_normalized(self.tree, {"same": self.origin, "other": seed}, seed)
        self.assertEqual(copied, len(names))
        self.assertIn("base/same.obj.stamp.json", names)
        self.assertIn("target/same.c.symbols.tsv", names)


class RemoveTests(unittest.TestCase):
    def test_remove_refuses_uncommitted_changes_then_removes_clean_tree(self):
        with tempfile.TemporaryDirectory(prefix="homm3 worktree git ") as raw:
            repo = Path(raw).resolve() / "repo"
            repo.mkdir()
            git = ["git", "-c", "user.name=t", "-c", "user.email=t@example.invalid",
                   "-c", "init.defaultBranch=main"]
            subprocess.run([*git, "init", "-q", str(repo)], check=True)
            (repo / "file").write_text("tracked")
            (repo / ".gitignore").write_text("build/\n")
            subprocess.run([*git, "-C", str(repo), "add", "."], check=True)
            subprocess.run([*git, "-C", str(repo), "commit", "-qm", "init"], check=True)
            lane = Path(raw).resolve() / "lane"
            subprocess.run([*git, "-C", str(repo), "worktree", "add", "-q", "-b", "lane",
                            str(lane)], check=True)
            (lane / "build").mkdir()
            (lane / "build/ignored.obj").write_text("generated")
            (lane / "file").write_text("edited")
            args = argparse.Namespace(path=str(lane), delete_branch=True)
            with patch.object(worktree.common, "HOMM3_DIR", repo), \
                    patch.object(worktree.shutil, "which", return_value=None):
                with self.assertRaisesRegex(worktree.WorktreeError, "uncommitted"):
                    worktree.cmd_remove(args)
                self.assertTrue(lane.is_dir())
                with self.assertRaisesRegex(worktree.WorktreeError, "main worktree"):
                    worktree.cmd_remove(argparse.Namespace(path=str(repo), delete_branch=False))
                subprocess.run(["git", "-C", str(lane), "checkout", "--", "file"], check=True)
                self.assertEqual(worktree.cmd_remove(args), 0)
            self.assertFalse(lane.exists())
            branches = subprocess.run(["git", "-C", str(repo), "branch", "--list", "lane"],
                                      capture_output=True, text=True, check=True).stdout
            self.assertEqual(branches, "")


def _checkout(path: Path) -> Path:
    for name in root.MARKER_FILES:
        (path / name).parent.mkdir(parents=True, exist_ok=True)
        (path / name).write_text("")
    tools = path / "scripts/homm3"
    tools.mkdir(parents=True)
    (tools / "__main__.py").write_text("")
    (tools / "normalize_objs.py").write_text("transform")
    return path


class ForeignCodeTests(unittest.TestCase):
    """A seeded worktree is comparable at once only under its own tools."""

    def setUp(self):
        self._tmp = tempfile.TemporaryDirectory(prefix="homm3 code root ")
        base = Path(self._tmp.name).resolve()
        self.main = _checkout(base / "main")
        self.tree = _checkout(base / "main/.claude/worktrees/lane")

    def tearDown(self):
        self._tmp.cleanup()

    def _inputs(self, checkout: Path) -> dict:
        return {"tool:normalize_objs.py": checkout / "scripts/homm3/normalize_objs.py"}

    def test_seeded_stamp_is_fresh_only_under_the_trees_own_tools(self):
        seeded = self.main / "build/objdiff/normalized/base/unit.obj"
        seeded.parent.mkdir(parents=True)
        seeded.write_bytes(b"normalized")
        with patch.object(normalized_freshness, "implementation_inputs",
                          lambda: self._inputs(self.main)):
            normalized_freshness.write_stamp(seeded, {})
        copy = self.tree / "build/objdiff/normalized/base/unit.obj"
        copy.parent.mkdir(parents=True)
        for name in ("unit.obj", "unit.obj.stamp.json"):
            (copy.parent / name).write_bytes((seeded.parent / name).read_bytes())
        for code, fresh in ((self.tree, True), (self.main, False)):
            with patch.object(normalized_freshness, "implementation_inputs",
                              lambda code=code: self._inputs(code)):
                problems = normalized_freshness.freshness_problems(copy)
            self.assertEqual(not problems, fresh, problems)

    def test_main_checkout_code_reexecutes_under_the_worktree(self):
        environ = {"HOMM3_DIR": str(self.main),
                   "PYTHONPATH": f"{self.main / 'scripts'}:/elsewhere"}
        plan = root.foreign_code_reexec(self.main, environ, cwd=self.tree / "src")
        self.assertIsNotNone(plan)
        environment, warning = plan
        self.assertIn("different checkout", warning)
        self.assertEqual(environment["HOMM3_DIR"], str(self.tree))
        self.assertEqual(environment["PYTHONPATH"], f"{self.tree / 'scripts'}:/elsewhere")
        # The re-executed process, an intentional override and the main
        # checkout itself all keep their code.
        self.assertIsNone(root.foreign_code_reexec(self.tree, environment, cwd=self.tree))
        self.assertIsNone(root.foreign_code_reexec(
            self.main, {**environ, root.FORCE_VARIABLE: "1"}, cwd=self.tree))
        self.assertIsNone(root.foreign_code_reexec(self.main, environ, cwd=self.main))
        self.assertIsNone(root.foreign_code_reexec(
            self.main, {**environ, root.REEXEC_VARIABLE: str(self.tree)}, cwd=self.tree))


if __name__ == "__main__":
    unittest.main()
