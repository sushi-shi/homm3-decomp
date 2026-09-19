"""Check value and reference identity of the canonical clamp syntax family."""
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

from homm3.vc6.test_rmg_families import generator


class ClampStructures(unittest.TestCase):
    @unittest.skipUnless(shutil.which("g++"), "native oracle needs g++")
    def test_values_and_selected_operand(self):
        module = generator("generate-rmg-clamp-structure-family.py")
        forms = [row["replace"] for row in module.variants()]
        self.assertEqual(len(set(forms)), 12)
        # Negative controls change the selected argument at each boundary,
        # or swap lower/upper priority when the bounds are inverted.
        wrong = [forms[0].replace("value < minimum", "value <= minimum"),
                 forms[0].replace("maximum < value", "maximum <= value"),
                 forms[0].replace("return maximum;", "return value;"),
                 forms[0].replace("value < minimum", "maximum < value", 1)
                         .replace("return minimum;", "return maximum;", 1)]
        text = "#include <climits>\n"
        for i, body in enumerate(forms + wrong):
            text += f"namespace Case{i} {{\n" + body + r"""
bool check() {
    const int values[] = {INT_MIN, -4, -1, 0, 1, 4, INT_MAX};
    for (int a = 0; a < 7; ++a) for (int b = 0; b < 7; ++b)
    for (int c = 0; c < 7; ++c) {
        int storage[3] = {values[a], values[b], values[c]};
        // All index triples include aliasing and disjoint operands.
        for (int l = 0; l < 3; ++l) for (int v = 0; v < 3; ++v)
        for (int h = 0; h < 3; ++h) {
            const int* expected = &storage[v];
            if (storage[v] < storage[l]) expected = &storage[l];
            else if (storage[v] > storage[h]) expected = &storage[h];
            const int& result = tLimit(storage[l], storage[v], storage[h]);
            if (&result != expected || result != *expected) return false;
            if (storage[0] != values[a] || storage[1] != values[b]
                || storage[2] != values[c]) return false;
        }
    }
    return true;
}
}
"""
        text += "int main() {\n"
        for i in range(len(forms)):
            text += f"if (!Case{i}::check()) return 1;\n"
        for i in range(len(forms), len(forms) + len(wrong)):
            text += f"if (Case{i}::check()) return 2;\n"
        text += "return 0;\n}\n"
        with tempfile.TemporaryDirectory(prefix="rmg-clamp-") as raw:
            path = Path(raw)
            source, binary = path / "clamp.cpp", path / "clamp"
            source.write_text(text)
            subprocess.run([shutil.which("g++"), "-std=c++98", str(source), "-o", str(binary)], check=True)
            subprocess.run([str(binary)], check=True)


if __name__ == "__main__":
    unittest.main()
