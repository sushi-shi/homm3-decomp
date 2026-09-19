#!/usr/bin/env python3
"""Two-state nonnull line painter pointer/reference interface hypothesis.

Refresh immediately invokes the painter through its proxy. Its six callers
also dereference that painter before reaching refresh, so no valid null path
is removed. Retail ECX is an address under either source declaration. Change
one canonical signature, its body and all callers together; retain the point
reference, proxy factory and neighbour helpers. Full seven-consumer checks.
"""
import argparse
import hashlib
import json
from pathlib import Path
from homm3.core.common import HOMM3_DIR
from homm3.vc6.source_families import load_manifest
from experiments._support import generator


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output',type=Path)
    args=parser.parse_args()
    source=(HOMM3_DIR/'src/rmg_terrain.cpp').read_text()
    header=(HOMM3_DIR/'include/rmg.h').read_text()
    extract=generator('generate-rmg-position-family.py').definition
    original=extract(source,'refreshRmgLinePoint')
    changed=original.replace('TRmgLinePainterInterface* painter,','TRmgLinePainterInterface& painter,').replace('painter->','painter.')
    modified=source.replace(original,changed)
    assert modified.count('refreshRmgLinePoint(painter,')==4
    assert modified.count('refreshRmgLinePoint(m_painter,')==2
    modified=modified.replace('refreshRmgLinePoint(painter,','refreshRmgLinePoint(*painter,').replace('refreshRmgLinePoint(m_painter,','refreshRmgLinePoint(*m_painter,')
    old='void refreshRmgLinePoint(TRmgLinePainterInterface* painter, const TRmgGridPoint& point);'
    new=old.replace('Interface* painter','Interface& painter')
    assert header.count(old)==1
    runner=Path(__file__).with_name('run-rmg-line-painter-reference-family.py')
    payload=dict(schema=1,units=['rmg','rmg_support','rmg_terrain','scenarioinfo','singleselectionpopups','singleselectionwindow','tiles'],evidence=__doc__,
        diagnostic_runner_sha256=hashlib.sha256(runner.read_bytes()).hexdigest(),axes=[dict(name='line_painter_interface',source='src/rmg_terrain.cpp',find=source,
        options=[dict(name='pointer_control',replace=source),dict(name='reference_painter',replace=modified,extra_edits=[dict(source='include/rmg.h',find=old,replace=new)])])])
    args.output.write_text(json.dumps(payload,indent=2)+'\n')
    load_manifest(args.output,HOMM3_DIR)
    print('two nonnull painter interface states')


if __name__=='__main__':main()
