#!/usr/bin/env python3
"""homm3.build.build - the `homm3 build` command.

    configure -> ninja (VC6 base objs) -> delink (including normalization)
    -> objdiff report -> projected MAX summary -> [full build] checkpoint-
    ledger refresh + MAX loss report (OBSERVATIONAL) + candidate link +
    banked-rows check (FATAL when a previously banked RVA left the baseline
    entirely) + source evidence gates + cleanliness board (FATAL when a
    ratcheted source metric rises above its committed floor - C-style casts
    are banned at 0) + README score block.

Full builds refresh retail targets before comparison and checkpointing.
`homm3 build` is Windows-only: Classic Mac full-TU objects, pair scores and
their checkpoint belong to the separate `homm3 mac build [TU ...]`. `--data`
adds the complete byte accounting (data coverage), which otherwise keeps
its last README line.
`--fast <TU>` keeps the existing targets and stops after the report. It
normalizes and fingerprints only the selected units, so their projected MAX
movements equal the full projection; other units' MAX is held. Run a full
build first to establish those targets.

Images (docs/tooling/images.md): a full game build then builds every other
pinned image, each in its own process (`homm3 --image KEY build`):
configure -> ninja -> placements check -> delink -> report -> ledger ->
banked-rows and VA-claim gates -> README block. A pinned image whose
executable (staged copy or `$HOMM3_<KEY>_EXE`) or SP3 MFC overlay is missing
fails the build before anything compiles; `--skip-image KEY` is the only
opt-out, and the summary names every skipped image. `--fast` and
`--image KEY build` build only the selected image, so the game's inner loop
never waits for an editor.
"""
from __future__ import annotations

import subprocess
import sys
import time

from homm3.build.normalized_freshness import ValidationContext
from homm3.core import common, inputs
from homm3.core.images import path as _image_path

ROOT = common.HOMM3_DIR


def _run(*command: str) -> int:
    return subprocess.run(list(command), cwd=ROOT).returncode


def _link() -> int:
    return _run(sys.executable, "-m", "homm3.build.link", "--out",
                _image_path("build/exe/HEROES3.candidate.EXE"))


def _link_diff() -> list[str]:
    """The link-diff ceiling gate (homm3.verify.link_diff) over the game's
    candidate; other images have no reconstructed link line yet."""
    import os
    from homm3.core.images import DEFAULT_IMAGE, IMAGE_ENV
    if os.environ.get(IMAGE_ENV, DEFAULT_IMAGE) != DEFAULT_IMAGE:
        return []
    from homm3.verify import link_diff
    try:
        report = link_diff.measure()
    except (OSError, ValueError) as exc:
        print(f"[build] link-diff: unavailable: {exc}", file=sys.stderr)
        return ["link-diff (unavailable)"]
    print("[link-diff] " + ", ".join(f"{name} {report.counts[name]}"
                                     for name in link_diff.GATED + link_diff.TRACKED))
    ceiling = link_diff.read_ceiling()
    for line in link_diff.tracked_rises(report, ceiling):
        print(f"[link-diff] tracked region rose (not gated): {line}")
    findings = link_diff.gate_findings(report, ceiling)
    for line in findings:
        print(f"[build] link-diff: {line}", file=sys.stderr)
    return [f"link-diff ({len(findings)} finding(s))"] if findings else []


def _selected_units(ninja_args: list[str]) -> set[str]:
    """Manifest units named as Ninja targets; options and other targets select none."""
    from homm3 import manifest as units_manifest
    manifest_units = {unit["unit"] for unit in
                      units_manifest.load(ROOT / _image_path("config/units.toml"))["unit"]}
    return {arg for arg in ninja_args if arg in manifest_units}


def main(argv=None) -> int:
    argv = list(argv or [])
    from homm3.core import worktree_lock
    # One mutating homm3 command per worktree: a second build, delink or
    # status update waits here instead of interleaving writes to build/.
    with worktree_lock.hold(" ".join(["homm3 build", *argv])):
        return _main(argv)


def _jobs(ninja_args: list[str]) -> list[str]:
    """The Ninja job options of a build, passed on to each image's build."""
    out = []
    for k, arg in enumerate(ninja_args):
        if arg == "-j" and k + 1 < len(ninja_args):
            out += [arg, ninja_args[k + 1]]
        elif arg.startswith("-j") or arg.startswith("-l"):
            out.append(arg)
    return out


