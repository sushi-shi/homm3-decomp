"""Exercise map-save failure propagation, count bytes and changing list bounds."""
import importlib.util
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[3]


class MapSaveLifetimeTests(unittest.TestCase):
    @unittest.skipUnless(shutil.which('g++'), 'requires native C++ compiler')
    def test_save_order_failures_and_live_list_bounds(self):
        spec = importlib.util.spec_from_file_location('map_save_lifetimes',
            ROOT / 'scripts/experiments/generate-map-save-lifetime-family.py')
        family = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(family)
        actual = family.original_body()
        variants = [('Actual', actual, True)]
        variants += [(f'Family{i}', body, True) for i, (_,body) in enumerate(family.variants(actual))]
        variants += [
            ('WrongLayers', actual.replace('if (twoLayers)', 'if (!twoLayers)'), False),
            ('WrongQuestGate', actual.replace('        outfile->write(&count, 2);',
                '        if (static_cast<unsigned>(outfile->write(&count, 2)) < 2) return -1;'), False),
            ('WrongSeerGate', actual.replace('            m_seerHutList[i].save(outfile);',
                '            if (m_seerHutList[i].save(outfile) < 0) return -1;'), False),
            ('CachedBounds', actual.replace('i < m_seerHutList.size()', 'i < static_cast<unsigned>(count)')
                .replace('i < m_questGuardList.size()', 'i < static_cast<unsigned>(count)'), False),
        ]
        fixture = r'''
#include <vector>
#include <cstdio>
struct NewfullMap;
struct TAbstractFile {
    NewfullMap* map;
    int seerStatus, questStatus, writes;
    bool grow;
    std::vector<int> trace, counts;
    int write(const void* buffer,int size) {
        trace.push_back(50+writes);
        if(size!=2) {trace.push_back(9999);return 0;}
        const unsigned char* b=static_cast<const unsigned char*>(buffer);
        counts.push_back(b[0]+256*b[1]);
        return writes++ ? questStatus : seerStatus;
    }
};
struct TSeerHut {int id; int save(TAbstractFile*);};
struct TQuestGuard {int id; void save(TAbstractFile*);};
struct NewfullMap {
    std::vector<TSeerHut> m_seerHutList;
    std::vector<TQuestGuard> m_questGuardList;
    int fail;
    int stage(TAbstractFile* file,int id) {file->trace.push_back(id);return id==fail?-7:7;}
    int saveMapLayer(TAbstractFile* f,int size,int layer) {
        if(size!=17) f->trace.push_back(9998);
        return stage(f,layer);
    }
    int saveMapObjects(TAbstractFile* f) {return stage(f,2);}
    int saveBlackBoxList(TAbstractFile* f) {return stage(f,3);}
    int saveTreasureList(TAbstractFile* f) {return stage(f,4);}
    int saveMonsterList(TAbstractFile* f) {return stage(f,5);}
    int saveTimedEventList(TAbstractFile* f) {return stage(f,6);}
    int saveTownEventList(TAbstractFile* f) {return stage(f,7);}
    // @DECLARATIONS@
};
int TSeerHut::save(TAbstractFile* file) {
    file->trace.push_back(100+id);
    if(file->grow && id==0) {TSeerHut extra={2};file->map->m_seerHutList.push_back(extra);}
    return -1; // The Complete driver deliberately ignores this result.
}
void TQuestGuard::save(TAbstractFile* file) {
    file->trace.push_back(200+id);
    if(file->grow && id==0) {TQuestGuard extra={2};file->map->m_questGuardList.push_back(extra);}
}
// @FUNCTIONS@
bool check(int (NewfullMap::*save)(TAbstractFile*,int,unsigned char)) {
    for(int layers=0;layers<2;++layers)
    for(int fail=-1;fail<8;++fail)
    for(int first=-1;first<4;++first)
    for(int second=-1;second<4;++second)
    for(int grow=0;grow<2;++grow)
    for(int nonempty=0;nonempty<2;++nonempty) {
        NewfullMap map;map.fail=fail;
        for(int i=0;i<2*nonempty;++i) {
            TSeerHut hut={i};map.m_seerHutList.push_back(hut);
            TQuestGuard guard={i};map.m_questGuardList.push_back(guard);
        }
        TAbstractFile file;file.map=&map;file.seerStatus=first;
        file.questStatus=second;file.writes=0;file.grow=grow;
        std::vector<int> expected,counts;
        bool failed=false;
        for(int stage=0;stage<6;++stage) {
            if(stage==1 && !layers) continue;
            expected.push_back(stage);
            if(stage==fail) {failed=true;break;}
        }
        if(!failed) {
            expected.push_back(50);counts.push_back(2*nonempty);
            if(first==0 || first==1) failed=true;
        }
        if(!failed) {
            for(int i=0;i<nonempty*(2+grow);++i) expected.push_back(100+i);
            expected.push_back(51);counts.push_back(2*nonempty);
            for(int i=0;i<nonempty*(2+grow);++i) expected.push_back(200+i);
            expected.push_back(6);
            if(fail==6) failed=true;
            else {expected.push_back(7);if(fail==7)failed=true;}
        }
        const int result=(map.*save)(&file,17,layers);
        if(result!=(failed?-1:0) || file.trace!=expected || file.counts!=counts) return false;
    }
    return true;
}
int main() {
// @CHECKS@
}
'''
        declarations, functions, checks = [], [], []
        for name,body,expected in variants:
            declarations.append(f'int save{name}(TAbstractFile*,int,unsigned char);')
            functions.append(body.replace('NewfullMap::save(',f'NewfullMap::save{name}(',1))
            checks.append(f'if(check(&NewfullMap::save{name}) != {str(expected).lower()}) '
                          f'{{std::puts("{name}");return 1;}}')
        source=fixture.replace('// @DECLARATIONS@','\n'.join(declarations))
        source=source.replace('// @FUNCTIONS@','\n'.join(functions)).replace('// @CHECKS@','\n'.join(checks))
        with tempfile.TemporaryDirectory(prefix='homm3-map-save-') as temp:
            cpp=Path(temp)/'test.cpp';binary=Path(temp)/'test';cpp.write_text(source)
            built=subprocess.run(['g++','-std=c++17','-O1',str(cpp),'-o',str(binary)],capture_output=True,text=True)
            self.assertEqual(built.returncode,0,built.stderr)
            result=subprocess.run([str(binary)],capture_output=True,text=True)
            self.assertEqual(result.returncode,0,result.stdout+result.stderr)


if __name__ == '__main__':
    unittest.main()
