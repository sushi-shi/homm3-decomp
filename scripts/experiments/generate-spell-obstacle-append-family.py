"""Restore the four source-proven obstacle push_back calls in CastSpell.

DC spells.cpp lines 849/925/962/996 all call vector<TObstacle>::push_back.
Three jsr sites load their named callee into r0 in an earlier source-line
block (0x14feaa/0x1500be/0x1502c0), so the dossier's local call attribution
alone reports them as indirect. Retail +0x64e/+0x86c/+0x9a2/+0xae4 keeps
count-insert bodies out of line; push_back can supply that nested call.
Preserve the canonical vector/record, each complete local obstacle, its
insertion before size()-1, and the subsequent PlaceObstacle argument/order.
The finite family isolates each public append boundary with no pin.
"""
import argparse
import json
from pathlib import Path

ROOT=Path(__file__).resolve().parents[2]


def make_manifest():
    source=(ROOT/'src/spells.cpp').read_text()
    start=source.index('void combatManager::castSpell(')
    finish=source.index('\n}\n',start)+2
    body=source[start:finish]
    axes=[]
    for name,next_name,value in [('QUICKSAND','LAND_MINE','newQuicksand'),
                                ('LAND_MINE','FORCE_FIELD','newLandmine'),
                                ('FORCE_FIELD','FIRE_WALL','newWall'),
                                ('FIRE_WALL','EARTHQUAKE','newWall')]:
        start=body.index('    case SPELL_'+name+':')
        end=body.index('    case SPELL_'+next_name+':',start)
        arm=body[start:end]
        insertion=f'm_obstacles.insert(m_obstacles.end(), 1, {value});'
        if arm.count(insertion)!=1:
            raise ValueError('Review append source in '+name)
        axes.append({'name':name.lower()+'-append','find':arm,'options':[
            {'name':'unchanged'}, {'name':'source-push-back','replace':arm.replace(insertion,f'm_obstacles.push_back({value});')}]})
    return {'schema':1,'source':'src/spells.cpp','units':['spells'],'evidence':__doc__,'axes':axes}


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output',type=Path)
    args=parser.parse_args()
    args.output.parent.mkdir(parents=True,exist_ok=True)
    args.output.write_text(json.dumps(make_manifest(),indent=2)+'\n')
