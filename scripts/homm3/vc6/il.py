"""homm3.vc6.il - the C1XX->C2 IL tap + `homm3 vc6 il-diff` (phase 1).

VC6 compiles in two passes: the front end (C1XX.DLL) writes an intermediate
language the back end (C2.DLL, reader.c) consumes.  The driver seeds the
handoff as ``-il <tmp>`` to both passes (spec record 0x7eec ``^il!>+`` ->
``1P2=*:*``) and the ``^`` makes the switch non-user-typable - but
``/d1il<prefix>`` reaches the front ends verbatim (record 0x7c30 ``d1!+`` ->
``1PM=!*``) and the front end honours the LAST ``-il``.  That is the tap:

  capture   ``/d1il<prefix>`` alone redirects pass-1 output to
            <prefix>{in,gl,sy,ex}; C2 still reads the untouched seed tmp and
            dies with its C1083, which freezes complete pass-1 output on
            disk (a capture run produces no obj BY DESIGN).
  feed      ``/d2il<prefix>`` symmetrically points C2 at a captured set.
            Round-trip bound: obj(C2 over captured IL) equals a plain
            compile byte-for-byte outside the COFF TimeDateStamp
            (418/418 B trivial TU; 24547/24547 B initialize.cpp).
  beware    giving BOTH passes the same prefix in ONE driver invocation
            self-destructs (C2's in-process handling deletes the streams,
            then C1083s re-opening <prefix>in) - mechanism unmodelled, so
            capture and feed stay separate invocations.

``il-diff <srcA> <srcB>`` captures both TUs (copied to a common basename in
equal-length work dirs so the ``-f`` token and any resolved include path
cannot pollute the comparison) and byte-diffs stream by stream, annotated by
the _il framing overlays (symbol names, handle shifts, per-function ex
spans).  rc 0 = IL identical, 1 = IL differs, 2 = error.

``python3 -m homm3.vc6.il killer`` runs the C1 include-set killer experiment
(behavior catalog C1, the flagship): initialize.cpp under the game profile
with an honest town.h vs town.h + one appended unused ``struct probe0_t
{ int a; };`` - the byte-proven 100.0 -> 96.09 perturbation - then feeds
both captured ILs through the same C2 and compares the objs.  Verdict:
FRONT-END if the IL differs (and the obj delta reproduces from IL alone),
C2-STATE if the IL is identical.  Result 2026-08-10: FRONT-END - see
docs/vc6/il-format.md section 5.
"""
from __future__ import annotations

import json as _json
import os
import shutil
import subprocess
import sys
from pathlib import Path

from homm3.core import cc_wrap
from homm3.vc6 import _common, _il, _toolchain

IL_DIR = _common.REPO / "build/vc6/il"
# The game C++ profile (config/units.toml game_o2_ml_gr_windows + /GR-, the
# catalog's profile-under-test; same list reg_model.GAME_FLAGS pins).
DEFAULT_FLAGS = ["/c", "/nologo", "/O2", "/Ob2", "/Oy-", "/Op", "/ML",
                 "/Gr", "/GX", "/GR-", "/D_WINDOWS"]
TS_BYTES = range(4, 8)          # COFF TimeDateStamp mask (docs/vc6/shim.md)
KILLER_SRC = _common.REPO / "src/initialize.cpp"
KILLER_HEADER = _common.REPO / "include/town.h"


# ---------------------------------------------------------------------------
# wine plumbing (mirrors shim/build.py; cc_wrap is not usable here because a
# capture run intentionally produces no obj, which cc_wrap treats as failure)
# ---------------------------------------------------------------------------

def _ensure_wine_env() -> None:
    if shutil.which("wine") is None:
        _common.die("wine not found - run inside `nix develop .#build`.")
    os.environ.setdefault("WINEDEBUG", "fixme-all,err-kerberos")
    if not Path(os.environ.get("WINEPREFIX", "")).is_dir():
        os.environ["WINEPREFIX"] = str(_common.REPO / "build/wineprefix")
    cc_wrap.ensure_wineserver()