def _image_problems(skipped: set[str]) -> list[str]:
    """Why a full build cannot build each pinned image it was not told to skip:
    its executable has neither a staged copy nor an environment override, or
    the SP3 MFC overlay is neither staged nor named by $HOMM3_MFC_SP3."""
    import os
    from homm3.core import images
    from homm3.core.project import Project
    from homm3.init import mfc_sp3
    problems = []
    project = Project(ROOT)
    for key in images.images(ROOT)[1:]:
        if key in skipped:
            continue
        executable = project.executable(images.input_key(key))
        if not os.environ.get(executable.env_var) and not executable.destination.is_file():
            problems.append(f"image {key}: {executable.name} not staged; set "
                            f"${executable.env_var} or run `homm3 --image {key} init "
                            f"--exe /path/to/{executable.destination.name}`")
        if not mfc_sp3.staged() and not os.environ.get("HOMM3_MFC_SP3"):
            problems.append(f"image {key}: SP3 MFC overlay not staged; set $HOMM3_MFC_SP3 "
                            f"or run `homm3 --image {key} init --mfc-sp3 DIR`")
    return problems


def _skipped_images(argv: list[str]) -> tuple[set[str], list[str]]:
    """`--skip-image KEY` (repeatable) and the remaining arguments; an
    unknown or game key is an error, never a silent no-op."""
    from homm3.core import images
    known = images.images(ROOT)[1:]
    skipped, rest, k = set(), [], 0
    while k < len(argv):
        arg = argv[k]
        if arg == "--skip-image" or arg.startswith("--skip-image="):
            if "=" in arg:
                key = arg.partition("=")[2]
            elif k + 1 < len(argv):
                key = argv[k + 1]
                k += 1
            else:
                raise ValueError("--skip-image needs an image key")
            if key not in known:
                raise ValueError(f"--skip-image {key}: not a pinned editor image "
                                 f"(config/project.toml: {', '.join(known) or 'none'})")
            skipped.add(key)
        else:
            rest.append(arg)
        k += 1
    return skipped, rest


def _build_images(ninja_args: list[str], skipped: set[str]) -> list[str]:
    """Build every other pinned image in its own process; the failed ones.
    The preflight (_image_problems) already refused a missing input."""
    import os
    from homm3.core import images
    from homm3.init import mfc_sp3
    failures = []
    for key in images.images(ROOT)[1:]:
        if key in skipped:
            print(f"[build] image {key}: SKIPPED (--skip-image {key})")
            continue
        if not mfc_sp3.staged():
            print(f"[build] SP3 MFC overlay verified and staged: "
                  f"{mfc_sp3.stage(os.environ['HOMM3_MFC_SP3'])}")
        print(f"[build] image {key}: building", flush=True)
        if _run(sys.executable, "-m", "homm3", "--image", key, "build",
                *(["--", *_jobs(ninja_args)] if _jobs(ninja_args) else [])):
            failures.append(f"image {key}")
    return failures


def _image_main(fast: bool, ninja_args: list[str]) -> int:
    """`homm3 --image KEY build`: one non-game image, its ledger and block."""
    from homm3.build import configure, normalize_objs
    from homm3.core import paths
    from homm3.match import status
    key = paths.image_key()
    if fast and not any((ROOT / _image_path("build/objdiff/target")).glob("*.obj")):
        print(f"[build] {key}: retail targets missing; run `homm3 --image {key} build` "
              "before `--fast`", file=sys.stderr)
        return 1
    try:
        inputs.stage_executable(inputs.RETAIL)
    except inputs.InputError as exc:
        print(f"[build] {exc}", file=sys.stderr)
        return 1
    selected = _selected_units(ninja_args) if fast else set()
    configure.configure()
    if _run("ninja", *configure.ninja_selection(), *ninja_args):
        return 1
    failures = []
    context = None
    if fast:
        normalize_objs.normalize_all(selected or None)
    else:
        from homm3.census import placements
        if placements.main(["--check"]):
            # the delink below reads the committed table: after `--write`,
            # build the image again so its labels carry the new names
            failures.append("placements (`homm3 --image "
                            f"{key} placements --write` after a shared-source change, "
                            f"then `homm3 --image {key} build` again)")
        from homm3.build import delink
        delink.run()
    report = status.refresh_report(context)
    fingerprint_pair = status.source_hash_pair(only_units=selected or None)
    print(f"[build] {key}: {status.overall_line(report, fingerprint_pair=fingerprint_pair)}")
    if fast:
        status.fast_max_movements(report, selected or None, fingerprint_pair)
        return 0
    history_patch = status.baseline_history()
    status.cmd_check(report, fingerprint_pair=fingerprint_pair)
    status.cmd_update(report, fingerprint_pair=fingerprint_pair, history_patch=history_patch)
    from homm3.match import banked_rows, verify_va_claims
    for gate in (banked_rows, verify_va_claims):
        try:
            fatal = (gate.run_gate(history_patch=history_patch) if gate is banked_rows
                     else gate.run_gate())
        except (inputs.InputError, OSError) as exc:
            fatal = [f"{gate.__name__}: evidence unavailable: {exc}"]
        if fatal:
            failures.append(f"{gate.__name__.rsplit('.', 1)[-1]} ({len(fatal)} finding(s))")
            for line in fatal:
                print(f"[build] {key}: {line}", file=sys.stderr)
    try:
        status.write_readme()
    except Exception as exc:  # the score block must never fail a build
        print(f"[build] {key}: README block skipped: {exc}")
    if failures:
        print(f"[build] {key}: FAILED gates: " + "; ".join(failures), file=sys.stderr)
        return 1
    print(f"[build] {key}: all gates passed")
    return 0


