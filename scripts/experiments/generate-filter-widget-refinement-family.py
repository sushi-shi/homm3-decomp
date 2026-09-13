"""Refine reproduced filter parents through actual widget-pointer conversions.

The retained vector append takes widget* by const reference; passing a
button* creates a converted pointer temporary. Retail's append argument
lives in a stack slot. Compare naming that conversion before or after the
unchanged frame updates, and binding the repeatedly used widget vector.
These locals carry real values. All constructors, direct highlight stores,
canonical disabled-frame calls, append order and public APIs are retained.
"""
import argparse
import json
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[2]
SOURCE = "src/singleselectionwindow.cpp"
SIGNATURE = "void TSingleSelectionWindow::createFilterWidgets()"


def body_from(source):
    start = source.index(SIGNATURE + "\n{")
    return source[start:source.index("\n}", start) + 2]


def make_manifest(checkpoint_path):
    context = checkpoint_path.parent
    snapshot = (context / "snapshot" / SOURCE).read_text()
    current = (ROOT / SOURCE).read_text()
    if snapshot != current:
        raise ValueError("The parent snapshot differs; restore its source checkpoint before refining")
    checkpoint = json.loads(checkpoint_path.read_text())
    options = [{"name": "unchanged"}]
    seen = {body_from(current)}
    for parent in checkpoint["elites"]:
        trial = context / "candidates" / parent["id"]
        repeated = json.loads((trial / "repeat/result.json").read_text())
        if repeated.get("object_hash") != parent.get("object_hash") or repeated["scores"] != parent["scores"]:
            raise ValueError("Parent was not reproduced: " + parent["id"])
        body = body_from((trial / "first/tree" / SOURCE).read_text())
        for conversion in range(3):
            for vector_binding in range(2):
                candidate = body
                if conversion:
                    loops = list(re.finditer(r"^( +)for \(i = 0; i < \d+; \+\+i\) \{\n([\s\S]*?)^\1\}", candidate, re.M))
                    if len(loops) != 6:
                        raise ValueError("Review the six filter loop scopes")
                    for loop in reversed(loops):
                        text = loop.group(0)
                        append = re.search(r"m_widgets\.(?:push_back\(|insert\(m_widgets\.end\(\), (?:1, )?)([^\n]+)\);", text)
                        if not append:
                            raise ValueError("Review the filter append argument")
                        value = append.group(1)
                        start, end = append.span(1)
                        text = text[:start] + "added" + text[end:]
                        marker = "m_highlightedFrame = 2;" if conversion == 1 else "setDisabledFrame(1);"
                        position = text.index(marker)
                        if conversion == 1:
                            position = text.rfind("\n", 0, position) + 1
                        else:
                            position = text.index("\n", position) + 1
                        declaration = loop.group(1) + "    widget* added = " + value + ";\n"
                        text = text[:position] + declaration + text[position:]
                        candidate = candidate[:loop.start()] + text + candidate[loop.end():]
                if vector_binding:
                    candidate = candidate.replace("m_widgets.", "widgets.")
                    candidate = candidate.replace(SIGNATURE + "\n{\n", SIGNATURE + "\n{\n    std::vector<widget*>& widgets = m_widgets;\n", 1)
                if candidate in seen:
                    continue
                seen.add(candidate)
                options.append({"name": parent["id"] + f"-conversion-{conversion}-vector-{vector_binding}", "replace": candidate})
    return {"schema": 1, "source": SOURCE, "units": ["singleselectionwindow"], "evidence": __doc__,
            "parent_context": context.name, "axes": [{"name": "filter-pointer-conversion", "find": body_from(current), "options": options}]}


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("checkpoint", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(make_manifest(args.checkpoint), indent=2) + "\n")
