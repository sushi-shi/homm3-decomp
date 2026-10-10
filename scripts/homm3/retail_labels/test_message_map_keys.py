"""A VA on BEGIN_MESSAGE_MAP names the class's GetMessageMap, anonymous or not."""

import tempfile
import unittest
from pathlib import Path
from unittest import mock

from homm3.retail_labels import source


class MessageMapKeysTest(unittest.TestCase):
    def test_message_maps_bind_by_their_class(self):
        declarations = (
            "BEGIN_MESSAGE_MAP(TNamedDlg, CDialog)",
            "BEGIN_MESSAGE_MAP( TAnonymousDlg , CDialog )",
        )
        symbols = (
            "?GetMessageMap@TNamedDlg@@MBEPBUAFX_MSGMAP@@XZ",
            "?GetMessageMap@TAnonymousDlg@?%C:\\Dev\\Editor\\Dlg.cpp1234@@"
            "MBEPBUAFX_MSGMAP@@XZ",
        )
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "dialogs.cpp"
            path.write_text("\n".join(
                f"VA(0x{0x401000 + i * 0x20:08x}, 0x6)\n{decl}\nEND_MESSAGE_MAP()"
                for i, decl in enumerate(declarations)))
            rows = source.scan_file(path, {0x1000, 0x1020})
        self.assertEqual([row["name"] for row in rows],
                         ["TNamedDlg_GetMessageMap", "TAnonymousDlg_GetMessageMap"])
        groups = {source._demangle_key(symbol): [(symbol, 0x6)]
                  for symbol in symbols}
        with mock.patch.object(source, "_base_authority_scan",
                               return_value=(groups, {})):
            source.join_unit("dialogs", rows)
        self.assertEqual(rows[0].get("joined"), symbols[0])

    def test_ir_names_an_anonymous_class_message_map(self):
        ir = "\n".join((
            'define internal x86_thiscallcc noundef ptr '
            '@"?GetMessageMap@TAnonymousDlg@?A0xE345B8E0@@MBEPBUAFX_MSGMAP@@XZ"(ptr %0) {',
            'define dso_local x86_thiscallcc noundef ptr '
            '@"?GetMessageMap@TNamedDlg@@MBEPBUAFX_MSGMAP@@XZ"(ptr %0) {',
        ))
        text = ("VA(0x00480112, 0x6)\nBEGIN_MESSAGE_MAP(TAnonymousDlg, CDialog)\n"
                "END_MESSAGE_MAP()\n\nVA(0x004801ff, 0x6)  // note\n"
                "BEGIN_MESSAGE_MAP(TMissingDlg, CDialog)\nEND_MESSAGE_MAP()\n")
        self.assertEqual(source.ir_message_map_names(ir, text), {
            0x80112: "?GetMessageMap@TAnonymousDlg@?A0xE345B8E0@@MBEPBUAFX_MSGMAP@@XZ"})


if __name__ == "__main__":
    unittest.main()
