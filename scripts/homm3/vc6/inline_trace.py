"""Read live C2 inline budgets, gated by an identical captured-IL replay.

C1 can change anonymous-namespace identifiers and BSS order between runs.
Both C2 invocations therefore consume the SAME four captured streams. The
complete objects must agree outside their timestamps, and the selected
function's bytes must reproduce the object the caller is diagnosing.
"""
from __future__ import annotations

import contextlib
import hashlib
import json
from pathlib import Path
import re
import shutil
import sys

from homm3.build.canonicalize_data_symbols import CoffObject, FUNCTION_TYPE
from homm3.core import cc_wrap
from homm3.vc6 import _common, _toolchain, il
from homm3.vc6.shim import build

_ROOT = re.compile(r"^# inline ROOT (\d+) sym=([0-9a-f]+) cb=(-?\d+) "
                   r"handle=([0-9a-f]+) flags=([0-9a-f]+) name=(.+)$")
_SITE = re.compile(r"^# inline SITE (\d+) depth=(\d+) budget=(-?\d+) "
                   r"remaining=(\d+) sym=([0-9a-f]+) cb=(-?\d+) "
                   r"handle=([0-9a-f]+) flags=([0-9a-f]+) name=(.+)$")


def parse_trace(text: str, symbol: str) -> dict:
    caller = None
    sites = []
    for line in text.splitlines():
        root = _ROOT.fullmatch(line)
        if root:
            if caller is not None or root[6] != symbol:
                raise ValueError("trace does not identify exactly one selected function")
            cb = int(root[3])
            caller = dict(id=int(root[1]), symbol=root[6], cb=cb,
                          initial_budget=min(35000, max(1000, 2 * cb)),
                          handle=int(root[4], 16), flags=int(root[5], 16))
            continue
        site = _SITE.fullmatch(line)
        if site:
            if caller is None or int(site[1]) != caller["id"]:
                raise ValueError("inline site has no matching caller")
            depth, budget, remaining, cb = map(int, (site[2], site[3], site[4], site[6]))
            if depth < 1 or remaining < 1:
                raise ValueError("invalid inline depth or remaining-site count")
            sites.append(dict(depth=depth, budget=budget, remaining=remaining,
                              cb=cb, symbol=site[9], handle=int(site[7], 16),
                              flags=int(site[8], 16),
                              budget_allows=cb <= 40 or budget >= cb))
        elif line.startswith("# inline"):
            raise ValueError(f"unreadable inline trace record: {line}")
    if caller is None:
        raise ValueError(f"C2 did not trace {symbol}")
    return dict(caller=caller, sites=sites)


def verify_identity(reference: bytes, traced: bytes) -> dict:
    if len(reference) < 20 or len(traced) < 20:
        raise ValueError("inline trace oracle received an incomplete COFF object")
    differences = build._masked_diff(reference, traced)
    if differences:
        raise ValueError(f"inline trace changed the captured-IL object: "
                         f"{len(differences)} byte differences outside its timestamp")
    masked = bytearray(reference)
    masked[4:8] = b"\0" * 4
    return dict(object_bytes=len(reference), timestamp_mask=[4, 8],
                masked_sha256=hashlib.sha256(masked).hexdigest())


def function_bytes(data: bytes, symbol: str) -> bytes:
    coff = CoffObject(data)
    matches = [s for s in coff.symbols.values()
               if s.name == symbol and s.section > 0 and s.typ == FUNCTION_TYPE]
    if len(matches) != 1:
        raise ValueError(f"expected one emitted function {symbol}, found {len(matches)}")
    sym = matches[0]
    section = coff.sections[sym.section - 1]
    end = min((s.value for s in coff.symbols.values()
               if s.section == sym.section and s.typ == FUNCTION_TYPE
               and s.value > sym.value), default=section.raw_size)
    if not section.raw_offset or not sym.value < end <= section.raw_size:
        raise ValueError(f"unreadable function extent for {symbol}")
    return data[section.raw_offset + sym.value:section.raw_offset + end]


