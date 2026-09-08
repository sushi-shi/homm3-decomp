// seerhuttext.h - Complete text-table layouts and helper declarations.
// The helper source filename and layout member names are provisional;
// neighboring retail RVAs alone do not prove a separate compiland.
#ifndef HOMM3_SEERHUTTEXT_H
#define HOMM3_SEERHUTTEXT_H

#include <string>
#include <vector>

#include <va.h>

class TSpreadsheetResource;

// One quest type's five text variants. PROVEN by the loader at 0x56c120:
// its inner statement run writes five consecutive 0x10-byte basic_strings
// and the outer induction advances the destination by exactly 0x50. They are
// five SEPARATE members and not an array: the compiler-generated constructor
// at 0x56bed0 and destructor at 0x56bf30 unroll all five in place, where an
// array member routes through the ??_L / ??_M vector iterators - which is
// exactly what the enclosing record's own pair does for its ten quests. The
// ordinal names are placeholders; only the count and the order are proven.
struct TSeerHutQuestText {
    // Before normalization: text0.
    std::string m_text0;
    // Before normalization: text1.
    std::string m_text1;
    // Before normalization: text2.
    std::string m_text2;
    // Before normalization: text3.
    std::string m_text3;
    // Before normalization: text4.
    std::string m_text4;
};
SIZE(TSeerHutQuestText, 0x50);

// One seerhut.txt column. PROVEN by the same loader: the quest block runs
// from +0x00 to +0x320 in nine 0x50-byte steps whose FIRST destination is
// +0x50, so index 0 is the reserved no-quest slot and the nine written
// entries are quest types 1..9; the two trailing strings are the row-1 and
// row-47 cells, which the loader writes before and after the block.
struct TSeerHutTextColumn {
    // Before normalization: quest.
    TSeerHutQuestText m_quest[10];  // +0x000, quest[0] never written
    // Before normalization: name.
    std::string m_name;             // +0x320, spreadsheet row 1
    // Before normalization: completion.
    std::string m_completion;       // +0x330, spreadsheet row 47
};
SIZE(TSeerHutTextColumn, 0x340);

// The two three-column tables the loader fills, and the name list the same
// entry point appends to. Their extents are byte-proven - they sit flush at
// 0x69e728, 0x69f0e8 and 0x69faa8, exactly 3 * 0x340 apart - and the column
// mapping is proven by the loader's own call pair, which feeds spreadsheet
// columns 1..3 to the SECOND table and 4..6 to the first. The A/B spelling
// is a house ordinal placeholder; the roles are proven, the names are not
// attested anywhere.
// Before normalization: gSeerHutTextA.
// Before normalization: gSeerHutTextB.
DATA(0x0069e728) extern TSeerHutTextColumn g_seerHutTextA[3];
// Before normalization: gSeerHutNames.
DATA(0x0069f0e8) extern TSeerHutTextColumn g_seerHutTextB[3];
DATA(0x0069faa8) extern std::vector<std::string> g_seerHutNames;

// Retail 0x56c120. Free fastcall under /Gr: the sheet arrives in ECX, the
// destination column record in EDX and the spreadsheet column index on the
// stack (`ret 4`).
// Before normalization (function): LoadSeerHutTextColumn.
void loadSeerHutTextColumn(TSpreadsheetResource* sheet,
                           TSeerHutTextColumn* column, int col);

// Retail 0x56c960. Join a string vector into one localized list. Free
// fastcall under /Gr with a by-value return, so the hidden result pointer
// takes ECX and the vector reference EDX.
// Before normalization (function): JoinTextList.
std::string joinTextList(const std::vector<std::string>& items);

#endif /* HOMM3_SEERHUTTEXT_H */
