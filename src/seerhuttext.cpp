// seerhuttext.cpp - the Complete-only compiland between search.obj and
// seerhut.obj (0x16bd30..0x16d3e0). No Dreamcast roster covers it, so the
// compiland's real name and every identifier below are PROVISIONAL; the
// retail bodies are the only evidence.
#include <string>
#include <vector>

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

DATA(0x0069e728) TSeerHutTextColumn gSeerHutTextA[3];
DATA(0x0069f0e8) TSeerHutTextColumn gSeerHutTextB[3];
DATA(0x0069faa8) std::vector<std::string> gSeerHutNames;

// Retail 0x56c3e0. The compiland's entry point: pull seerhut.txt out of the
// resource cache, fill both three-column tables from it, then walk every row
// from 50 to the end and append each non-blank first cell to the name list.
//
// Two guards, and only the first disposes nothing either: neither the missing
// spreadsheet nor the short one releases the resource, where the success tail
// does. The row count is re-read on EVERY iteration of the name walk - retail
// reloads _First and _Last at the loop head rather than caching a bound.
//
// Residual (79.88%): a three-way register permutation and one inline
// decision, both downstream of where `sheet` lands. Retail keeps it in EBX -
// pushed alone at entry, with ESI/EDI pushed later, inside the region past
// both guards - and never spills it; our compile puts it in EDI, pushes EDI
// at entry, and homes it at [ebp-0x18], which is the whole 4-byte frame
// surplus (0x1c against retail's 0x18). Every block is otherwise
// instruction-for-instruction retail with the three callee-saved registers
// rotated (sheet EBX->EDI, the column counter ESI->ESI, the record offset
// EDI->EBX). The one call-stream divergence rides along: retail CALLS
// basic_string::assign for the appended name and we expand it down to
// _Grow.
// Measured and byte-flat: hoisting both loop counters to function scope
// ahead of `sheet`, and writing the blank-cell guard positively instead of
// `continue`. Measured and worse: a named `std::string entry = name;` local
// (77.62) and spelling the append as `insert(end(), 1, name)` (23.68), which
// is what retail's out-of-line callee is but not what its source wrote.
VA(0x0056c3e0, 0x183)  // anchor-string(seerhut.txt 0x683214) + anchor-callee(LoadSeerHutTextColumn 0x56c120) + retail-only
unsigned char InitializeSeerHutText()
{
    TSpreadsheetResource* sheet = ResourceManager::GetSpreadsheet(
        DATA_COMPGEN(0x00683214, seerHutSpreadsheetName, "seerhut.txt"));
    if (!sheet)
        return 0;

    if (sheet->GetNumberOfRows() < 60)
        return 0;

    for (int c = 0; c < 3; ++c) {
        LoadSeerHutTextColumn(sheet, &gSeerHutTextB[c], c + 1);
        LoadSeerHutTextColumn(sheet, &gSeerHutTextA[c], c + 4);
    }

    for (int row = 50; row < sheet->GetNumberOfRows(); ++row) {
        const char* name = sheet->GetRow(row)[0];
        if (!name[0] || name[0] == ' ')
            continue;
        gSeerHutNames.push_back(name);
    }

    sheet->Dispose();
    return 1;
}