def capture(source: Path, flags: list[str], symbol: str, expected_object: Path,
            *, workdir: Path | None = None) -> dict:
    """Trace current source with its exact profile; refuse non-identical output."""
    source = source.resolve()
    expected_object = expected_object.resolve()
    if not source.is_file() or not expected_object.is_file():
        raise ValueError("inline trace requires an existing source and matching object")
    if workdir is None:
        tag = hashlib.sha256(symbol.encode()).hexdigest()[:12]
        workdir = _common.REPO / "build/vc6/inline-trace" / source.stem / tag
    workdir = workdir.resolve()
    workdir.mkdir(parents=True, exist_ok=True)
    for subject in ("CL.EXE", "C1.DLL", "C1XX.DLL", "C2.DLL"):
        _toolchain.resolve(subject)
    build._ensure_wine_env()
    # Always install our clean shim: an interrupted negative-control run must
    # never silently become the next trace's compiler.
    with contextlib.redirect_stdout(sys.stderr):
        build.build_overlay()
        build.compile_shim()
    real = cc_wrap.msvc_dir()
    include = ";".join(cc_wrap.winepath_w(p) for p in
                       [real / "include", _common.REPO / "include",
                        _common.REPO / cc_wrap.ZLIB_INC] if p.is_dir())
    w = cc_wrap.winepath_w
    cwd = expected_object.parent
    capdir, feeddir = workdir / "capture", workdir / "feed"
    capdir.mkdir(exist_ok=True)
    feeddir.mkdir(exist_ok=True)
    streams = ("in", "gl", "sy", "ex")
    for ext in streams:
        (capdir / f"il{ext}").unlink(missing_ok=True)
    proc = build._wine(cc_wrap.find_ci(real / "bin", "cl.exe"),
                       [*flags, "/d1il" + w(capdir / "il"),
                        "/Fo" + w(workdir / "never.obj"), w(source)],
                       cwd, {"INCLUDE": include})
    if il._real_errors(proc) or not all((capdir / f"il{s}").is_file() for s in streams):
        raise ValueError("inline trace front-end capture failed:\n" + build._tail(proc))
    log = workdir / "trace.log"
    log.unlink(missing_ok=True)
    reference, traced = workdir / "reference.obj", workdir / "traced.obj"
    for compiler, obj, extra in (
        (real, reference, {}),
        (build.OVERLAY_MSVC, traced,
         {"HOMM3_VC6_INLINE_TRACE": "1", "HOMM3_VC6_TRACE_FN": symbol,
          "HOMM3_VC6_SHIM_LOG": w(log)}),
    ):
        for ext in streams:
            shutil.copyfile(capdir / f"il{ext}", feeddir / f"il{ext}")
        obj.unlink(missing_ok=True)
        proc = build._wine(cc_wrap.find_ci(compiler / "bin", "cl.exe"),
                           [*flags, "/d2il" + w(feeddir / "il"),
                            "/Fo" + w(obj), w(source)], cwd,
                           {"INCLUDE": include, **extra})
        if proc.returncode or not obj.is_file():
            raise ValueError("inline trace C2 replay failed:\n" + build._tail(proc))
    reference_data, traced_data = reference.read_bytes(), traced.read_bytes()
    oracle = verify_identity(reference_data, traced_data)
    body = function_bytes(reference_data, symbol)
    if body != function_bytes(expected_object.read_bytes(), symbol):
        raise ValueError("inline trace source does not reproduce the selected build object; "
                         "refresh its source/profile before using the trace")
    oracle["matching_function_bytes"] = len(body)
    oracle["matching_function_sha256"] = hashlib.sha256(body).hexdigest()
    if not log.is_file():
        raise ValueError("C2 wrote no inline trace log")
    report = parse_trace(log.read_text(encoding="latin1"), symbol)
    report.update(oracle=oracle, source=str(source), flags=flags,
                  directory=str(workdir))
    (workdir / "trace.json").write_text(json.dumps(report, indent=2) + "\n")
    return report
