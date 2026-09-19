"""The focused compile → normalize → report stage used by interactive views."""
from dataclasses import dataclass
from pathlib import Path
import subprocess
from homm3.core import common


NINJA_FILE = common.HOMM3_DIR / "build.ninja"
REFRESH_LOCK = common.HOMM3_DIR / "build/.sema-refresh.lock"

@dataclass(frozen=True)
class RefreshResult:
    unit: str
    compiled: bool = False
    normalized: bool = False
    report: Path | None = None
    seconds: float = 0.0


def refresh_unit(unit: str, *, run=subprocess.run) -> RefreshResult:
    """Compile and normalize one unit, producing a report only after changes.

    Serialize writers in this build directory. Failed compilation or reporting
    raises with tool diagnostics; callers decide how to present the failure.
    """
    root = common.HOMM3_DIR
    ninja_file = NINJA_FILE
    refresh_lock = REFRESH_LOCK
    import fcntl
    import time
    if not ninja_file.is_file() or not unit:
        return RefreshResult(unit)
    target = f"build/objdiff/base/{unit}.obj"
    started = time.time()
    refresh_lock.parent.mkdir(parents=True, exist_ok=True)
    with refresh_lock.open("w") as lock:
        fcntl.flock(lock, fcntl.LOCK_EX)
        res = run(["ninja", "-f", str(ninja_file), target],
                  cwd=root, capture_output=True, text=True)
        if res.returncode != 0:
            tail = "\n".join((res.stdout + res.stderr).strip().splitlines()[-25:])
            from homm3.core.usage import classify_error
            category = classify_error(tail)
            if category.startswith("environment."):
                advice = "check Wine/toolchain availability and execution permissions"
            elif category == "compile.cpp":
                advice = "fix the C++ compiler errors below"
            else:
                advice = "check the build command and diagnostics below"
            raise RuntimeError(f"{unit} candidate refresh failed [{category}] - {advice} "
                "(--no-build only compares an existing, fresh normalized object):"
                f"\n{tail}")
        compiled = "no work to do" not in res.stdout
        from homm3.build import normalize_objs
        counts = normalize_objs.normalize_unit(unit)
        if not compiled and not counts["wrote"]:
            return RefreshResult(unit)
        from homm3.build.report import generate
        report = generate(root / 'build/objdiff', run=run)
    return RefreshResult(unit, compiled, bool(counts["wrote"]), report,
                         time.time() - started)
