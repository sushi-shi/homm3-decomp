"""Validate the modern campaign wire format across the reviewed local lifetimes.

The native fixture imports the actual modern arm and all 32 generated arms.
Legacy promotion is unchanged and excluded from this host layout fixture;
VC6/retail comparison remains the ABI and legacy-construction verdict.
"""
import importlib.util
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[3]


class CampaignLoadLifetimeTests(unittest.TestCase):
    @unittest.skipUnless(shutil.which('g++'), 'requires native C++ compiler')
    def test_modern_wire_format_and_conversions(self):
        spec = importlib.util.spec_from_file_location('campaign_lifetimes',
            ROOT / 'scripts/experiments/generate-campaign-load-lifetime-family.py')
        family = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(family)
        actual = family.original_body()
        marker = '        return;\n    }\n\n'
        prefix, modern = actual.split(marker, 1)
        variants = [('Actual', modern, True)]
        for index, (_, body) in enumerate(family.variants(actual)):
            legacy, arm = body.split(marker, 1)
            self.assertEqual(legacy, prefix, 'The family must preserve the entire legacy arm')
            variants.append((f'Family{index}', arm, True))
        variants += [
            ('WrongRemap', modern.replace('== PRE36_CAMPAIGN_REMAP_SOURCE',
                                         '== -999'), False),
            ('WrongDays', modern.replace('scenario.m_days = days;',
                                        'scenario.m_days = 0;'), False),
            ('WrongSign', modern.replace('static_cast<signed char>',
                                        'static_cast<unsigned char>'), False),
            ('WrongArtifact', modern.replace('artifactPool[whichArtifact].m_artifactId = artifactValue.m_artifact;',
                                             'artifactPool[whichArtifact].m_artifactId = TArtifact(0);'), False),
            ('WrongFlags', modern.replace('m_campaignCompleted + 14,',
                                          'm_campaignCompleted + 15,'), False),
        ]
        fixture = r'''
#include <algorithm>
#include <cstring>
#include <string>
#include <vector>
#include <cstdio>
struct TAbstractFile {
    std::vector<unsigned char> bytes;
    std::vector<int> expected, observed;
    unsigned cursor;
    bool bad;
    TAbstractFile() : cursor(0), bad(false) {}
    void put(unsigned value, int width) {
        expected.push_back(width);
        for(int i=0;i<width;++i) bytes.push_back((value >> (8*i)) & 255);
    }
    int read(void* data, int width) {
        observed.push_back(width);
        if(width<0 || cursor+width>bytes.size()) {bad=true; return 0;}
        std::memcpy(data, &bytes[cursor], width); cursor+=width; return width;
    }
};
std::string readLengthPrefixedString(TAbstractFile* file) {
    int count; file->read(&count, 4);
    if(count<0 || count>16) {file->bad=true; return std::string();}
    char data[16]; file->read(data,count); return std::string(data,count);
}
enum TArtifact { ARTIFACT_NONE=-1 };
struct type_artifact { TArtifact m_artifactId; int m_extra; };
struct hero {
    int id, version;
    void load(TAbstractFile* file,int saveVersion) {file->read(&id,4);version=saveVersion;}
};
struct CampaignScenarioInfo {
    bool m_completed;
    int m_days, m_score, m_index, m_completeOrder;
};
const int PRE36_CAMPAIGN_REMAP_SOURCE=13, PRE36_CAMPAIGN_REMAP_TARGET=20;
struct SCampaign {
    typedef CampaignScenarioInfo MapScore;
    unsigned char m_isCheater, m_secretActive, m_crossoverArrayIndex;
    signed char m_currentMap;
    int m_currentCampaign, m_numMapRegions, m_briefingChoice;
    std::string m_campaignFilename;
    unsigned char m_campaignCompleted[21];
    std::vector<MapScore> m_mapScores;
    std::vector<std::vector<hero> > m_carryOverHeroes;
    std::vector<std::vector<type_artifact> > m_carryoverArtifact;
    std::vector<int> m_assignedCarryover;
    // @DECLARATIONS@
};
// @FUNCTIONS@
bool check(void (SCampaign::*load)(TAbstractFile*,int)) {
    for(int version=28;version<=36;version+=(version==28?7:1))
    for(unsigned sample=0;sample<256;++sample)
    for(unsigned count=0;count<4;count+= count==0?1:2) {
        TAbstractFile file;
        for(int i=0;i<7;++i) file.put(sample,1);
        file.put(3,4); file.expected.push_back(3);
        file.bytes.push_back('h');file.bytes.push_back('3');file.bytes.push_back('c');
        const int flags=version<36?14:21;
        file.expected.push_back(flags);
        for(int i=0;i<flags;++i) file.bytes.push_back((sample+i)&255);
        file.put(count,1);
        for(unsigned i=0;i<count;++i) {
            file.put(sample+i,1); file.put(0x80000000u+sample+i,4);
            file.put(0x10203040u-sample-i,4); file.put(sample+i,1); file.put(sample+i+1,1);
        }
        file.put(count,1);
        for(unsigned pool=0;pool<count;++pool) {
            file.put(2,1); file.put(sample+pool,4); file.put(sample+pool+1000,4);
            file.put(2,2);
            file.put(0x8000u+sample+pool,2); file.put(0xffffu-sample-pool,2);
            file.put(sample+pool,2); file.put(0x7fffu-sample-pool,2);
        }
        file.put(count,1);
        for(unsigned i=0;i<count;++i) file.put(0x8000u+sample+i,2);
        SCampaign campaign;
        std::memset(campaign.m_campaignCompleted,0xa5,21);
        campaign.m_mapScores.resize(5); campaign.m_carryOverHeroes.resize(5);
        campaign.m_carryoverArtifact.resize(5); campaign.m_assignedCarryover.resize(5);
        (campaign.*load)(&file,version);
        if(file.bad || file.cursor!=file.bytes.size() || file.observed!=file.expected) return false;
        int campaignId=version<36 && sample==13?20:sample;
        if(campaign.m_isCheater!=(sample!=0) || campaign.m_secretActive!=(sample!=0)
           || campaign.m_currentMap!=(signed char)sample || campaign.m_currentCampaign!=campaignId
           || campaign.m_numMapRegions!=(signed char)sample || campaign.m_briefingChoice!=(signed char)sample
           || campaign.m_crossoverArrayIndex!=sample || campaign.m_campaignFilename!="h3c") return false;
        for(int i=0;i<21;++i)
            if(campaign.m_campaignCompleted[i]!=(i<flags?(sample+i)&255:0)) return false;
        if(campaign.m_mapScores.size()!=count || campaign.m_carryOverHeroes.size()!=count
           || campaign.m_carryoverArtifact.size()!=count || campaign.m_assignedCarryover.size()!=count) return false;
        for(unsigned i=0;i<count;++i) {
            const CampaignScenarioInfo& score=campaign.m_mapScores[i];
            if(score.m_completed!=(((sample+i)&255)!=0)
               || (unsigned)score.m_days!=0x80000000u+sample+i
               || (unsigned)score.m_score!=0x10203040u-sample-i
               || score.m_completeOrder!=(signed char)(sample+i)
               || score.m_index!=(signed char)(sample+i+1)) return false;
            if(campaign.m_carryOverHeroes[i].size()!=2 || campaign.m_carryoverArtifact[i].size()!=2) return false;
            for(int h=0;h<2;++h)
                if(campaign.m_carryOverHeroes[i][h].id!=int(sample+i+1000*h)
                   || campaign.m_carryOverHeroes[i][h].version!=version) return false;
            const std::vector<type_artifact>& artifacts=campaign.m_carryoverArtifact[i];
            if(int(artifacts[0].m_artifactId)!=(short)(0x8000u+sample+i)
               || artifacts[0].m_extra!=(short)(0xffffu-sample-i)
               || int(artifacts[1].m_artifactId)!=int(sample+i)
               || artifacts[1].m_extra!=int(0x7fffu-sample-i)
               || campaign.m_assignedCarryover[i]!=(short)(0x8000u+sample+i)) return false;
        }
    }
    return true;
}
int main() {
// @CHECKS@
}
'''
        declarations, functions, checks = [], [], []
        for name, arm, expected in variants:
            declarations.append(f'void load{name}(TAbstractFile*, int);')
            functions.append(f'void SCampaign::load{name}(TAbstractFile* infile, int saveVersion) {{\n'
                '    int i; int pool;\n    std::vector<MapScore>& rMapScores = m_mapScores;\n'
                '    rMapScores.clear();\n    m_carryOverHeroes.clear();\n' + arm)
            checks.append(f'if(check(&SCampaign::load{name}) != {str(expected).lower()}) '
                          f'{{std::puts("{name}");return 1;}}')
        source = fixture.replace('// @DECLARATIONS@','\n'.join(declarations))
        source = source.replace('// @FUNCTIONS@','\n'.join(functions)).replace('// @CHECKS@','\n'.join(checks))
        with tempfile.TemporaryDirectory(prefix='homm3-campaign-load-') as temp:
            cpp=Path(temp)/'test.cpp'; binary=Path(temp)/'test'
            cpp.write_text(source)
            built=subprocess.run(['g++','-std=c++17','-O1',str(cpp),'-o',str(binary)],capture_output=True,text=True)
            self.assertEqual(built.returncode,0,built.stderr)
            result=subprocess.run([str(binary)],capture_output=True,text=True)
            self.assertEqual(result.returncode,0,result.stdout+result.stderr)


if __name__ == '__main__':
    unittest.main()