def _include_env(src: Path) -> str:
    """msvc include; repo include; the source's own directory (so headers
    that live beside the original source keep resolving after the copy)."""
    w = cc_wrap.winepath_w
    parts = [w(cc_wrap.msvc_dir() / "include")]
    if (_common.REPO / "include").is_dir():
        parts.append(w(_common.REPO / "include"))
    parts.append(w(src.parent))
    return ";".join(parts)


def _wine_cl(args: list[str], cwd: Path, include: str) -> subprocess.CompletedProcess:
    cl = cc_wrap.find_ci(cc_wrap.msvc_dir() / "bin", "cl.exe")
    env = os.environ.copy()
    env["INCLUDE"] = include
    timeout = float(os.environ.get("HOMM3_CL_TIMEOUT", "300"))
    return subprocess.run(["wine", str(cl), *args], cwd=str(cwd), env=env,
                          stdin=subprocess.DEVNULL, capture_output=True,
                          encoding="latin1", errors="replace", timeout=timeout)


def _tail(proc: subprocess.CompletedProcess, n: int = 8) -> str:
    return "\n".join((proc.stdout + proc.stderr).strip().splitlines()[-n:])


def _real_errors(proc: subprocess.CompletedProcess) -> list[str]:
    """Compiler error lines OTHER than C2's expected C1083 on the seed tmp
    ('Cannot open compiler intermediate file') - those mean the FRONT END
    failed and the capture is void."""
    out = []
    for line in (proc.stdout + proc.stderr).splitlines():
        if " error " not in f" {line} " and "error C" not in line:
            continue
        if "C1083" in line and "compiler intermediate file" in line:
            continue
        out.append(line.strip())
    return out


# ---------------------------------------------------------------------------
# capture / feed
# ---------------------------------------------------------------------------

def capture(src: Path, flags: list[str], workdir: Path,
            shadow: dict[str, str] | None = None) -> dict[str, Path]:
    """One front-end run of *src*; returns {stream suffix: path}.

    The TU is copied to <workdir>/tu<suffix> and compiled with cwd=workdir so
    the ``-f`` token (which embeds in gl/ex) is the bare copy name, identical
    for every capture.  *shadow* files (name -> text) are written beside the
    copy; the includer-directory-first quoted-include rule makes them win
    over same-named headers anywhere else - the killer's town.h lever.
    """
    src = src.resolve()
    if not src.is_file():
        _common.die(f"source missing: {src}")
    if workdir.exists():
        shutil.rmtree(workdir)
    workdir.mkdir(parents=True)
    tu = workdir / ("tu" + src.suffix.lower())
    shutil.copyfile(src, tu)
    for name, text in (shadow or {}).items():
        (workdir / name).write_text(text)

    prefix = cc_wrap.winepath_w(workdir) + "\\il"
    proc = _wine_cl([*flags, f"/d1il{prefix}", "/Foil_never.obj", tu.name],
                    workdir, _include_env(src))
    errs = _real_errors(proc)
    if errs:
        _common.die(f"front end failed on {src.name}:\n  " + "\n  ".join(errs[:6]))
    cap = {s: workdir / f"il{s}" for s in _il.STREAM_ORDER
           if (workdir / f"il{s}").is_file()}
    missing = [s for s in ("in", "gl", "sy", "ex") if s not in cap]
    if missing:
        _common.die(f"capture incomplete for {src.name}: no il{'/il'.join(missing)} "
                    f"written - front end did not finish.\n{_tail(proc)}")
    return cap


def feed_c2(tu_dir: Path, flags: list[str], cap: dict[str, Path],
            out_name: str, include: str) -> Path:
    """Compile <tu_dir>/tu.* with C2 reading a COPY of *cap* (C2 may consume
    the streams; the copy keeps the capture pristine).  Returns the obj."""
    feed = IL_DIR / "feed"
    if feed.exists():
        shutil.rmtree(feed)
    feed.mkdir(parents=True)
    for s, p in cap.items():
        shutil.copyfile(p, feed / f"il{s}")
    tu = next(p for p in tu_dir.iterdir() if p.stem == "tu")
    prefix = cc_wrap.winepath_w(feed) + "\\il"
    obj = tu_dir / out_name
    obj.unlink(missing_ok=True)
    proc = _wine_cl([*flags, f"/d2il{prefix}", f"/Fo{out_name}", tu.name],
                    tu_dir, include)
    if not obj.is_file():
        _common.die(f"C2 feed produced no {out_name}:\n{_tail(proc)}")
    return obj


