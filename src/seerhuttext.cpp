// seerhuttext.cpp - the Complete-only compiland between search.obj and
// seerhut.obj (0x16bd30..0x16d3e0). No Dreamcast roster covers it, so the
// compiland's real name and every identifier below are PROVISIONAL; the
// retail bodies are the only evidence.
#include <string>

#include <va.h>
#include "resourcemanager.h"
#include "seerhuttext.h"
#include "textresource.h"

// Retail 0x56c120. Copy one seerhut.txt column into one TSeerHutTextColumn.
//
// The row map is read straight off the body: row 1 into `name` before the
// loop, then nine groups of five rows (2..46) into quest[1..9].text[0..4],
// then row 47 into `completion`. The two induction variables retail keeps
// live are the row byte offset (0x14 per step, tested `cmp esi, 0xc8`, i.e.
// the strength-reduced `q < 10`) and the destination pointer (0x50 per step),
// which is what proves the 0x50 quest stride and the 0x340 record extent.
//
// The five inner statements are one source statement each: VC6 CALLS
// basic_string::assign at the first two sites and EXPANDS it at the other
// three, which is an /Ob2 budget outcome and not a spelling difference.
//
// Residual (99.9587%): ONE encoder tie-break. All 24 blocks, all 13 branches
// and all 3 returns agree; the only divergence is the fourth inlined
// basic_string::_Eos terminator, where retail encodes `mov byte ptr
// [ecx + eax], 0` and we encode `mov byte ptr [eax + ecx], 0` - the same
// instruction with the SIB base and index exchanged. The other three
// expansions of the same statement already agree, so this is the bounded
// B18 class, not a source fact.
VA(0x0056c120, 0x2A3)  // anchor-string(seerhut.txt caller 0x56c3e0) + anchor-callee(basic_string::assign) + retail-only
void LoadSeerHutTextColumn(TSpreadsheetResource* sheet,
                           TSeerHutTextColumn* column, int col)
{
    column->name = sheet->GetRow(1)[col];

    for (int q = 1; q < 10; ++q) {
        column->quest[q].text[0] = sheet->GetRow(5 * q - 3)[col];
        column->quest[q].text[1] = sheet->GetRow(5 * q - 2)[col];
        column->quest[q].text[2] = sheet->GetRow(5 * q - 1)[col];
        column->quest[q].text[3] = sheet->GetRow(5 * q)[col];
        column->quest[q].text[4] = sheet->GetRow(5 * q + 1)[col];
    }

    column->completion = sheet->GetRow(47)[col];
}
