// seerhuttext.cpp - provisional owner for Complete's text-table helpers.
// Retail bodies prove these layouts and helpers; the original source filename
// is unresolved. InitializeSeerHutText belongs to seerhut.cpp (DC line 50),
// so the neighboring RVA range does not establish a separate compiland.
#include "va.h"

#include <string>
#include <vector>

#include "seerhuttext.h"

#include "resourcemanager.h"
#include "textresource.h"

VA_COMPGEN(0x0056bde0, 0x5A, CLASS_CTOR, TSeerHutTextColumn)

VA_COMPGEN(0x0056be40, 0x8A, IMPLICIT_DTOR, TSeerHutTextColumn)

VA_COMPGEN(0x0056bed0, 0x56, CLASS_CTOR, TSeerHutQuestText)

VA_COMPGEN(0x0056bf30, 0xF4, IMPLICIT_DTOR, TSeerHutQuestText)

DATA(0x0069e728) TSeerHutTextColumn g_seerHutTextA[3];
DATA(0x0069f0e8) TSeerHutTextColumn g_seerHutTextB[3];
// Initial contents recovered from the pinned Complete image.
// The quest readers use 52 contiguous strings per column; the loader's
// aggregate owns their construction. Retail cells point to B and A, in that order.
DATA(0x0068320c) const TSeerHutTextColumn* g_questTextA = g_seerHutTextB;
DATA(0x00683210) const TSeerHutTextColumn* g_questTextB = g_seerHutTextA;

DATA(0x0069faa8) std::vector<std::string> g_seerHutNames;

// Copy one seerhut.txt column into one TSeerHutTextColumn.

// Row 1 goes into `name` before the loop, then nine groups of five rows
// (2..46) into quest[1..9].text[0..4], then row 47 into `completion`; the
// quest stride is 0x50 and the record 0x340 bytes. The five inner statements
// are one source statement each.
//
// The tables above are defined before the loader, as in the original order
// (docs/vc6/phase-flag.md).
VA(0x0056c120, 0x2A3)
MAC_ADDRESS(0x2543f0, 0x1fc)  // anchor-string(seerhut.txt caller 0x56c3e0) + anchor-callee(basic_string::assign) + retail-only
void loadSeerHutTextColumn(TSpreadsheetResource* sheet,
                           TSeerHutTextColumn* column, int col)
{
    column->m_name = sheet->GetRow(1)[col];

    for (int q = 1; q < 10; ++q) {
        column->m_quest[q].m_text0 = sheet->GetRow(5 * q - 3)[col];
        column->m_quest[q].m_text1 = sheet->GetRow(5 * q - 2)[col];
        column->m_quest[q].m_text2 = sheet->GetRow(5 * q - 1)[col];
        column->m_quest[q].m_text3 = sheet->GetRow(5 * q)[col];
        column->m_quest[q].m_text4 = sheet->GetRow(5 * q + 1)[col];
    }

    column->m_completion = sheet->GetRow(47)[col];
}

// Two `+=` statements in an if/else; the return is the ordinary copy
// construction of the accumulator.
VA(0x0056c960, 0x216)
MAC_ADDRESS(0x163f0c, 0xe8)
std::string joinTextList(const std::vector<std::string>& items)
{
    std::string result;

    for (int i = 0; i < items.size(); ++i) {
        if (i > 0) {
            if (i == items.size() - 1)
                result += g_generalText->GetText(GENERAL_TEXT_LIST_AND);
            else
                result += DATA_COMPGEN(0x0066032c, seerHutListSeparator, ", ");
        }
        result += items[i];
    }

    return result;
}