def _masked_obj_diff(a: bytes, b: bytes) -> int:
    n = min(len(a), len(b))
    d = sum(1 for i in range(n) if a[i] != b[i] and i not in TS_BYTES)
    return d + (max(len(a), len(b)) - n)


# ---------------------------------------------------------------------------
# the diff
# ---------------------------------------------------------------------------

def _clusters(diffs: list[int], gap: int = 8) -> list[tuple[int, int]]:
    out: list[list[int]] = []
    for i in diffs:
        if out and i - out[-1][1] <= gap:
            out[-1][1] = i
        else:
            out.append([i, i])
    return [tuple(c) for c in out]


def _byte_report(a: bytes, b: bytes) -> dict:
    if a == b:
        return {"identical": True, "size_a": len(a), "size_b": len(b)}
    n = min(len(a), len(b))
    diffs = [i for i in range(n) if a[i] != b[i]]
    diffs += list(range(n, max(len(a), len(b))))
    return {"identical": False, "size_a": len(a), "size_b": len(b),
            "first": diffs[0], "ndiff": len(diffs),
            "clusters": _clusters(diffs)}


def _annotate_in(a: bytes, b: bytes, names_a: dict, names_b: dict) -> list[str]:
    ha, hb = _il.parse_in(a), _il.parse_in(b)
    if ha is None or hb is None or not (_il.roundtrip_in(a) and _il.roundtrip_in(b)):
        return ["framing: mixed record types (not the all-09 <u16> simple "
                "form; rich-TU grammar open) - byte-level only"]
    notes = [f"framing exact (round-trip OK): {len(ha)} | {len(hb)} function records"]
    delta = [k for k in range(min(len(ha), len(hb))) if ha[k] != hb[k]]
    if delta:
        k = delta[0]
        na = names_a.get(ha[k], "?")
        nb = names_b.get(hb[k], "?")
        notes.append(f"{len(delta)} record(s) differ, first #{k}: "
                     f"handle {ha[k]:#x} ({na}) -> {hb[k]:#x} ({nb})")
        if na == nb != "?":
            notes.append("same symbol NAME at the first differing record - "
                         "a pure handle renumbering, not a different function list")
    if len(ha) != len(hb):
        notes.append(f"function COUNT differs: {len(ha)} -> {len(hb)}")
    return notes


def _annotate_gl(a: bytes, b: bytes) -> tuple[list[str], list[dict], list[dict]]:
    hw_a, hw_b = _il.gl_highwater(a), _il.gl_highwater(b)
    notes = []
    if hw_a is not None and hw_b is not None:
        tag = "" if hw_a == hw_b else f"  ({hw_b - hw_a:+d})"
        notes.append(f"handle high-water {hw_a:#x} | {hw_b:#x}{tag}")
    na = _il.scan_names(a, hw_a, min_len=3)
    nb = _il.scan_names(b, hw_b, min_len=3)
    if [r["name"] for r in na] == [r["name"] for r in nb]:
        shifts = [(ra["name"], rb["handle"] - ra["handle"])
                  for ra, rb in zip(na, nb) if ra["handle"] != rb["handle"]]
        if shifts:
            deltas = sorted({d for _, d in shifts})
            notes.append(f"{len(na)} symbol names IDENTICAL in order; "
                         f"{len(shifts)} handle field(s) shifted "
                         f"({', '.join(f'{d:+d}' for d in deltas)}), "
                         f"first: {shifts[0][0][:48]}")
        else:
            notes.append(f"{len(na)} symbol names and handles identical "
                         "(diff lies outside named records)")
    else:
        sa = {r["name"] for r in na}
        sb = {r["name"] for r in nb}
        add = sorted(sb - sa)[:4]
        rem = sorted(sa - sb)[:4]
        notes.append(f"symbol name sets differ: {len(na)} | {len(nb)} records"
                     + (f"; added {add}" if add else "")
                     + (f"; removed {rem}" if rem else ""))
    return notes, na, nb


