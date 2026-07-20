#!/usr/bin/env python3
"""cc_wrap.py - the `wine cl` compiler wrapper ninja's `cl` rule invokes.

ninja drives the base/recompile side natively on Linux; the compiler is VC6 SP3
CL.EXE under Wine. This wrapper translates paths, sets the pinned MSVC include
root, runs `wine cl <flags> /Fo<obj.w> <src.w>`, and exits
non-zero if the .obj wasn't produced (wine noise can mask cl's real exit).

Toolchain + prefix come from `nix develop .#build` (MSVC_DIR, WINEPREFIX).
Usage (emitted into build.ninja by configure.py):
    cc_wrap.py --out <obj> --src <src> -- <cl flags...>
"""
import argparse, os, re, shutil, signal, subprocess, sys, tempfile
from pathlib import Path
SCRIPT_DIR = Path(__file__).resolve().parent
HOMM3_DIR = next((p for p in SCRIPT_DIR.parents if (p / "flake.nix").exists()), SCRIPT_DIR)

_INC_RE = re.compile(r'^[ \t]*#[ \t]*include[ \t]*[<"]([^>"]+)[>"]', re.M)

def scan_header_deps(src, inc_root):
    """Recover header dependencies independently of compiler diagnostic output: recursively resolve
    every `#include` against the repo include root (+ each file's own dir for "quoted"
    includes) and return the repo headers reached. System headers (<string.h> etc.) don't
    resolve under inc_root and are skipped — they never change. Over-approximates (ignores
    #if), which is SAFE for a depfile: worst case an extra rebuild, never a stale obj."""
    src = Path(src).resolve(); inc_root = Path(inc_root)
    seen = set(); stack = [src]
    while stack:
        f = stack.pop()
        if f in seen:
            continue
        seen.add(f)
        try:
            text = f.read_text(errors="replace")
        except OSError:
            continue
        for inc in _INC_RE.findall(text):
            for cand in (f.parent / inc, inc_root / inc):
                if cand.is_file():
                    stack.append(cand.resolve()); break
    return sorted(str(p) for p in seen if p != src)

def die(m): print(f"[cc_wrap] ERROR: {m}", file=sys.stderr); sys.exit(1)
def find_ci(d, name):
    return next((p for p in d.iterdir() if p.name.lower() == name.lower()), None) if d.is_dir() else None
def msvc_dir():
    # A valid explicit override wins. Otherwise use the directory emitted by
    # create-toolchain-release.py, with build/toolchain retained as a supported
    # manually unpacked location.
    env = os.environ.get("MSVC_DIR")
    if env and find_ci(Path(env) / "bin", "cl.exe"):
        return Path(env)
    toolchain = os.environ.get("HOMM3_TOOLCHAIN")
    candidates = []
    if toolchain:
        candidates.append(Path(toolchain) / "msvc")
    candidates.extend([
        HOMM3_DIR / "build/homm3-toolchain-vc6-sp3/msvc",
        HOMM3_DIR / "build/toolchain/msvc",
    ])
    return next((path for path in candidates
                 if find_ci(path / "bin", "cl.exe")), candidates[0])
def winepath_w(p):
    return subprocess.check_output(["winepath", "-w", str(p)], text=True, stderr=subprocess.DEVNULL).strip()
def ensure_wineserver():
    ws = shutil.which("wineserver")
    if ws: subprocess.run([ws, "-p"], check=False, stdin=subprocess.DEVNULL,
                          stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)

def _run_cl(cmd, out):
    timeout = float(os.environ.get("HOMM3_CL_TIMEOUT", "300"))
    with tempfile.TemporaryFile() as logf:
        proc = subprocess.Popen(cmd, cwd=str(out.parent), stdin=subprocess.DEVNULL,
                                stdout=logf, stderr=subprocess.STDOUT, start_new_session=True)
        try:
            proc.wait(timeout=timeout); rc = proc.returncode
        except subprocess.TimeoutExpired:
            try: os.killpg(os.getpgid(proc.pid), signal.SIGKILL)
            except (ProcessLookupError, PermissionError): pass
            proc.wait(); rc = 0 if out.exists() else 1
        logf.seek(0); return logf.read().decode("latin1", "replace"), rc

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--out", required=True); ap.add_argument("--src", required=True)
    ap.add_argument("flags", nargs=argparse.REMAINDER)
    a = ap.parse_args()
    flags = a.flags[1:] if a.flags and a.flags[0] == "--" else a.flags
    msvc = msvc_dir(); cl = find_ci(msvc / "bin", "cl.exe")
    if not cl: die(f"CL.EXE not under {msvc}/bin - run inside `nix develop .#build`.")
    if shutil.which("wine") is None: die("wine not found - run inside `nix develop .#build`.")
    src = Path(a.src).resolve(); out = Path(a.out).resolve()
    if not src.exists(): die(f"source missing: {src}")
    out.parent.mkdir(parents=True, exist_ok=True)
    if out.exists(): out.unlink()
    os.environ.setdefault("WINEDEBUG", "fixme-all,err-kerberos")
    if not Path(os.environ.get("WINEPREFIX", "")).is_dir():   # same anti-stale anchor
        os.environ["WINEPREFIX"] = str(HOMM3_DIR / "build/wineprefix")
    ensure_wineserver()
    # Do not expose vendor SDK directories implicitly. The current zlib sources
    # resolve their quoted headers from their own source directory.
    incs = [msvc / "include"]
    if (HOMM3_DIR / "include").is_dir(): incs.append(HOMM3_DIR / "include")
    os.environ["INCLUDE"] = ";".join(winepath_w(p) for p in incs)
    cmd = ["wine", str(cl), *flags, f"/Fo{winepath_w(out)}", winepath_w(src)]
    output, rc = _run_cl(cmd, out)
    if not out.exists():
        sys.stderr.write(f"[cc_wrap] FAILED {src.name} -> {out}\n" + "\n".join(output.strip().splitlines()[-15:]) + "\n")
        sys.exit(rc or 1)
    # Emit a conservative depfile so Ninja recompiles on local-header edits.
    deps = scan_header_deps(src, HOMM3_DIR / "include")
    dep_list = " ".join(d.replace(" ", "\\ ") for d in deps)
    Path(str(out) + ".d").write_text(f"{a.out}: {dep_list}\n")
    sys.exit(0)

if __name__ == "__main__": main()
