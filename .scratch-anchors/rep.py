import json, sys
r = json.load(open("build/objdiff/report.json"))
pat = sys.argv[1]
for u in r["units"]:
    if pat in u["name"]:
        for f in u.get("functions", []):
            print("%8.2f  %s" % (f.get("fuzzy_match_percent", 0.0), f.get("name")))
