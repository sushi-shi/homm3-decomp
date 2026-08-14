// ai_spellvalue.h - the AI's per-hero spell valuer.
// HAND-OWNED. The Dreamcast roster gives this class its own header of
// exactly this name (`E:\gamedcs\ai_spellvalue.h`, cited by
// set_stack_value at ai.cpp:2457, can_cast_spells / set_power at
// ai_player.cpp:1756/1763 and get_mana / set_mana at philai.cpp:914/921),
// so the type lives here rather than in any one consumer's header.
#ifndef HOMM3_AI_SPELLVALUE_H
#define HOMM3_AI_SPELLVALUE_H

#include <va.h>
#include <vector>
// The value list's element. The Dreamcast puts it in its own
// ai_creature_value.h; this tree currently models it in ai_player.h,
// which is where the include comes from until someone splits it out.
#include "ai_player.h"

class hero;

// Layout is FRAME-PROVEN, not guessed: combatManager::has_ranged_advantage
// (0x420a80) builds one as a local at [ebp-0x50] under a `sub esp,0x44`
// frame whose other ten dwords are all accounted for, which fixes
// sizeof at exactly 0x24. Two members are byte-proven inside it - the
// dword at +0x04 that the caller writes with the side's total combat
// value before asking for a spell value (the DC roster's
// set_stack_value, ai_spellvalue.h:124, whose 4-byte body is that one
// store), and the std::vector at +0x14, proven by the compiler-generated
// destructor the caller INLINES: `operator delete(+0x18)` followed by
// zero stores to +0x18/+0x1c/+0x20, i.e. a Dinkumware vector's
// allocator/_First/_Last/_End at +0x14..+0x23. fill_creature_value_list
// (dc 0x10fc6c) is what fills it, which is what types the element.
// The three remaining dwords are unread by any decoded body; the DC
// roster's get_mana/set_mana and set_power say what two of them are for
// but not which.
class type_spellvalue {
public:
    char pad_00[0x4];
    long stack_value;                                  // +0x04
    char pad_08[0xc];
    std::vector<type_creature_value> creature_values;  // +0x14

    // 0x526d40 in philai.obj (RETAIL_LOCATED there); has_ranged_advantage
    // is a located caller and pushes the side's hero.
    type_spellvalue(const hero* new_hero);
    // 0x5275b0, philai.obj. `bits` is a spell-category mask.
    long get_best_spell_value(long bits);
    // DC ai_spellvalue.h:124 - the one-store setter, inlined at every
    // retail call site (dc 0x27c74 is the 4-byte out-of-line copy).
    void set_stack_value(long arg) { stack_value = arg; }
};
SIZE(type_spellvalue, 0x24);

#endif /* HOMM3_AI_SPELLVALUE_H */
