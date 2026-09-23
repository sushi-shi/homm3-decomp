"""Fresh raw inputs and a separate static-data checkpoint for normal builds."""
from __future__ import annotations

from homm3.build import compiled_freshness
from homm3.core.project import Project
from homm3.sema import data_match


def schedule(root, targets=None):
    """Invalidate only stale stamps; Ninja then recompiles their owning objects.

    Content provenance is broader than Ninja's timestamp dependencies. A normal
    checkpoint repairs every stale unit, including ones outside a focused target.
    Fast builds repair only the explicitly selected manifest units.
    """
    project = Project(root)
    units = project.manifest['unit']
    selected = {u['unit'] for u in units}.intersection(targets or ())
    if targets and selected:
        units = [u for u in units if u['unit'] in selected]
    includes = [project.toolchain/'include', *(p for p in project.includes if p.is_dir())]
    snapshots, stale = {}, []
    for unit in units:
        source = root/unit['source']
        flags = project.manifest['flags'][unit['flags']]
        key = (source.parent, tuple(flags))
        if key not in snapshots:
            snapshots[key] = compiled_freshness.snapshot(root, source, flags, includes, project.toolchain)
        template = snapshots[key]
        files = dict(template['files'])
        files.pop(template['source'])
        files[str(source.resolve())] = compiled_freshness.digest(source)
        expected = dict(template, source=str(source.resolve()), files=files)
        output = root/f'build/objdiff/base/{unit["unit"]}.obj'
        try:
            compiled_freshness.validate(output, expected)
        except ValueError:
            compiled_freshness.stamp_path(output).unlink(missing_ok=True)
            stale.append(unit['unit'])
    if stale:
        print(f'[build] data provenance: scheduling {len(stale)} stale/missing raw object(s)')
    return stale


def run(root, *, require_exact=False, evidence=None):
    """Static pointer/coverage checks complement the ordinary objdiff report."""
    evidence = evidence or data_match.prepare(root, build_vendor=True)
    report = data_match.generate(root, evidence=evidence)
    data_match.export(report, root/'build/data-match')
    summary = report['summary']
    counts = summary['bytes_by_status']
    print(f"[build] DATA: {summary['matched_initialized_bytes']:,} initialized bytes matched; "
          f"{summary['zero_fill_agreement_bytes']:,} zero-fill bytes agree; "
          f"{counts.get('pointer-unresolved', 0):,} unresolved pointer bytes; "
          f"{counts.get('unenrolled', 0):,}/{summary['total_bytes']:,} bytes unenrolled")
    print('[build] DATA: ordinary objdiff project; exhaustive ranges in build/gen/data/retail-data.tsv')
    failures = [f'data analysis unavailable: {issue}' for issue in report['analysis_issues']]
    if require_exact and not data_match.exact(report):
        failures.append('static DATA is not exact; inspect build/data-match/data-match-summary.json')
    return failures
