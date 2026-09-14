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

VA_COMPGEN(0x0056bde0, 0x5A, CLASS_CTOR, TSeerHutTextColumn)
VA_COMPGEN(0x0056be40, 0x8A, IMPLICIT_DTOR, TSeerHutTextColumn)
VA_COMPGEN(0x0056bed0, 0x56, CLASS_CTOR, TSeerHutQuestText)
VA_COMPGEN(0x0056bf30, 0xF4, IMPLICIT_DTOR, TSeerHutQuestText)

// Retail 0x56c120. Copy one seerhut.txt column into one TSeerHutTextColumn.

// The row map is read straight off the body: row 1 into `name` before the
// loop, then nine groups of five rows (2..46) into quest[1..9].text[0..4],
// then row 47 into `completion`. The two induction variables retail keeps
// live are the row byte offset (0x14 per step, tested `cmp esi, 0xc8`, i.e.
// the strength-reduced `q < 10`) and the destination pointer (0x50 per step),
// which is what proves the 0x50 quest stride and the 0x340 record extent.

// The five inner statements are one source statement each: VC6 CALLS
// basic_string::assign at the first two sites and EXPANDS it at the other
// three, which is an /Ob2 budget outcome and not a spelling difference.

// Residual (99.9587%): ONE encoder tie-break. All 24 blocks, all 13 branches
// and all 3 returns agree; the only divergence is the fourth inlined
// basic_string::_Eos terminator, where retail encodes `mov byte ptr
// [ecx + eax], 0` and we encode `mov byte ptr [eax + ecx], 0` - the same
// instruction with the SIB base and index exchanged. The other three
// expansions of the same statement already agree. The current divergent
// block is B8, within text4 assignment; source labels do not recover the
// original expression. Nine row/record lifetime variants emit six objects,
// all reproduced: direct row expressions remain 99.9621 with an indexed,
// reference or pointer destination; a local row base or paired induction
// drops to 89.6136..89.7917. All five scored siblings remain exact.
// The recovered spreadsheet cell accessor is not interchangeable with this
// caller's row-access model: replacing all seven getRow(row)[column] sites
// with getSpreadsheet(row,column) scores53.0833 and changes the call stream.
// The four-state caller/sibling family reproduces all four objects; changing
// initializeSeerHutText alone also loses its exact body (99.7101). No
// Dreamcast counterpart proves either replacement in this Complete-only TU.
// Sixty further cell/endpoint lifetime states produce six objects, all
// reproduced. Actual char-pointer, destination-reference and row-reference
// bindings preserve the seven reads/assignments, but none improves 99.9621%.
// Shared input-pointer lifetime falls to 99.8712%; binding both endpoint
// destinations reaches 98.7121% (98.6212% combined). All five siblings stay
// exact. These meaningful local bindings do not explain the SIB choice.
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

// Both separator arms expand basic_string::append in full and the
// cross-jumper merges their copy tails, which is what two `+=` statements in
// an if/else produce; the return is the ordinary copy construction of the
// accumulator, `_Tidy()` plus `assign(result, 0, npos)`.

VA(0x0056c960, 0x216)
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
