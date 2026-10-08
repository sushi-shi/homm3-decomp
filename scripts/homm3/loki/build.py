"""Compile, delink and score the Loki h3maped image (separate from `homm3 build`).

    build/h3maped-loki/obj/<unit>.o              GCC 2.95.2 object of the unit's source
    build/h3maped-loki/objdiff/base/<unit>.o     its canonical comparison form
    build/h3maped-loki/objdiff/target/<unit>.o   the retail object, delinked and canonical
    build/h3maped-loki/objdiff/report.json       objdiff-cli report

The game build, its ledger and README are untouched.
"""
from __future__ import annotations

from concurrent.futures import ThreadPoolExecutor
from dataclasses import dataclass
import json
from pathlib import Path
import subprocess
import tomllib

from homm3.core import common
from homm3.loki import census, datacmp, delink, ledger, objwriter, toolchain
from homm3.loki.image import IMAGE, LokiImage

ROOT = common.HOMM3_DIR
OUT = ROOT / "build" / IMAGE
OBJDIFF = OUT / "objdiff"
VENDOR_INCLUDES = ("bink-0.5a/include", "ifc-2.0.3/include", "miles-5.0e/include",
                   "smacker-3.2h/include", "zlib-1.1.3")


@dataclass(frozen=True)
class Unit:
    name: str
    obj: int
    source: Path
    flags: tuple[str, ...]
    driver: str = "g++"


def manifest() -> dict:
    with (ROOT / "config/loki/units.toml").open("rb") as stream:
        return tomllib.load(stream)


def units(selected: list[str] | None = None) -> list[Unit]:
    spec = manifest()
    out = []
    for row in spec["unit"]:
        profile = spec["profiles"][row.get("profile", "editor")]
        flags = (*profile["flags"], *(f"-D{d}" for d in profile.get("defines", ())),
                 *(("-include", str(ROOT / profile["prefix"])) if "prefix" in profile else ()))
        out.append(Unit(row["name"], row["object"], ROOT / row["source"], flags,
                        profile.get("driver", "g++")))
    if selected:
        known = {u.name for u in out}
        missing = [name for name in selected if name not in known]
        if missing:
            raise ValueError(f"unknown Loki units: {', '.join(missing)}")
        out = [u for u in out if u.name in selected]
    return out


def link_only_units() -> list[tuple[Unit, int]]:
    """Objects only the link needs (no census object): each with the census object it follows."""
    spec = manifest()
    out = []
    for row in spec.get("link_only", ()):
        profile = spec["profiles"][row.get("profile", "editor")]
        flags = (*profile["flags"], *(f"-D{d}" for d in profile.get("defines", ())),
                 *(("-include", str(ROOT / profile["prefix"])) if "prefix" in profile else ()))
        out.append((Unit(row["name"], -1, ROOT / row["source"], flags, profile.get("driver", "g++")),
                    row["after"]))
    return out


def include_flags() -> list[str]:
    flags = [f"-I{ROOT / 'src'}", f"-I{ROOT / 'include'}"]
    flags += [f"-I{ROOT / 'vendor' / path}" for path in VENDOR_INCLUDES]
    # GTK+/GLib 1.2.8 (`gtk-config --cflags`): glibconfig.h lives beside the library.
    flags += [f"-I{toolchain.GTK / 'usr/include'}", f"-I{toolchain.GTK / 'usr/lib/glib/include'}"]
    return flags


def compile_unit(unit: Unit) -> tuple[Path, str | None]:
    out = OUT / "obj" / f"{unit.name}.o"
    out.parent.mkdir(parents=True, exist_ok=True)
    # Loki compiled each file from its own directory: every __FILE__ (assert
    # text, TRuntimeError sites) is a bare name such as "GzBuf.cpp".
    command = toolchain.driver_command("-c", *unit.flags, *include_flags(), unit.source.name, "-o", str(out),
                                       driver=unit.driver)
    environment = toolchain.environment()
    seed = census.anonymous_table().get(unit.source.name)
    if seed is not None:
        # The retail anonymous namespace suffix: anonseed.so (preloaded into
        # cc1plus) hands append_random_chars this sum as the time of day.
        environment.update(HOMM3_LOKI_TIMEOFDAY=f"{seed}.0", HOMM3_LOKI_PID="0")
    completed = subprocess.run(command, env=environment, capture_output=True, text=True,
                               cwd=unit.source.parent)
    (OUT / "obj" / f"{unit.name}.log").write_text(completed.stdout + completed.stderr)
    if completed.returncode:
        out.unlink(missing_ok=True)
        lines = [line for line in completed.stderr.splitlines() if "warning:" not in line]
        return out, (lines[-1] if lines else f"exit {completed.returncode}")
    return out, None


def objdiff_config(names: list[str], built: set[str]) -> None:
    config = {
        "$schema": "https://raw.githubusercontent.com/encounter/objdiff/main/config.schema.json",
        "build_base": False, "build_target": False,
        # objdiff's report merges sections by name prefix unless told not
        # to; merged .gnu.linkonce.t.* bodies pair same-sized strangers
        # (`_._12length_error` took `_._Q213TGzInflateBuf10TDataError`).
        "options": {"functionRelocDiffs": "all", "combineTextSections": False,
                    "combineDataSections": False},
        "units": [{"name": name, "target_path": f"./target/{name}.o",
                   **({"base_path": f"./base/{name}.o"} if name in built else {})}
                  for name in names],
    }
    OBJDIFF.mkdir(parents=True, exist_ok=True)
    (OBJDIFF / "objdiff.json").write_text(json.dumps(config, indent=2) + "\n")


