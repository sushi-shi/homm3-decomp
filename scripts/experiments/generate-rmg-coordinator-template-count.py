#!/usr/bin/env python3
"""Separate generation's template-count and selected-index source values.

Retail 0x549930 computes the template count into EDI before rand, divides by
it, then preserves the chosen remainder in EBX across string assignment.
The candidate reuses EDI for that remainder. Immutable reference selection
already fixes the later human/computer-load order but leaves those four
register operands. Test a named denominator, preserving the separate empty
check and every live template-vector query after selection. No pointer cache,
new helper, dummy use or callback change is introduced. RMG has no DC record.
"""
import argparse
import json
from pathlib import Path
from homm3.core.common import HOMM3_DIR
from homm3.vc6.source_families import load_manifest
from homm3.vc6.test_rmg_families import generator


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output',type=Path)
    args=parser.parse_args()
    source=(HOMM3_DIR/'src/rmg.cpp').read_text()
    original=generator('generate-rmg-coordinator-family.py').definition(source)
    old='    unsigned int selected = rand() % m_templates.size();\n'
    assert original.count(old)==1
    options=[]
    for index_type in ('unsigned int','const unsigned int&'):
        for label,count_type in (('expression',None),('value','unsigned int'),('constant','const unsigned int'),
                                 ('reference','const unsigned int&'),('signed','int'),('constant_signed','const int'),('reference_signed','const int&')):
            lines=[]
            denominator='m_templates.size()'
            if count_type:
                lines.append('    '+count_type+' templateCount = m_templates.size();')
                denominator='templateCount'
            lines.append('    '+index_type+' selected = rand() % '+denominator+';')
            options.append(dict(name=('index_reference' if '&' in index_type else 'index_value')+'+'+label,
                                replace=original.replace(old,'\n'.join(lines)+'\n')))
    assert options[0]['replace']==original
    payload=dict(schema=1,source='src/rmg.cpp',units=['rmg'],evidence=__doc__,axes=[dict(name='template_count_lifetime',find=original,options=options)])
    args.output.parent.mkdir(parents=True,exist_ok=True)
    args.output.write_text(json.dumps(payload,indent=2)+'\n')
    load_manifest(args.output,HOMM3_DIR)
    print('14 template-count and selected-index ownership controls')


if __name__=='__main__':main()
