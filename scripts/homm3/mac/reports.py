"""Atomic, locked publication preserves other workers' unit observations."""
import fcntl
import json
from pathlib import Path
import tempfile


def atomic_text(path: Path, text: str) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    temporary = None
    try:
        with tempfile.NamedTemporaryFile("w", dir=path.parent, delete=False) as stream:
            temporary = Path(stream.name)
            stream.write(text)
        temporary.replace(path)
    finally:
        if temporary is not None:
            temporary.unlink(missing_ok=True)


def publish(path: Path, report: dict, *, units: list[str] | None = None,
            finish=None) -> dict:
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.with_suffix(".lock").open("a") as lock:
        fcntl.flock(lock, fcntl.LOCK_EX)
        if units is not None and path.is_file():
            old = json.loads(path.read_text())
            retained = []
            for row in old.get("pairs", []):
                if row.get("unit") not in units:
                    # Never relabel old evidence with today's tooling hash.
                    row.setdefault("analysis_sha256", old.get("analysis_sha256"))
                    row.setdefault("executable_sha256", old.get("target_sha256"))
                    retained.append(row)
            replaced = {row["retail_va"] for row in report["pairs"]}
            report["pairs"] += [row for row in retained if row["retail_va"] not in replaced]
        report["pairs"].sort(key=lambda row: row["retail_va"])
        if finish:
            finish(report)
        atomic_text(path, json.dumps(report, indent=2) + "\n")
    return report
