#!/usr/bin/env python3
"""Recombine reproduced video object scopes with signed scalar bounds."""
import argparse
import json
from pathlib import Path

p=argparse.ArgumentParser(description=__doc__)
p.add_argument('parent_checkpoint',type=Path)
p.add_argument('output',type=Path)
a=p.parse_args()
root=Path(__file__).resolve().parents[2]
def function(s):
 b=s.index('void videoDrawRects()\n{');return s[b:s.index('\n}\n',b)+2]
source=(root/'src/smackmgr.cpp').read_text();body=function(source)
assert function((a.parent_checkpoint.parent/'snapshot/src/smackmgr.cpp').read_text())==body
anchor='''    BinkRect bounds;
    long& x = bounds.m_left;
    long& y = bounds.m_top;
    long& w = bounds.m_width;
    long& h = bounds.m_height;'''
options=[{'name':'authored-all-function-control'}]
parents=[]
for row in json.loads(a.parent_checkpoint.read_text())['elites']:
 repeat=a.parent_checkpoint.parent/'candidates'/row['id']/'repeat'
 result=json.loads((repeat/'result.json').read_text())
 assert result['scores']==row['scores'] and result['object_hash']==row['object_hash']
 parent=function((repeat/'tree/src/smackmgr.cpp').read_text())
 assert parent.count(anchor)==1
 parents.append(row['id'])
 for kind in ('sdk-bound-references','long-scalars','int-scalars'):
  c=parent
  if kind!='sdk-bound-references':
   typ='long' if kind=='long-scalars' else 'int'
   c=c.replace(anchor,'    '+typ+' x, y, w, h;')
  if c!=body:options.append({'name':row['id']+'-'+kind,'replace':c})
a.output.write_text(json.dumps({
 'schema':1,'source':'src/smackmgr.cpp','units':['smackmgr'],
 'axes':[{'name':'reproduced-scope-and-bound-types','find':body,'options':options}],
 'evidence':[
  'Parentcheckpoint '+str(a.parent_checkpoint),
  'Eight retained parents each verified against repeat scores, emitted-object identity and source snapshot: '+','.join(parents),
  'Current whole-function source matches the parent snapshot. Scope family produced frames0x94/0x98/0xa4/0xa8, never retail0x9c; its best90.3379 uses function-scope POINT/sourceRECT/DDSURFACEDESC and overlay-scope destinationRECT.',
  'Retail bounds are signed32-bit values. SDK Smack LastRect fields and BinkRect fields are signed long; current actual BinkRect reference variables also have long type. Earlier scalar experiment used int. Test direct signed long scalars as the missing canonical SDK result type and int scalars as the prior-boundary negative control across the reproduced scopes.',
  'The local BinkRect bounds was a reconstruction hypothesis, not a DC-proven local: DC is a platform stub. This family preserves the proven SDK structures and declarations at their input/API boundaries, with no new type or false helper.',
  'All captures, comparisons, update order, actual screen geometry and API calls remain unchanged. No padding or artificial liveness operations.',
 ],
},indent=2)+'\n')
