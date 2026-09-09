"""Request-worker contract with actual request declaration and constructor.

The generator is an opaque scripted owner: capture constructor arguments,
mutate request seat inputs, and record generate/write/destruction boundaries.
This verifies the caller's behavior, not the generator algorithm or x86 ABI.
"""
import json
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

from homm3.vc6.test_rmg_families import generator


class RequestWorkerTests(unittest.TestCase):
    @unittest.skipUnless(shutil.which("g++"), "native caller oracle needs g++")
    def test_request_results_and_cleanup(self):
        root = Path(__file__).resolve().parents[2]
        source = (root / "src/rmg.cpp").read_text()
        header = (root / "include/rmg_request.h").read_text()
        module = generator("generate-rmg-request-worker-family.py")
        extract = generator("generate-rmg-position-family.py").definition
        current = module.definition(source)
        original = module.baseline(current)
        current_declaration = current_helper = ""
        if "generator.setHumanPlayer(seat);" in current:
            generator_header = (root / "include/rmg.h").read_text()
            for name, signature in [("setHumanPlayer", "int seat"), ("setTownChoice", "int seat, int town")]:
                declaration = "    void " + name + "(" + signature + ");"
                self.assertEqual(generator_header.count(declaration), 1)
                current_declaration += declaration + "\n"
                current_helper += extract(source, "type_random_map_generator::" + name) + "\n"
        forms = [("current", current, "unsigned char", "unsigned char", current_declaration, current_helper)] + [
            (name,body,"unsigned char","unsigned char","","") for name,body in module.variants(original)]
        for path in filter(None, os.environ.get("HOMM3_REQUEST_WORKER_MANIFEST", "").split(os.pathsep)):
            payload = json.loads(Path(path).read_text())
            for option in payload["axes"][0]["options"]:
                input_type=output_type="unsigned char"
                helper_declaration=helper_body=""
                for edit in option.get("extra_edits",[]):
                    if edit["source"]=="include/rmg_request.h" and "m_isHumanSeat[8]" in edit.get("find",""):
                        input_type=edit["replace"].split("m_isHumanSeat")[0].strip()
                    elif edit["source"]=="include/rmg.h" and "m_fixedHumanPlayers[8]" in edit.get("find",""):
                        output_type=edit["replace"].split("m_fixedHumanPlayers")[0].strip()
                    elif edit["source"]=="include/rmg.h" and edit.get("insert_before")=="    void removeObject(type_object* object);":
                        helper_declaration=edit["text"]
                    elif edit["source"]=="src/rmg.cpp" and edit.get("text","").startswith("// Provisional request seat setter:"):
                        helper_body=edit["text"]
                    else:
                        self.fail("unmodeled request-family edit: "+str(edit))
                self.assertEqual(bool(helper_declaration),bool(helper_body))
                forms.append((option["name"],option["replace"],input_type,output_type,helper_declaration,helper_body))
        forms = [(body,name,src,dst,decl,helper) for (body,src,dst,decl,helper),name in dict(
            ((body,src,dst,decl,helper),name) for name,body,src,dst,decl,helper in forms).items()]
        positive = len(forms)
        negatives = [
            ("lower_clamp", "strength = 1;", "strength = 0;"),
            ("upper_clamp", "strength = 5;", "strength = 6;"),
            ("repair_boundary", "m_computerPlayerCount < 2", "m_computerPlayerCount < 1"),
            ("repair_value", "m_computerPlayerCount = 1;", "m_computerPlayerCount = 2;"),
            ("team_argument", "m_computerTeamCount, m_waterContent", "m_humanTeamCount, m_waterContent"),
            ("progress_argument", "static_cast<TProgressSink*>(progress)", "static_cast<TProgressSink*>(0)"),
            ("normalize_flag", "generator.m_fixedHumanPlayers[seat] = 1;", "generator.m_fixedHumanPlayers[seat] = m_isHumanSeat[seat];"),
            ("town_argument", "= m_townType[seat];", "= m_townType[7 - seat];"),
            ("last_seat", "seat < 8", "seat < 7"),
            ("generate_result", "return RANDOM_MAP_FAILED_3;", "return RANDOM_MAP_FAILED_2;"),
            ("write_result", "result = RANDOM_MAP_FAILED_2;", "result = RANDOM_MAP_FAILED_3;"),
            ("missing_write", "if (!generator.writeMap(outfile))", "if (false)"),
            ("double_generate", "if (!generator.generate())", "generator.generate();\n    if (!generator.generate())"),
        ]
        for name, old, new in negatives:
            self.assertEqual(original.count(old), 1, name)
            forms.append((original.replace(old, new), name, "unsigned char", "unsigned char", "", ""))
        for name,selector,old,new in [
            ("human_setter","::setHumanPlayer(int seat)","m_fixedHumanPlayers[seat] = 1;","m_fixedHumanPlayers[seat] = 0;"),
            ("enabled_setter","::setHumanPlayer(int seat, unsigned char enabled)","m_fixedHumanPlayers[seat] = enabled;","m_fixedHumanPlayers[seat] = 0;"),
            ("town_setter","::setTownChoice(","m_townChoices[seat] = town;","m_townChoices[seat] = town + 1;"),
            ("combined_setter","::setPlayerOptions(","if (human)","if (!human)"),
        ]:
            matches=[row for row in forms[:positive] if selector in row[5]]
            if matches:
                body,_,src,dst,decl,helper=matches[0]
                self.assertEqual(helper.count(old),1,name)
                forms.append((body,name,src,dst,decl,helper.replace(old,new)))
        negative_count=len(forms)-positive
        declaration = header[header.index("class TRandomMapRequest {"):header.index("\n};", header.index("class TRandomMapRequest {"))+3]
        results = header[header.index("enum ERandomMapResult {"):header.index("\n};", header.index("enum ERandomMapResult {"))+3]
        constructor = extract(source, "TRandomMapRequest::TRandomMapRequest")
        text = "#include <algorithm>\n#include <climits>\n#include <cstring>\n#include <cstdio>\n#include <vector>\nusing std::memset;\n"
        text += "namespace std { template<class T> const T& _cpp_min(const T& a,const T& b) { return b<a?b:a; } template<class T> const T& _cpp_max(const T& a,const T& b) { return a<b?b:a; } }\n"
        text += results + r'''
struct TAbstractFile { int m_identity; };
struct TProgressSink { int m_identity; };
struct ProbeFailure { int m_stage; explicit ProbeFailure(int stage):m_stage(stage) {} };
struct RequestMutator { virtual void change(int index,int seed)=0; };
template<class Request> struct RequestMutation : RequestMutator {
    Request* m_request;
    explicit RequestMutation(Request* request):m_request(request) {}
    void change(int i,int seed) { m_request->m_isHumanSeat[i]=static_cast<unsigned char>((i+seed)%3==0?255:0);m_request->m_townType[i]=800+i*19; }
};
struct Context {
    int m_args[10],m_throwStage,m_seed,m_mutate,m_alive,m_destroyed;
    unsigned char m_generateResult,m_writeResult;
    unsigned char m_seenFlags[8];
    int m_seenTowns[8];
    RequestMutator* m_mutator;
    TProgressSink* m_progress;
    TAbstractFile* m_file;
    std::vector<int> m_events;
    Context():m_throwStage(0),m_seed(0),m_mutate(0),m_alive(0),m_destroyed(0),
        m_generateResult(0),m_writeResult(0),m_mutator(0),m_progress(0),m_file(0) {
        std::fill(m_args,m_args+10,-777);
        std::fill(m_seenFlags,m_seenFlags+8,177);
        std::fill(m_seenTowns,m_seenTowns+8,-777);
    }
};
static Context* g_context;
template<class Flag> struct GeneratorFixture {
    Flag m_fixedHumanPlayers[8];
    int m_townChoices[8];
    GeneratorFixture(int width,int height,int levels,int humans,int humanTeams,
        int computers,int computerTeams,int water,int strength,TProgressSink* progress,int version) {
        Context& c=*g_context;c.m_events.push_back(1);
        int args[]={width,height,levels,humans,humanTeams,computers,computerTeams,water,strength,version};
        std::copy(args,args+10,c.m_args);c.m_progress=progress;
        for(int i=0;i<8;++i) {
            m_fixedHumanPlayers[i]=static_cast<Flag>((c.m_seed+i)%3);
            m_townChoices[i]=-900-i;
            if(c.m_mutate) c.m_mutator->change(i,c.m_seed);
        }
        if(c.m_throwStage==1) throw ProbeFailure(1);
        ++c.m_alive;
    }
    ~GeneratorFixture() { --g_context->m_alive;++g_context->m_destroyed;g_context->m_events.push_back(4); }
    unsigned char generate() {
        Context& c=*g_context;c.m_events.push_back(2);
        std::copy(m_fixedHumanPlayers,m_fixedHumanPlayers+8,c.m_seenFlags);
        std::copy(m_townChoices,m_townChoices+8,c.m_seenTowns);
        if(c.m_throwStage==2) throw ProbeFailure(2);
        return c.m_generateResult;
    }
    unsigned char writeMap(TAbstractFile* file) {
        Context& c=*g_context;c.m_events.push_back(3);c.m_file=file;
        if(c.m_throwStage==3) throw ProbeFailure(3);
        return c.m_writeResult;
    }
};
template<class Request> bool equalRequest(const Request& a,const Request& b) {
    for(int i=0;i<8;++i) if(a.m_isHumanSeat[i]!=b.m_isHumanSeat[i] || a.m_townType[i]!=b.m_townType[i]) return false;
    return a.m_width==b.m_width && a.m_height==b.m_height && a.m_levels==b.m_levels
        && a.m_humanPlayerCount==b.m_humanPlayerCount && a.m_humanTeamCount==b.m_humanTeamCount
        && a.m_computerPlayerCount==b.m_computerPlayerCount && a.m_computerTeamCount==b.m_computerTeamCount
        && a.m_waterContent==b.m_waterContent && a.m_monsterStrength==b.m_monsterStrength && a.m_mapVersion==b.m_mapVersion;
}
template<class Request,class Flag> bool check() {
    int strengths[]={INT_MIN,-8,-4,-3,-2,-1,0,1,2,3,INT_MAX-3};
    int humans[]={-2,0,1,2,8},computers[]={-1,0,1,4};
    unsigned char answers[]={0,1,255};
    for(int si=0;si<11;++si) for(int hi=0;hi<5;++hi) for(int ci=0;ci<4;++ci)
    for(int pattern=0;pattern<8;++pattern) for(int gi=0;gi<3;++gi) for(int wi=0;wi<3;++wi)
    for(int mutate=0;mutate<2;++mutate) for(int throwing=0;throwing<4;++throwing) {
        int seed=si+hi+ci+pattern;
        Request request(36+si*3,24+hi*7,ci%3);
        request.m_humanPlayerCount=humans[hi];request.m_computerPlayerCount=computers[ci];
        request.m_humanTeamCount=hi-2;request.m_computerTeamCount=ci+5;
        request.m_waterContent=pattern%4;request.m_monsterStrength=strengths[si];request.m_mapVersion=pattern%3;
        for(int i=0;i<8;++i) {
            request.m_isHumanSeat[i]=static_cast<unsigned char>(pattern==0?0:pattern==1?1:pattern==2?255:((i+pattern)%3==0?255:0));
            request.m_townType[i]=i*17-pattern-1;
        }
        Request expected=request;
        if(humans[hi]+computers[ci]<2) { expected.m_humanPlayerCount=1;expected.m_computerPlayerCount=1; }
        int ordered[]={1,strengths[si]+3,5};std::sort(ordered,ordered+3);
        int expectedArgs[]={expected.m_width,expected.m_height,expected.m_levels,
            expected.m_humanPlayerCount,expected.m_humanTeamCount,expected.m_computerPlayerCount,
            expected.m_computerTeamCount,expected.m_waterContent,ordered[1],expected.m_mapVersion};
        if(mutate) for(int i=0;i<8;++i) { expected.m_isHumanSeat[i]=static_cast<unsigned char>((i+seed)%3==0?255:0);expected.m_townType[i]=800+i*19; }
        TProgressSink progress;TAbstractFile file;
        Context context;context.m_seed=seed;context.m_mutate=mutate;context.m_throwStage=throwing;
        context.m_generateResult=answers[gi];context.m_writeResult=answers[wi];
        RequestMutation<Request> mutation(&request);context.m_mutator=&mutation;g_context=&context;
        int result=-77,caught=0;
        try { result=request.generateToFile(&file,&progress); } catch(const ProbeFailure& failure) { caught=failure.m_stage; }
        std::vector<int> events;events.push_back(1);
        int expectedCatch=0,expectedResult=-77,expectedDestroyed=0;
        bool wrote=false;
        if(throwing==1) expectedCatch=1;
        else {
            events.push_back(2);
            if(throwing==2) expectedCatch=2;
            else if(gi==0) expectedResult=3;
            else {
                events.push_back(3);wrote=true;
                if(throwing==3) expectedCatch=3;
                else expectedResult=wi==0?2:0;
            }
            events.push_back(4);expectedDestroyed=1;
            for(int i=0;i<8;++i) {
                unsigned char human=expected.m_isHumanSeat[i]?1:static_cast<Flag>((seed+i)%3);
                if(context.m_seenFlags[i]!=human || context.m_seenTowns[i]!=expected.m_townType[i]) return false;
            }
        }
        if(caught!=expectedCatch || result!=expectedResult || context.m_events!=events
            || context.m_alive!=0 || context.m_destroyed!=expectedDestroyed
            || context.m_progress!=&progress || context.m_file!=(wrote?&file:0)
            || !std::equal(context.m_args,context.m_args+10,expectedArgs) || !equalRequest(request,expected)) return false;
    }
    return true;
}
'''
        for index, (body, name, input_type, output_type, helper_declaration, helper_body) in enumerate(forms):
            request=declaration.replace("unsigned char m_isHumanSeat[8]",input_type+" m_isHumanSeat[8]")
            text += "namespace N%d {\n" % index + "typedef "+output_type+" FixtureFlag;\nstruct type_random_map_generator : GeneratorFixture<FixtureFlag> {\n    using GeneratorFixture<FixtureFlag>::GeneratorFixture;\n" + helper_declaration + "};\n" + request + "\n" + constructor + "\n" + helper_body + body + "\n}\n"
        text += "int main() {\n"
        for index, (body, name, input_type, output_type, helper_declaration, helper_body) in enumerate(forms):
            text += 'if(check<N%d::TRandomMapRequest,N%d::FixtureFlag>() != %s) { std::fprintf(stderr,"failed %s\\n"); return 1; }\n' % (index, index, "true" if index < positive else "false", name)
        text += 'std::printf("%d request forms; 126720 scenarios each; %d negative controls rejected\\n");}\n' % (positive,negative_count)
        with tempfile.TemporaryDirectory(prefix="homm3-request-worker-") as folder:
            unit = Path(folder) / "request.cpp"
            unit.write_text(text)
            binary = Path(folder) / "request"
            subprocess.run(["g++", "-std=c++11", "-O2", "-fno-elide-constructors", str(unit), "-o", str(binary)], check=True, capture_output=True, text=True)
            subprocess.run([str(binary)], check=True)


if __name__ == "__main__":
    unittest.main()
