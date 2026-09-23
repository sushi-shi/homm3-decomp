"""Produce an objdiff report. Reading it is a separate, side-effect-free step."""
from pathlib import Path
import subprocess


COMPARISON_POLICY = "objdiff-3.7.3-all-data-addends-v1"


def generate(directory: Path, *, run=subprocess.run) -> Path:
    result = run(['objdiff-cli', 'report', 'generate', '-o', 'report.json',
                  '-c', 'functionRelocDiffs=all'],
                 cwd=directory, capture_output=True, text=True)
    if result.returncode:
        raise RuntimeError('objdiff-cli report generate failed:\n' + result.stdout + result.stderr)
    return directory / 'report.json'
