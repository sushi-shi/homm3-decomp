#!/usr/bin/env python3
"""Video bound captures and source-rectangle initialization from two parents."""
import argparse
import itertools
import json
from pathlib import Path

p=argparse.ArgumentParser(description=__doc__)
p.add_argument('output',type=Path)
a=p.parse_args()
root=Path(__file__).resolve().parents[2]
context=root/'build/source-families/a0b7ae8978754f63cbff'
def function(s):
 b=s.index('void videoDrawRects()\n{');return s[b:s.index('\n}\n',b)+2]
body=function((root/'src/smackmgr.cpp').read_text())
assert body==function((context/'snapshot/src/smackmgr.cpp').read_text())
checkpoint=json.loads((context/'checkpoint.json').read_text())
parents=[]
for ident in ('da341e8f602aa4dde9786718','db3d1eb678b25767ba0c04a1'):
 row=next(row for row in checkpoint['elites'] if row['id']==ident)
 repeat=context/'candidates'/ident/'repeat';result=json.loads((repeat/'result.json').read_text())
 assert result['scores']==row['scores'] and result['object_hash']==row['object_hash']
 parents.append((ident,function((repeat/'tree/src/smackmgr.cpp').read_text())))
fields={'x':'m_left','y':'m_top','w':'m_width','h':'m_height'}
def captures(kind,order):
 if kind=='smack':return '\n'.join('        '+v+' = smk->m_lastRect'+v+';' for v in order)
 return '\n'.join('            '+v+' = bnk->m_frameRects[0].'+fields[v]+';' for v in order)
src='''            src.left = 0;
            src.top = 0;
            src.right = bnk->m_width;
            src.bottom = bnk->m_height;'''
options=[{'name':'authored-all-function-control'}]
for ident,parent in parents:
 assert parent.count(src)==1
 assert parent.count(captures('smack','whxy'))==parent.count(captures('bink','whxy'))==1
 for smack_order,bink_order,dimensions_first,chain_zero in itertools.product(('whxy','xywh','xwyh'),('whxy','xywh','xwyh'),range(2),range(2)):
  c=parent.replace(captures('smack','whxy'),captures('smack',smack_order))
  c=c.replace(captures('bink','whxy'),captures('bink',bink_order))
  zero='            src.left = src.top = 0;' if chain_zero else '            src.left = 0;\n            src.top = 0;'
  dims='            src.right = bnk->m_width;\n            src.bottom = bnk->m_height;'
  c=c.replace(src,dims+'\n'+zero if dimensions_first else zero+'\n'+dims)
  options.append({'name':f'{ident}-{smack_order}-{bink_order}-dimensions-{dimensions_first}-chain-{chain_zero}','replace':c})
a.output.write_text(json.dumps({
 'schema':1,'source':'src/smackmgr.cpp','units':['smackmgr'],
 'axes':[{'name':'retail-bound-capture-stages','find':body,'options':options}],
 'evidence':[
  'Reproduced parents from a0b7ae8978754f63cbff: da341e8f602aa4dde9786718(90.3379,SDKbounds,0x98frame) and db3d1eb678b25767ba0c04a1(89.7123,longscalars,0x9cframe). Authored snapshot, repeated scores and object identity verified.',
  'The long-scalar parent reproduces every DirectDraw object address: POINT-0x10,dst-0x20,src-0x30,DDSURFACEDESC-0x9c. Its lower aggregate score is retained as a useful source-lifetime lead.',
  'Retail source-rectangle preparation loads Bink width and height then stores right before zero left/top and bottom. Compare coordinate-zero-first versus dimension-first source groups, with separate versus chained zero assignments. Preserve API calls and initialization stage; these are pure stores into the same actual RECT.',
  'Retail initial Smack loads x,w,h,y; Bink loads w,h,x,y. Earlier capture-order trials coupled both paths. Independently test dimensions-first, SDKlayout(x,y,w,h), and axis-paired(x,w,y,h) initial bound captures on both parents. Each reads the same four signed SDK fields once, after its SDK call and before any union updates.',
  '73 finite states including authored control; no dummy alternatives or synthetic operations. DC is a platform stub; source punctuation and scopes remain retail-tested hypotheses.',
 ],
},indent=2)+'\n')
