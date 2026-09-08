// seerhuttext.cpp - provisional owner for Complete's text-table helpers.
// Retail bodies prove these layouts and helpers; the original source filename
// is unresolved. InitializeSeerHutText belongs to seerhut.cpp (DC line 50),
// so the neighboring RVA range does not establish a separate compiland.
#include <string>
#include <vector>

#include <va.h>
#include "resourcemanager.h"
#include "seerhuttext.h"
#include "textresource.h"

// The four compiler-generated special members the two table definitions
// below instantiate. They are the strongest independent proof of both
// layouts: 0x56bde0 hands ??_L (the vector constructor iterator) the triple
// (stride 0x50, count 0xa, element ctor 0x56bed0) before touching the two
// trailing basic_strings at +0x320 and +0x330, and 0x56bed0 zero-fills five
// consecutive 0x10-byte strings. The destructors run the same shapes in
// reverse, starting at +0x334 and +0x44 - the last string's _Ptr in each.
VA_COMPGEN(0x0056bde0, 0x5A, CLASS_CTOR, TSeerHutTextColumn)
VA_COMPGEN(0x0056be40, 0x8A, IMPLICIT_DTOR, TSeerHutTextColumn)
VA_COMPGEN(0x0056bed0, 0x56, CLASS_CTOR, TSeerHutQuestText)
VA_COMPGEN(0x0056bf30, 0xF4, IMPLICIT_DTOR, TSeerHutQuestText)

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
void loadSeerHutTextColumn(TSpreadsheetResource* sheet,
                           TSeerHutTextColumn* column, int col)
{
    column->m_name = sheet->getRow(1)[col];

    for (int q = 1; q < 10; ++q) {
        column->m_quest[q].m_text0 = sheet->getRow(5 * q - 3)[col];
        column->m_quest[q].m_text1 = sheet->getRow(5 * q - 2)[col];
        column->m_quest[q].m_text2 = sheet->getRow(5 * q - 1)[col];
        column->m_quest[q].m_text3 = sheet->getRow(5 * q)[col];
        column->m_quest[q].m_text4 = sheet->getRow(5 * q + 1)[col];
    }

    column->m_completion = sheet->getRow(47)[col];
}

DATA(0x0069e728) TSeerHutTextColumn g_seerHutTextA[3];
DATA(0x0069f0e8) TSeerHutTextColumn g_seerHutTextB[3];
DATA(0x0069faa8) std::vector<std::string> g_seerHutNames;

// Retail 0x56c960. Join a string vector into one localized list: every entry
// after the first is preceded by ", " except the last, which takes general
// text 142 - the localized final conjunction. The index guard is SIGNED
// (`jle` on the strength-reduced byte offset) while the bound and the
// last-entry test are unsigned, which is exactly an `int i` against
// `items.size()`.
//
// Both separator arms expand basic_string::append in full and the
// cross-jumper merges their copy tails, which is what two `+=` statements in
// an if/else produce; the return is the ordinary copy construction of the
// accumulator, `_Tidy()` plus `assign(result, 0, npos)`.
//
// EXACT on the first spelling: 32 of 32 blocks, 18 of 18 branches, both
// returns.
VA(0x0056c960, 0x216)  // anchor-string(", " 0x66032c) + anchor-callee(basic_string::_Grow/_Eos) + bracket(seerhut.obj caller 0x16dfa0), retail-only
std::string joinTextList(const std::vector<std::string>& items)
{
    std::string result;

    for (int i = 0; i < items.size(); ++i) {
        if (i > 0) {
            if (i == items.size() - 1)
                result += g_generalText->getText(142);
            else
                result += DATA_COMPGEN(0x0066032c, seerHutListSeparator, ", ");
        }
        result += items[i];
    }

    return result;
}