def _annotate_ex(rep: dict, gl_a: bytes, names_a: list[dict], ex_a: bytes) -> list[str]:
    spans = _il.fn_ex_offsets(gl_a, names_a, len(ex_a))
    if not spans:
        return ["no per-function spans recovered from gl - opaque"]
    notes = [f"{len(spans)} function span(s) recovered from gl ex-offsets"]
    if not rep.get("identical"):
        import bisect
        offs = [o for o, _ in spans]
        hit_fns = []
        for lo, _hi in rep["clusters"]:
            k = bisect.bisect_right(offs, lo) - 1
            fn = spans[k][1] if k >= 0 else "(header/hole)"
            if fn not in hit_fns:
                hit_fns.append(fn)
        notes.append(f"divergence touches {len(hit_fns)} function(s), "
                     f"first: {hit_fns[0][:56]}")
    return notes


def diff_captures(cap_a: dict[str, Path], cap_b: dict[str, Path],
                  fn: str | None = None) -> dict:
    streams = [s for s in _il.STREAM_ORDER if s in cap_a or s in cap_b]
    data = {s: (cap_a[s].read_bytes() if s in cap_a else b"",
                cap_b[s].read_bytes() if s in cap_b else b"") for s in streams}
    report = {"streams": {}, "identical": True, "notes": {}}
    for s in streams:
        rep = _byte_report(*data[s])
        report["streams"][s] = rep
        report["identical"] &= rep["identical"]

    gl_notes, names_a, names_b = _annotate_gl(*data["gl"]) if "gl" in data \
        else ([], [], [])
    map_a = {r["handle"]: r["name"] for r in names_a}
    map_b = {r["handle"]: r["name"] for r in names_b}
    if "in" in data:
        report["notes"]["in"] = _annotate_in(*data["in"], map_a, map_b)
    if "gl" in data:
        report["notes"]["gl"] = gl_notes
    if "sy" in data:
        ba = _il.sy_block_spans(data["sy"][0])
        bb = _il.sy_block_spans(data["sy"][1])
        report["notes"]["sy"] = [f"{len(ba)} | {len(bb)} local-symbol block(s)"]
    if "ex" in data and "gl" in data:
        report["notes"]["ex"] = _annotate_ex(report["streams"]["ex"],
                                             data["gl"][0], names_a,
                                             data["ex"][0])
    if fn:
        report["fn"] = _fn_report(fn, names_a, names_b, data, report)
    return report


def _fn_report(fn: str, names_a: list[dict], names_b: list[dict],
               data: dict, report: dict) -> dict:
    def find(names):
        exact = [r for r in names if r["name"] == fn]
        subs = [r for r in names if fn in r["name"]]
        return exact[0] if exact else (subs[0] if len(subs) == 1 else
                                       subs[0] if subs else None)
    ra, rb = find(names_a), find(names_b)
    out = {"query": fn}
    if ra is None or rb is None:
        out["status"] = "not found in gl symbol scan"
        return out
    out["name"] = ra["name"]
    out["handle_a"], out["handle_b"] = ra["handle"], rb["handle"]
    if "ex" in data:
        spans = _il.fn_ex_offsets(data["gl"][0], names_a, len(data["ex"][0]))
        offs = [o for o, _ in spans]
        mine = [(o, n) for o, n in spans if n == ra["name"]]
        if mine:
            import bisect
            lo = mine[0][0]
            k = bisect.bisect_right(offs, lo)
            hi = offs[k] if k < len(offs) else len(data["ex"][0])
            rep = report["streams"]["ex"]
            touched = (not rep["identical"]
                       and any(c[0] < hi and c[1] >= lo for c in rep["clusters"]))
            out["ex_span"] = [lo, hi]
            out["ex_span_differs"] = bool(touched)
    return out


