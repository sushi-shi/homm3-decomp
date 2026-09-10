#!/usr/bin/env python3
"""Retail video rectangle field results and DirectDraw local lifetimes."""
import argparse
import itertools
import json
from pathlib import Path

p=argparse.ArgumentParser(description=__doc__)
p.add_argument('output',type=Path)
a=p.parse_args()
root=Path(__file__).resolve().parents[2]
s=(root/'src/smackmgr.cpp').read_text()
begin=s.index('void videoDrawRects()\n{')
body=s[begin:s.index('\n}\n',begin)+2]
smack_head='        while (_SmackToBufferRect(smk, g_smackBufferFlags)) {\n'
bink_head='            for (i = 1; i < bnk->m_numRects; i++) {\n'
geometry='''            POINT pt;
            RECT dst;
            RECT src;
            DDSURFACEDESC ddsd;
'''
assert all(body.count(x)==1 for x in (smack_head,bink_head,geometry))

def capture_loop(source,kind,head,after,fields,indent):
    if not kind:return source
    start=source.index(head)
    end=source.index(after,start)
    block=source[start:end]
    count=2 if kind==1 else 4
    declarations=''
    for member,name in fields[:count]:
        declarations+=indent+'long '+name+' = '+member+';\n'
        block=block.replace(member,name)
    block=block.replace(head,head+declarations)
    return source[:start]+block+source[end:]

options=[]
for smk_kind,bnk_kind,geometry_scope in itertools.product(range(3),range(3),range(4)):
    c=capture_loop(body,smk_kind,smack_head,'        g_windowManager->updateScreen(x, y, w, h);',[
        ('smk->m_lastRectx','rectX'),('smk->m_lastRecty','rectY'),
        ('smk->m_lastRectw','rectWidth'),('smk->m_lastRecth','rectHeight')],'            ')
    c=capture_loop(c,bnk_kind,bink_head,'            g_windowManager->updateScreen(g_binkX + x',[
        ('bnk->m_frameRects[i].m_left','rectX'),('bnk->m_frameRects[i].m_top','rectY'),
        ('bnk->m_frameRects[i].m_width','rectWidth'),('bnk->m_frameRects[i].m_height','rectHeight')],'                ')
    if geometry_scope:
        if geometry_scope==1:
            # The screen-space objects can belong to the function even
            # though only the overlay path initializes or consumes them.
            c=c.replace(geometry,'')
            c=c.replace('void videoDrawRects()\n{\n','void videoDrawRects()\n{\n'+
                '\n'.join(line[8:] for line in geometry.rstrip().splitlines())+'\n\n',1)
        elif geometry_scope==2:
            c=c.replace(geometry,'')
            c=c.replace('        Bink* bnk;\n','        Bink* bnk;\n'+
                '\n'.join(line[4:] for line in geometry.rstrip().splitlines())+'\n',1)
        else:
            # Delay declarations until each actual initialization stage.
            c=c.replace(geometry,'')
            c=c.replace('            pt.x = 0;','            POINT pt;\n            pt.x = 0;',1)
            c=c.replace('            dst.left = 0;','            RECT dst;\n            dst.left = 0;',1)
            c=c.replace('            src.left = 0;','            RECT src;\n            src.left = 0;',1)
            c=c.replace('            memset(&ddsd, 0, sizeof(ddsd));','            DDSURFACEDESC ddsd;\n            memset(&ddsd, 0, sizeof(ddsd));',1)
    o={'name':f'smack-{smk_kind}-bink-{bnk_kind}-geometry-{geometry_scope}'}
    if smk_kind or bnk_kind or geometry_scope:o['replace']=c
    options.append(o)
a.output.write_text(json.dumps({
    'schema':1,'source':'src/smackmgr.cpp','units':['smackmgr'],
    'axes':[{'name':'sdk-field-results-and-window-geometry-lifetimes','find':body,'options':options}],
    'evidence':[
        'Fresh full DC show/lines/asm/inline-clues and retail sema summary/structure/source/calls pass in continued/rects-fresh-*.txt. DC14ac60 is a4-byte platform stub; no body facts are inferred from its line boundary.',
        'Retail Smack loop loads x into EDX, y into ECX and keeps x/w/h stack homes around SDK calls. Bink loop likewise loads each SDK rectangle field before its comparisons. Test actual signed long coordinate results or all four fields, preserving every original comparison/store and the x/y-before-extents behavior.',
        'The SDK inputs are plain fields with no intervening calls or writes to the input rect inside the loop. Earlier pure captures preserve their values; no volatile, dummy operation, or fake API is introduced.',
        'Retail reserves0x9c stack bytes; current0x94. POINT/RECT/RECT/DDSURFACEDESC have real address-taking lifetimes in the overlay arm. Test declarations in the existing overlay scope, owning Bink scope, whole function, or at each initialization without changing APIs, object types, or initialization order.',
        '36 finite states on the fully checked current parent. Retain the BinkRect bounds and canonical SDK/window interfaces; score all smackmgr siblings.',
    ],
},indent=2)+'\n')
