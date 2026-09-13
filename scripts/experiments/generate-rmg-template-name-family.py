#!/usr/bin/env python3
"""Recover the borrowed template-name boundary in the generation coordinator.

Retail generate 0x549930 passes the selected template's string subobject to
string assignment and retains its numeric selection across that call. The
remaining four register operands use EBX rather than EDI. Test a canonical
const-reference name inspector, defined ordinarily beside TRmgTemplate's
other methods, and direct versus named borrowed results. Keep both live
later template-vector queries, and compare mutable/value versus immutable
reference selection. Declaration/body-only controls distinguish source calls
from unrelated TU state. No Dreamcast TRmgTemplate declaration survives, so
the getter is a source hypothesis and its name is provisional.
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
    header=(HOMM3_DIR/'include/rmg.h').read_text()
    helper=generator('generate-rmg-position-family.py')
    original=helper.definition(source,'type_random_map_generator::generate')
    anchor=helper.definition(source,'TRmgTemplate::findZone')
    get_name='const std::string& TRmgTemplate::getName() const\n{\n    return m_name;\n}'
    declaration='    TRmgTownSlot* findZone(int zoneIndex);'
    assert header.count(declaration)==1
    statement='    m_templateName = m_templates[selected]->m_name;'
    assert original.count(statement)==1
    options=[]
    for binding in ('value','borrowed'):
        body=original
        if binding=='borrowed':body=body.replace('unsigned int selected =','const unsigned int& selected =',1)
        for projection in ('field','declaration_only','expression','named_reference'):
            changed=body
            if projection=='expression':changed=body.replace(statement,'    m_templateName = m_templates[selected]->getName();')
            elif projection=='named_reference':changed=body.replace(statement,'    const std::string& name = m_templates[selected]->getName();\n    m_templateName = name;')
            option=dict(name=binding+'+'+projection,replace=changed)
            if projection!='field':
                option['extra_edits']=[dict(source='include/rmg.h',find=declaration,replace=declaration+'\n    const std::string& getName() const;'),
                                       dict(source='src/rmg.cpp',find=anchor,replace=anchor+'\n\n'+get_name)]
            options.append(option)
    assert options[0]['replace']==original
    payload=dict(schema=1,source='src/rmg.cpp',evidence=__doc__,
                 units=['rmg','rmg_support','rmg_terrain','tiles','singleselectionpopups','singleselectionwindow','scenarioinfo'],
                 axes=[dict(name='template_name_boundary',find=original,options=options)])
    args.output.parent.mkdir(parents=True,exist_ok=True)
    args.output.write_text(json.dumps(payload,indent=2)+'\n')
    load_manifest(args.output,HOMM3_DIR)
    print('8 template-name ownership and source-call controls')


if __name__=='__main__':main()