# ---------------------------------------------------------------------------
# reporting
# ---------------------------------------------------------------------------

def _verdict_line(report: dict) -> str:
    if report["identical"]:
        total = sum(r["size_a"] for r in report["streams"].values())
        return (f"IL IDENTICAL ({' '.join(report['streams'])}: "
                f"{total} bytes)")
    parts, total = [], 0
    for s, rep in report["streams"].items():
        if not rep["identical"]:
            parts.append(f"{s}@{rep['first']:#x}({rep['ndiff']}B)")
            total += rep["ndiff"]
    return (f"IL DIFFERS: first divergence {parts[0]}; "
            f"{len(parts)}/{len(report['streams'])} streams differ, "
            f"{total} byte(s): " + " ".join(parts))


def _print_report(report: dict, label_a: str, label_b: str,
                  flags: list[str]) -> None:
    print(_verdict_line(report))
    print(f"[il-diff] A: {label_a}")
    print(f"[il-diff] B: {label_b}")
    print(f"[flags]   {' '.join(flags)}")
    for s, rep in report["streams"].items():
        if rep["identical"]:
            head = f"identical ({rep['size_a']} B)"
        else:
            head = (f"sizes {rep['size_a']}|{rep['size_b']}, "
                    f"{rep['ndiff']} byte(s) in {len(rep['clusters'])} "
                    f"cluster(s), first @{rep['first']:#x}")
        print(f"[{s:2}]      {head}")
        for note in report["notes"].get(s, []):
            print(f"          {note}")
    if "fn" in report:
        f = report["fn"]
        if "handle_a" in f:
            line = (f"[--fn]    {f['name'][:56]}: handle {f['handle_a']:#x} | "
                    f"{f['handle_b']:#x}")
            if "ex_span" in f:
                line += (f"; ex span {f['ex_span'][0]:#x}..{f['ex_span'][1]:#x} "
                         + ("DIFFERS" if f["ex_span_differs"] else "identical"))
            print(line)
        else:
            print(f"[--fn]    {f['query']}: {f['status']}")


# ---------------------------------------------------------------------------
# entries
# ---------------------------------------------------------------------------

def _gate_subjects() -> None:
    for name in ("CL.EXE", "C1XX.DLL", "C2.DLL"):
        _toolchain.resolve(name)


def _flags_of(args) -> list[str]:
    flags = args.flags.split() if getattr(args, "flags", "") else list(DEFAULT_FLAGS)
    if "/c" not in flags and "-c" not in flags:
        flags.insert(0, "/c")
    return flags


def run_diff(args) -> int:
    """`homm3 vc6 il-diff <srcA> <srcB> [--flags ...] [--fn NAME] [--json]`"""
    _ensure_wine_env()
    _gate_subjects()
    flags = _flags_of(args)
    src_a, src_b = Path(args.srcA).resolve(), Path(args.srcB).resolve()
    if src_a.suffix.lower() != src_b.suffix.lower():
        _common.die("srcA and srcB must share an extension "
                    "(different front ends otherwise)")
    cap_a = capture(src_a, flags, IL_DIR / "diff" / "a")
    cap_b = capture(src_b, flags, IL_DIR / "diff" / "b")
    report = diff_captures(cap_a, cap_b, fn=getattr(args, "fn", None))
    report["srcA"], report["srcB"] = str(src_a), str(src_b)
    report["flags"] = flags
    report["captures"] = {"a": {s: str(p) for s, p in cap_a.items()},
                          "b": {s: str(p) for s, p in cap_b.items()}}
    if getattr(args, "json", False):
        print(_json.dumps(report, indent=2))
    else:
        _print_report(report, str(src_a), str(src_b), flags)
    return 0 if report["identical"] else 1