def report() -> dict:
    completed = subprocess.run(["objdiff-cli", "report", "generate", "-o", "report.json"],
                               cwd=OBJDIFF, capture_output=True, text=True)
    if completed.returncode:
        raise RuntimeError("objdiff-cli report generate failed:\n" + completed.stdout + completed.stderr)
    return json.loads((OBJDIFF / "report.json").read_text())


def run(selected: list[str] | None = None, jobs: int = 3, verbose: bool = False,
        bank: bool = False) -> int:
    toolchain.stage()
    chosen = units(selected)
    image = LokiImage()
    with ThreadPoolExecutor(max_workers=max(1, jobs)) as pool:
        compiled = list(pool.map(compile_unit, chosen))
    ready = []
    built: dict[str, tuple[Unit, list, dict[str, str]]] = {}
    for unit, (obj, error) in zip(chosen, compiled):
        delink_path = OBJDIFF / "target" / f"{unit.name}.o"
        target = delink.target_sections(unit.obj, image)
        base_path = OBJDIFF / "base" / f"{unit.name}.o"
        if error:
            objwriter.write(delink_path, target)
            base_path.unlink(missing_ok=True)
            print(f"[loki] {unit.name}: compile failed: {error}")
            continue
        base = delink.base_sections(obj.read_bytes())
        delink.pair_locals(base, target)
        unpaired = delink.pair_statics(base, target)
        delink.pair_data(base, target)
        if unpaired and verbose:
            print(f"[loki] {unit.name}: unpaired file-static functions: {', '.join(unpaired)}")
        objwriter.write(delink_path, target)
        objwriter.write(base_path, base)
        ready.append(unit.name)
        built[unit.name] = (unit, target, ledger.fingerprints(base))
    objdiff_config([u.name for u in chosen], set(ready))
    report_data = report()
    total_exact = total = 0
    for row in report_data.get("units", []):
        measures = row.get("measures", {})
        functions = row.get("functions", [])
        exact = sum(1 for f in functions if f.get("fuzzy_match_percent", 0) >= 100)
        total_exact += exact
        total += len(functions)
        print(f"[loki] {row['name']:14} {measures.get('fuzzy_match_percent', 0):6.2f}%  "
              f"{exact}/{len(functions)} functions exact")
        if verbose:
            for function in functions:
                name = function.get("metadata", {}).get("demangled_name") or function["name"]
                print(f"         {function.get('fuzzy_match_percent', 0):6.2f}  {name}")
    print(f"[loki] {total_exact}/{total} retail functions exact across {len(chosen)} units")
    data = compare_data(set(built), image, verbose)
    if bank:
        rows = ledger.load()
        for report_unit in report_data.get("units", []):
            if report_unit["name"] not in built:
                continue
            unit, target, prints = built[report_unit["name"]]
            scores = {f["name"]: f.get("fuzzy_match_percent", 0.0) for f in report_unit.get("functions", [])}
            ledger.bank(rows, unit.name, unit.obj, target, scores, prints)
        for result in data:
            ledger.bank_data(rows, result.unit, result.obj, result.matched, result.total, result.fingerprint)
        ledger.save(rows)
        changed = ledger.write_readme(rows)
        print(f"[loki] banked {len(built)} units into {ledger.LEDGER.relative_to(ROOT)}"
              + ("; README Loki block refreshed" if changed else ""))
    if selected is None:
        return link_gate(jobs, len(ready) == len(chosen))
    return 0


def link_gate(jobs: int, compiled: bool) -> int:
    """A full build links the image and runs the link gate (homm3.loki.linkdiff.check) whenever
    the link media are staged."""
    if not toolchain.link_staged():
        print("[loki] link gate not run: link media not staged (`homm3 loki toolchain --libs DIR`)")
        return 0
    if not compiled:
        print("[loki] link gate FAIL: not every unit compiled")
        return 1
    for unit, _ in link_only_units():
        _, error = compile_unit(unit)
        if error:
            print(f"[loki] link gate FAIL: {unit.name}: {error}")
            return 1
    from homm3.loki import link
    return link.main(jobs, compile_units=False)


def compare_data(selected: set[str], image: LokiImage, verbose: bool = False) -> list[datacmp.UnitData]:
    """Data bytes of the selected units against their image slices. Every
    unit's last compiled object takes part in the layout (linkonce
    ownership, slice ends), built in this run or not."""
    objects = {u.name: (u.obj, OUT / "obj" / f"{u.name}.o") for u in units()}
    results = datacmp.run(objects, selected, image)
    for result in results:
        if result.matched < result.total or verbose:
            print(f"[loki] {result.unit:14} data {result.matched}/{result.total} bytes"
                  + (f"  ({'; '.join(result.notes)})" if result.notes and verbose else ""))
        if verbose:
            for item in result.differing():
                print(f"         {item.matched:6}/{item.total:<6} {item.name}")
    matched = sum(r.matched for r in results)
    total = sum(r.total for r in results)
    print(f"[loki] {matched}/{total} data bytes match across {len(results)} units")
    return results
