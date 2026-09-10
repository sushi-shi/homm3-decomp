"""Source-line geometry from DC records, without a candidate-side comparison.

An interval between recorded lines is observable; its original text is not.
Keep every attribution (including equal addresses) and avoid interpreting an
observed file envelope as the function's opening/closing braces.
"""
from __future__ import annotations

from collections import defaultdict
from bisect import bisect_left


CAUTION = (
    "Line gaps may contain empty lines, comments, declarations, braces, or "
    "optimized/release-elided statements. They support source hypotheses, "
    "not recovered text. Observed spans are not total function line counts; "
    "inline attributions can extend them and trailing lines are unknown."
)


def source_key(path):
    return path.replace("/", "\\").lower()


class LineIndex:
    """Index a module once for corpus browsing; preserve equal-address order."""

    def __init__(self, records):
        self.rows = sorted(records, key=lambda row: row[2])
        self.addresses = [row[2] for row in self.rows]

    def window(self, start, end):
        return self.rows[bisect_left(self.addresses, start):bisect_left(self.addresses, end)]


def recover(records, *, source, boundary_line, boundary_reliable, bodyless):
    """Describe records already restricted to the function's byte extent.

    Stable address order retains repeated rows and source switches. File-local
    gaps use distinct lexical line numbers instead of machine instruction order.
    Foreign-file and earlier same-file attributions remain visible separately.
    """
    ordered = sorted(records, key=lambda row: row[2])
    groups = defaultdict(list)
    for filename, line, address in ordered:
        groups[source_key(filename)].append((filename, line, address))
    files = []
    for key, rows in groups.items():
        lines = sorted({line for _filename, line, _address in rows if line > 0})
        gaps = [{"first_line": left + 1, "last_line": right - 1,
                 "line_count": right - left - 1}
                for left, right in zip(lines, lines[1:]) if right > left + 1]
        files.append({
            "source": rows[0][0], "owning_source": key == source_key(source),
            "first_recorded_line": lines[0] if lines else None,
            "last_recorded_line": lines[-1] if lines else None,
            "observed_span_lines": lines[-1] - lines[0] + 1 if lines else None,
            "recorded_line_count": len(lines), "line_row_count": len(rows),
            "unrecorded_lines_within_span": sum(gap["line_count"] for gap in gaps),
            "gaps": gaps,
        })
    return {
        "caution": CAUTION,
        "boundary_line": boundary_line,
        "boundary_reliable": boundary_reliable,
        "bodyless": bodyless,
        "function_line_count": None,
        "blank_line_count": None,
        "trailing_line_count": None,
        "files": files,
        "rows": [{"source": filename, "line": line, "address": address,
                  "attribution": (
                      "foreign-source" if source_key(filename) != source_key(source)
                      else "earlier-owning-source" if boundary_line is not None and line < boundary_line
                      else "unreliable-boundary" if line == boundary_line and not boundary_reliable
                      else "owning-source")}
                 for filename, line, address in ordered],
    }


def render(layout, *, rows=False):
    """Compact numbered source outline; absent lines are explicit intervals."""
    output = ["Source-line layout (observations, not recovered source text):"]
    output.append("  Function total / empty / trailing lines: unknown")
    boundary = layout["boundary_line"]
    output.append(f"  Procedure boundary: {boundary if boundary is not None else 'unknown'}"
                  + (" (unreliable)" if not layout["boundary_reliable"] else ""))
    if layout["bodyless"]:
        output.append("  Minimal SH4 body; line attributions do not establish a body span.")
    if not layout["files"]:
        output.append("  No source-line rows recorded inside the procedure extent.")
    for item in layout["files"]:
        output.append(f"  {item['source']} ({'owning' if item['owning_source'] else 'foreign'} source)")
        output.append(f"    observed {item['first_recorded_line']}..{item['last_recorded_line']}: "
                      f"{item['observed_span_lines']} lines; {item['recorded_line_count']} recorded; "
                      f"{item['unrecorded_lines_within_span']} unrecorded; {item['line_row_count']} rows")
        if rows:
            by_line = defaultdict(list)
            for row in layout["rows"]:
                if source_key(row["source"]) == source_key(item["source"]):
                    by_line[row["line"]].append(row)
            for line in sorted(by_line):
                for row in by_line[line]:
                    output.append(f"    {line:>6} | dc:0x{row['address']:08x} {row['attribution']}")
                gap = next((gap for gap in item["gaps"] if gap["first_line"] == line + 1), None)
                if gap:
                    output.append(f"    {gap['first_line']}..{gap['last_line']} | "
                                  f"{gap['line_count']} unrecorded line(s), contents unknown")
        else:
            for gap in item["gaps"]:
                output.append(f"    gap {gap['first_line']}..{gap['last_line']}: "
                              f"{gap['line_count']} unrecorded line(s), contents unknown")
    if rows and layout["rows"]:
        output.append("  Attribution sequence (address order, repeated rows retained):")
        output.extend(f"    dc:0x{row['address']:08x} {row['source']}:{row['line']}"
                      for row in layout["rows"])
    output.append("  " + CAUTION)
    return "\n".join(output)
