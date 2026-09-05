// seerhuttext.h - prototypes of src/seerhuttext.cpp, the Complete-only
// compiland that links between search.obj (ends 0x16bd2c) and seerhut.obj
// (starts 0x16d3e0). It has NO Dreamcast counterpart, so every name below is
// PROVISIONAL and describes the retail body that fixes it.
#ifndef HOMM3_SEERHUTTEXT_H
#define HOMM3_SEERHUTTEXT_H

#include <string>

#include <va.h>

class TSpreadsheetResource;

// One quest type's five text variants. PROVEN by the loader at 0x56c120:
// its inner statement run writes five consecutive 0x10-byte basic_strings
// and the outer induction advances the destination by exactly 0x50.
struct TSeerHutQuestText {
    std::string text[5];
};
SIZE(TSeerHutQuestText, 0x50);

// One seerhut.txt column. PROVEN by the same loader: the quest block runs
// from +0x00 to +0x320 in nine 0x50-byte steps whose FIRST destination is
// +0x50, so index 0 is the reserved no-quest slot and the nine written
// entries are quest types 1..9; the two trailing strings are the row-1 and
// row-47 cells, which the loader writes before and after the block.
struct TSeerHutTextColumn {
    TSeerHutQuestText quest[10];  // +0x000, quest[0] never written
    std::string name;             // +0x320, spreadsheet row 1
    std::string completion;       // +0x330, spreadsheet row 47
};
SIZE(TSeerHutTextColumn, 0x340);

// Retail 0x56c120. Free fastcall under /Gr: the sheet arrives in ECX, the
// destination column record in EDX and the spreadsheet column index on the
// stack (`ret 4`).
void LoadSeerHutTextColumn(TSpreadsheetResource* sheet,
                           TSeerHutTextColumn* column, int col);

#endif /* HOMM3_SEERHUTTEXT_H */