def run_killer(structs: int = 1, use_json: bool = False) -> int:
    """The C1 include-set killer experiment (catalog C1 flagship).

    A: initialize.cpp with a shadow town.h == include/town.h verbatim.
    B: same, town.h + *structs* appended unused dummy struct(s).
    Equal-length work dirs a/ b/ keep any resolved path the same length;
    town.h resolves from the shadow dir in both, so no path can pollute.
    Then both captured ILs are fed through the SAME C2 and the objs compared
    (masked COFF timestamp) - causation, not just correlation.
    """
    _ensure_wine_env()
    _gate_subjects()
    flags = list(DEFAULT_FLAGS)
    header = KILLER_HEADER.read_text()
    dummy = "".join(f"\nstruct probe{i}_t {{ int a; }};\n" for i in range(structs))
    base = IL_DIR / "killer"
    cap_a = capture(KILLER_SRC, flags, base / "a", shadow={"town.h": header})
    cap_b = capture(KILLER_SRC, flags, base / "b",
                    shadow={"town.h": header + dummy})
    report = diff_captures(cap_a, cap_b)
    report["experiment"] = {
        "src": str(KILLER_SRC), "header": str(KILLER_HEADER),
        "perturbation": f"{structs} unused dummy struct(s) appended "
                        "to a shadow town.h (catalog C1: 100.0 -> 96.09)"}

    # causation: same C2, the two captured ILs, masked obj compare
    include = _include_env(KILLER_SRC)
    tu_dir = base / "a"
    proc = _wine_cl([*flags, "/Foplain.obj", "tu.cpp"], tu_dir, include)
    plain = tu_dir / "plain.obj"
    if not plain.is_file():
        _common.die(f"plain reference compile failed:\n{_tail(proc)}")
    obj_a = feed_c2(tu_dir, flags, cap_a, "from_a.obj", include)
    obj_b = feed_c2(tu_dir, flags, cap_b, "from_b.obj", include)
    fidelity = _masked_obj_diff(plain.read_bytes(), obj_a.read_bytes())
    causation = _masked_obj_diff(obj_a.read_bytes(), obj_b.read_bytes())
    report["experiment"]["fidelity_masked_diff"] = fidelity
    report["experiment"]["obj_masked_diff"] = causation

    if not report["identical"]:
        verdict = ("FRONT-END - the include-state changes the IL C1XX hands "
                   "to C2" + ("; feeding the two ILs through the same C2 "
                              f"reproduces an obj delta ({causation} byte(s)) "
                              "from the IL alone" if causation else
                              "; but C2 emits the SAME obj from both - the "
                              "differing IL bytes are inert (partial verdict)"))
    else:
        verdict = ("C2-STATE - the IL is IDENTICAL under the include-state "
                   "change; the perturbation lives in back-end state keyed "
                   "on something outside the IL")
    report["experiment"]["verdict"] = verdict

    if use_json:
        print(_json.dumps(report, indent=2))
        return 0
    _print_report(report, "honest town.h (shadow copy)",
                  f"town.h + {structs} unused dummy struct(s)", flags)
    print(f"[oracle]  obj(C2 over IL A) vs plain compile: "
          f"{fidelity} byte(s) outside the COFF timestamp "
          f"({'capture is faithful' if fidelity == 0 else 'CAPTURE NOT FAITHFUL'})")
    print(f"[oracle]  obj(C2 over IL A) vs obj(C2 over IL B): "
          f"{causation} byte(s) outside the COFF timestamp")
    print(f"KILLER VERDICT: {verdict}")
    return 0 if fidelity == 0 else 1


