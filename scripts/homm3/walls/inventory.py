"""The score input adapter used by Gruntz's relocation checks."""
import json
from homm3.verify.scores import report_path


def report_scores():
    path = report_path()
    doc = json.loads(path.read_text())
    return str(path), {
        (unit['name'].split('/')[-1], fn['name']):
        float(fn.get('fuzzy_match_percent') or 0)
        for unit in doc.get('units', []) for fn in unit.get('functions', [])}