def _main(argv: list[str]) -> int:
    fast = "--fast" in argv
    data = "--data" in argv
    try:
        skipped, argv = _skipped_images(argv)
    except ValueError as exc:
        print(f"[build] {exc}", file=sys.stderr)
        return 1
    ninja_args = [a for a in argv if a not in ("--fast", "--data")]
    started = time.monotonic()
    from homm3.core import paths
    if not paths.is_game():
        if skipped:
            print("[build] --skip-image applies to the full game build only", file=sys.stderr)
            return 1
        return _image_main(fast, ninja_args)
    if not fast:
        # A full build certifies every pinned image; one that cannot build
        # fails here, before the game compiles, never silently later.
        problems = _image_problems(skipped)
        if problems:
            for line in problems:
                print(f"[build] {line}", file=sys.stderr)
            print("[build] FAILED: pinned image inputs missing (stage them, or opt out "
                  "explicitly with `--skip-image KEY`)", file=sys.stderr)
            return 1

    def phase(name: str) -> None:
        print(f"[build] {name} ({time.monotonic() - started:.0f}s elapsed)", flush=True)

    from homm3.build import configure, normalize_objs
    from homm3.match import status

    if fast and not any((ROOT / _image_path("build/objdiff/target")).glob("*.c.obj")):
        print("[build] retail targets missing; run `homm3 build` before `--fast`",
              file=sys.stderr)
        return 1

    if not fast:
        try:
            for executable in (inputs.RETAIL, inputs.DREAMCAST):
                inputs.stage_executable(executable)
        except inputs.InputError as exc:
            print(f"[build] {exc}", file=sys.stderr)
            return 1

    selected = _selected_units(ninja_args) if fast else set()
    configure.configure()
    if _run("ninja", *configure.ninja_selection(), *ninja_args):
        return 1
    context = None
    if fast:
        # Ninja rebuilt only the selected units; normalize just those. Any
        # other stale comparison (a tool or manifest change) falls back to
        # the complete pass, exactly as before.
        normalize_objs.normalize_all(selected or None)
        if selected:
            context = ValidationContext()
            if status.comparison_problems(context):
                normalize_objs.normalize_all()
                context = None
        configure.configure()
    else:
        print("[build] refreshing retail targets")
        from homm3.build import delink
        delink.run()
        phase("compiled and delinked")

    report = status.refresh_report(context)
    # A fast build fingerprints only its selected units: their projection is
    # the full one, while unselected functions keep MAX (unknown hash).
    fingerprint_pair = status.source_hash_pair(only_units=selected or None)
    print(f"[build] {status.overall_line(report, fingerprint_pair=fingerprint_pair)}")
    print(f"[build] CUR diagnostic report: {status.REPORT.relative_to(ROOT)}")

    if fast:
        status.fast_max_movements(report, selected or None, fingerprint_pair)
        print("[build] fast: delink + checkpoint ledger + gates + README skipped - "
              "bank measured scores and regenerate README before committing")
        return 0

    # A byte score is a checkpoint, not an admissibility invariant. Coherent
    # restoration of a Dreamcast-proven source shape may lower several local
    # scores before the surrounding class/TU reaches the retail lowering.
    # Preserve each implementation's MAX and the all-time HIST, but never make
    # a score regression fatal. Only a function whose own source hash changed
    # and whose new implementation MAX fell below the preceding MAX is worth
    # reporting; unrelated CUR dips leave MAX held and stay silent.
    # Check BEFORE updating the ledger so a changed function is compared with
    # its preceding MAX/source hash. The update then resets MAX for a proven
    # source edit while preserving HIST.
    history_patch = status.baseline_history()
    status.cmd_check(report, fingerprint_pair=fingerprint_pair)
    status.cmd_update(report, fingerprint_pair=fingerprint_pair, history_patch=history_patch)

    # Run every independent evidence/source gate, even after one fails.
    # Report unavailable evidence as fatal; dependent checks cannot certify it.
    # These gates, not a local objdiff maximum, decide admissibility.
    # Per-TU comparisons cannot detect duplicate definitions or unresolved
    # references in the complete game. Keep independent evidence gates
    # running after a link failure so the checkpoint remains diagnosable.
    phase("ledger checkpointed")
    # Each entry names one failed gate; the summary at the end lists them all.
    failures = []
    if _link() != 0:
        failures.append("candidate link")
    else:
        failures += _link_diff()
    phase("candidate linked")

    # banked_rows runs alongside cmd_check, not inside it: the ratchet
    # compares the rows that ARE in the baseline, this one asks whether a
    # row that used to be there still is. Clean ratchet + lost row is
    # exactly how army::can_shoot left the ledger green (2026-08-15).
    from homm3.match import banked_rows, single_view, verify_va_claims, source_ownership, source_inventory
    origins = None
    for gate in (banked_rows, verify_va_claims, single_view, source_ownership, source_inventory):
        try:
            if gate is source_ownership:
                origins = source_ownership.read_dc(include_declarations=True)
                fatal = source_ownership.run_gate(origins=origins)
            elif gate is source_inventory:
                fatal = source_inventory.run_gate(origins=origins)
            elif gate is banked_rows:
                fatal = banked_rows.run_gate(history_patch=history_patch)
            else:
                fatal = gate.run_gate()
        except (inputs.InputError, OSError) as exc:
            fatal = [f'{gate.__name__}: evidence unavailable: {exc}']
        if fatal:
            failures.append(f"{gate.__name__.rsplit('.', 1)[-1]} ({len(fatal)} finding(s))")
            for line in fatal:
                print(f"[build] {line}", file=sys.stderr)

    # Roll the cleanliness floors down only on an otherwise-green build:
    # a down-only bless recorded off a failing tree would bake in state
    # nobody reviewed. Check always, write only when clean.
    phase("evidence gates checked")
    from homm3.cleanliness import board
    if origins is None:
        # Missing evidence must not look like an empty, clean DC inventory.
        violations = ['cleanliness: cannot check without Dreamcast origins; floors unchanged']
    else:
        violations = board.check_and_roll(write=not failures, dc_origins=origins)
    if violations:
        ratchets = sum(not line.startswith(" ") for line in violations)
        failures.append(f"cleanliness board ({ratchets} violation(s))")
        for line in violations:
            print(f"[build] {line}", file=sys.stderr)
    phase("cleanliness board checked")

    # Scores describe the completed compile/delink, including when an
    # independent source gate fails. Keep README consistent with the ledger.
    # Complete byte accounting takes minutes; it is opt-in (`--data`, or
    # `homm3 verify data-coverage`). Without it README keeps its last line.
    data_accounting = None
    if data:
        try:
            from homm3.verify.byte_accounting import report as account_bytes
            data_accounting = account_bytes()
            totals = data_accounting['totals']['file']
            print(f"[data-coverage] {totals.get('missing', 0):,} unclaimed file bytes; "
                  f"{totals.get('overlap', 0):,} overlapping bytes; "
                  "worklist in build/gen/data_coverage.json")
            from homm3.verify import generated_code
            generated = data_accounting['generated_code']
            # The compiler-generated verdicts follow the game objects and are
            # banked like the score ledger; the runtime table is a retail fact
            # that only a reviewed `homm3 verify generated-code --write` moves.
            generated_code.write_baseline(generated['baseline'])
            if generated_code.stale(generated['runtime'], None):
                print("[generated-code] config/retail/runtime-functions.tsv disagrees with "
                      "`homm3 verify library-code`; review and run "
                      "`homm3 verify generated-code --write`", file=sys.stderr)
                failures.append("runtime function verdicts")
        except (ValueError, OSError) as exc:
            print(f"[data-coverage] unavailable: {exc}")
            failures.append("byte accounting")
        phase("byte accounting refreshed")
    try:
        status.write_readme(data_accounting=data_accounting)
    except Exception as exc:  # the score block must never fail a build
        print(f"[build] README block skipped: {exc}")

    failures += _build_images(ninja_args, skipped)
    phase("finished")
    print("[build] Mac pairs are not part of `homm3 build`; run `homm3 mac build` "
          "for the Classic Mac preservation checkpoint")
    if not data:
        print("[build] byte accounting skipped; run `homm3 build --data` "
              "to refresh data coverage")
    note = (" (images NOT built: " + ", ".join(sorted(skipped)) + ", --skip-image)"
            if skipped else "")
    if failures:
        print("[build] FAILED gates (details above): " + "; ".join(failures) + note,
              file=sys.stderr)
        return 1
    print("[build] all gates passed" + note)
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