def run_handles(args) -> int:
    """`handles <src> [--against SRC_B] [--shadow NAME=FILE[:b]] [--predict]`

    The handle-order model's CLI (homm3.vc6._handles; the model doc is
    docs/vc6/handle-order.md):

      one source   capture + print the symbol-handle CREATION table; with
                   --predict also run the pure predictor over the TU's
                   file-scope declarations and print predicted vs measured.
      --against    capture both and report the handle DELTA: which handles
                   shifted, by how much, from which creation point on, and
                   which declaration in the source/shadow diff explains it
                   (the include-set advisor - catalog C1 made actionable).
      --shadow     NAME=FILE places FILE as a shadow header NAME beside the
                   capture copy (the killer's town.h lever); suffix ':b'
                   applies it to the --against side only.
    """
    from homm3.vc6 import _handles
    _ensure_wine_env()
    _gate_subjects()
    flags = _flags_of(args)
    src = Path(args.src).resolve()
    shadow_a, shadow_b = {}, {}
    for spec in args.shadow or []:
        name, _, path = spec.partition("=")
        side_b = path.endswith(":b")
        text = Path(path[:-2] if side_b else path).read_text()
        (shadow_b if side_b else shadow_a)[name] = text
        if not side_b:
            shadow_b.setdefault(name, text)
    if args.against:
        rep = _handles.handle_delta(
            src, Path(args.against).resolve(), flags,
            shadow_a=shadow_a or None, shadow_b=shadow_b or None)
        if args.json:
            print(_json.dumps({k: v for k, v in rep.items()}, indent=2,
                              default=str))
        else:
            print(f"[handles] A: {rep['src_a']}\n[handles] B: {rep['src_b']}")
            for line in _handles.format_delta_report(rep):
                print(line)
        return 0 if rep.get("hw_delta") == 0 else 1

    cap = _handles.capture_handles(src, flags, IL_DIR / "handles" / src.stem,
                                   shadow=shadow_a or None)
    rows = _handles.creation_table(cap)
    if args.json:
        print(_json.dumps({"src": str(src), "highwater": cap.highwater,
                           "creation": rows}, indent=2))
        return 0
    print(f"[handles] {src.name}: high-water {cap.highwater:#x} "
          f"({len(rows)} named symbol(s) scanned; heuristic overlay - "
          "unnamed handles are invisible)")
    for r in rows:
        print(f"  {r['handle']:#6x}  {r['stream']}  {r['name'][:72]}")
    if args.predict:
        pred = _handles.predict_handles(src.read_text())
        tag = " (APPROXIMATE - rich TU)" if pred["approximate"] else ""
        print(f"[predict] file-scope walk from base {pred['base']:#x}{tag}:")
        for row in pred["rows"]:
            d = row["decl"]
            end = f"..{row['end']:#x}" if row["end"] is not None else " ?"
            print(f"  {row['start']:#6x}{end}  {d.kind:<10} "
                  f"cost={d.cost if d.cost is not None else '?':<3} {d.text[:56]}")
        print(f"[predict] high-water >= {pred['highwater_at_least']:#x} "
              f"(measured {cap.highwater:#x})")
    return 0


def main(argv: list[str] | None = None) -> int:
    import argparse
    ap = argparse.ArgumentParser(
        prog="python3 -m homm3.vc6.il", description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = ap.add_subparsers(dest="cmd", required=True)
    pk = sub.add_parser("killer", help="the C1 include-set killer experiment")
    pk.add_argument("--structs", type=int, default=1,
                    help="dummy structs appended to the shadow town.h (default 1)")
    pk.add_argument("--json", action="store_true")
    ph = sub.add_parser("handles",
                        help="symbol-handle order: capture table, predictor, "
                             "A/B delta advisor (docs/vc6/handle-order.md)")
    ph.add_argument("src")
    ph.add_argument("--against", metavar="SRC_B",
                    help="second source: report the handle delta A -> B")
    ph.add_argument("--shadow", action="append", metavar="NAME=FILE[:b]",
                    help="shadow header beside the capture copy; ':b' = "
                         "--against side only (repeatable)")
    ph.add_argument("--flags", default="",
                    help="cl flags (default: the game profile)")
    ph.add_argument("--predict", action="store_true",
                    help="also run the pure predictor and compare")
    ph.add_argument("--json", action="store_true")
    args = ap.parse_args(argv)
    if args.cmd == "handles":
        rc = run_handles(args)
    else:
        rc = run_killer(structs=args.structs, use_json=args.json)
    import shlex
    _common.log_invocation(rc, shlex.join(
        ["python3", "-m", "homm3.vc6.il", *(argv or sys.argv[1:])]))
    return rc


if __name__ == "__main__":
    sys.exit(main())
