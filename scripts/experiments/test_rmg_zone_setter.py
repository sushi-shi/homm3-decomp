"""Native value/snapshot/self-alias checks for the actual setter family."""
import subprocess
import tempfile
from pathlib import Path

from homm3.vc6.test_rmg_families import generator


def main():
    module = generator("generate-rmg-zone-setter-family.py")
    original = module.definition((module.HOMM3_DIR / "src/rmg.cpp").read_text())
    bodies = [row["replace"] for row in module.variants(original)]
    bad = [original.replace("m_levelPosition = position;", "m_levelPosition.m_x = position.m_x;"),
           original.replace("m_levelPosition = position;", "m_levelPosition = position; m_levelPosition.m_z = 0;"),
           original.replace("m_levelPosition = position;", "m_levelPosition = position; m_levelPosition.m_x = position.m_y;")]
    text = "#include <climits>\n#include <cstdio>\nstruct TRmgMapPosition { int m_x,m_y,m_z; };\n"
    for i, body in enumerate(bodies + bad):
        text += f"struct Case{i} {{ TRmgMapPosition m_levelPosition; " + body.replace("TRmgZone::", "") + " };\n"
    text += r'''
template<class C> bool check() {
    const int values[]={INT_MIN,-1,0,1,INT_MAX};
    for(int x:values) for(int y:values) for(int z:values) {
        C object;
        object.m_levelPosition={17,29,43};
        TRmgMapPosition source={x,y,z};
        object.setLevelPosition(source);
        if(source.m_x!=x || source.m_y!=y || source.m_z!=z) return false;
        if(object.m_levelPosition.m_x!=x || object.m_levelPosition.m_y!=y || object.m_levelPosition.m_z!=z) return false;
        source={53,59,61};
        if(object.m_levelPosition.m_x!=x || object.m_levelPosition.m_y!=y || object.m_levelPosition.m_z!=z) return false;
        object.setLevelPosition(object.m_levelPosition);
        if(object.m_levelPosition.m_x!=x || object.m_levelPosition.m_y!=y || object.m_levelPosition.m_z!=z) return false;
        object.setLevelPosition(TRmgMapPosition{x,y,z});
        if(object.m_levelPosition.m_x!=x || object.m_levelPosition.m_y!=y || object.m_levelPosition.m_z!=z) return false;
    }
    return true;
}
int main() {
'''
    for i in range(len(bodies) + len(bad)):
        text += f'if(check<Case{i}>() != {str(i < len(bodies)).lower()}) {{ std::printf("case {i} failed\\n"); return 1; }}\n'
    text += "}\n"
    with tempfile.TemporaryDirectory(prefix="rmg-zone-setter-") as folder:
        cpp, binary = Path(folder) / "oracle.cpp", Path(folder) / "oracle"
        cpp.write_text(text)
        subprocess.run(["g++", "-std=c++11", "-O1", str(cpp), "-o", str(binary)], check=True)
        subprocess.run([str(binary)], check=True)
    print("21 actual setters × 125 values; lvalue, temporary and self-alias checks; 3 negative controls rejected")


if __name__ == "__main__":
    main()
