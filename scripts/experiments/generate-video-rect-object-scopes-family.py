#!/usr/bin/env python3
"""Individual DirectDraw object scopes from the reproduced video parent."""
import argparse
import json
from pathlib import Path

p=argparse.ArgumentParser(description=__doc__)
p.add_argument('output',type=Path)
a=p.parse_args()
root=Path(__file__).resolve().parents[2]
s=(root/'src/smackmgr.cpp').read_text()
def function(s):
 b=s.index('void videoDrawRects()\n{');return s[b:s.index('\n}\n',b)+2]
body=function(s)
parent=root/'build/source-families/acc58cffad04bcf4a073/candidates/c54ecd27b6d7859533ca903a/repeat'
assert function((parent/'tree/src/smackmgr.cpp').read_text())==body
results=json.loads((parent/'result.json').read_text())
assert results['scores']['smackmgr|?videoDrawRects@@YIXXZ']==90.3333
objects=('POINT pt;', 'RECT dst;', 'RECT src;', 'DDSURFACEDESC ddsd;')
options=[]
for mask in [15,*range(15)]:
 c=body;inside=[]
 for bit,decl in enumerate(objects):
  assert c.count('    '+decl+'\n')==1
  if not mask&(1<<bit):
   c=c.replace('    '+decl+'\n','',1)
   inside.append('            '+decl)
 if inside:
  c=c.replace('        } else {\n\n            pt.x = 0;',
      '        } else {\n'+ '\n'.join(inside)+'\n\n            pt.x = 0;',1)
  assert all(c.count(decl)==1 for decl in objects)
 o={'name':'function-scope-mask-'+str(mask)}
 if mask!=15:o['replace']=c
 options.append(o)
a.output.write_text(json.dumps({
 'schema':1,'source':'src/smackmgr.cpp','units':['smackmgr'],
 'axes':[{'name':'directdraw-object-scopes','find':body,'options':options}],
 'evidence':[
  'Reproduced parentc54ecd27b6d7859533ca903a in acc58cffad04bcf4a073 has90.3333; exact authored body and repeated result checked.',
  'Retail reserves0x9c, all-overlay declarations0x94, all-function declarations0xa8. POINT resides at ebp-0x10, destination RECT at-0x20, source RECT at-0x30, DDSURFACEDESC at-0x9c. These address-taking locals overlap earlier scalar lifetimes differently. Isolate their four real source scopes; retain initialization and API order.',
  'All16 function/overlay-scope combinations are meaningful declaration-lifetime hypotheses. No new object, padding, false inline declaration, altered SDK field, or dummy operation is added.',
  'DC is a platform stub; it supplies no local-scope evidence for this retail body. Both scopes remain hypotheses until x86 validation, and score alone does not recover unique source.',
 ],
},indent=2)+'\n')
