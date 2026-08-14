import subprocess, sys, json, re
ROOT="/home/sheep/Projects/homm3/wt-cold-windows"
ORIG=open(ROOT+"/.scratch-srclines/ai_combat.cpp.orig").read()
ANCHOR="""    float attacker = static_cast<float>(get_final_melee_value());"""
def build(n):
    pad = "".join(f"    int pad{i} = {i}; pad{i} = pad{i};\n" for i in range(n))
    s = ORIG.replace(ANCHOR, pad + ANCHOR, 1)
    open(ROOT+"/src/ai_combat.cpp","w").write(s)
    subprocess.run(["nix","develop",".#build","--command","bash","-c",
                    "homm3 build --fast >/dev/null 2>&1"],cwd=ROOT)
    d=json.load(open(ROOT+"/build/objdiff/report.json"))
    for u in d["units"]:
        for f in u.get("functions",[]):
            if "do_general_melee" in f.get("name",""):
                return f.get("fuzzy_match_percent",0.0)
    return -1
for n in range(0,int(sys.argv[1])+1):
    print(f"  mass {n:3d}  {build(n):9.4f}", flush=True)
