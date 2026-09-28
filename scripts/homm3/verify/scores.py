"""Read the report produced by the normal HoMM3 comparison pipeline."""
from homm3.core.paths import BUILD


def report_path():
    path = BUILD / 'objdiff/report.json'
    if not path.is_file():
        raise SystemExit('no report.json - run `homm3 compare` first')
    return path
