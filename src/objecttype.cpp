// objecttype.cpp - Complete object-template and image-name registry support.
//
// This compiland is absent from the Dreamcast roster. Retail groups
// TObjectType::setImageName, TObjectTypeTable::load and the global image-name
// registry here; the registry's tree nodes hold a VC6 std::string at +0x0c.
#include <va.h>
#define _MT
#include <yvals.h>
#undef _MT
#include <map>
#include <string>
#include <vector>

#include "advmgr_objects.h"
#include "objecttype.h"

// The registry's implicit default constructor, emitted as its own COMDAT:
// the _Tree constructor (shared-nil refcount at 0x69cba0, nil node at
// 0x69cba4, _Lockit around the head-node purchase) followed by the four
// zero stores of the vector at +0x10. GetImageName's function-local static
// and setImageName's are the same object, so both initialize through this.
VA_COMPGEN(0x00514060, 0xCA, CLASS_CTOR, TObjectImageNameTable)

// Retail 0x514960. Two function-local statics with independent guard bytes -
// the empty string at 0x69cb48 (guard 0x69cb70) and the registry at
// 0x69cb80 (guard 0x69cb64, atexit 0x514050) - then an UNSIGNED bound test
// (`cmp ecx,eax` / `jae`) of the int member against the vector's size, which
// retail computes inline from _First/_Last. The in-range arm returns
// `rows[imageNumber]` biased by 0x0c, i.e. the pair's key; the out-of-range
// arm returns the address of the empty string itself.
VA(0x00514960, 0xAD)  // anchor-global 0x69cb80 registry + 0x69cb48 empty name; sole caller CObjectType(TObjectType*), retail-only
const std::basic_string<char, std::char_traits<char>, std::allocator<char> >&
TObjectType::GetImageName()
{
    static std::string emptyImageName;
    static TObjectImageNameTable imageNames;

    if (imageNumber < imageNames.rows.size())
        return imageNames.rows[imageNumber]->first;
    return emptyImageName;
}

// Retail 0x514a60, chained off setImageName's result by the per-row `>>`
// at 0x514b80. The mask arrives by reference and is COPIED before the
// `&=`, which is Dinkumware's `operator&` written out at the call site:
// retail builds `~passableMask` first (copy, two-word flip, 0xffff trim),
// then copies the argument over it and ANDs high word down. `any()` walks
// the same two words downward into the byte at +0x29, and the false arm's
// {8, 6} sentinel comes straight out of memory. The scan's outer loop has
// NO bound in retail - any() has already guaranteed a hit - and its
// `0x2f - y*8 - x` is CObjectType::_getBitPos verbatim, strength-reduced
// onto a second induction variable in the dead parameter home.
//
// The sentinel arm issues BOTH loads before either store, which two plain
// assignments do not produce (96.26%); naming the two halves first does
// (99.98%, residual: two unclaimed .rdata reloc names).
VA(0x00514a60, 0x11D)  // anchor-callee 0x514b80 per-row `>>`; anchor-global {8,6} at 0x640278; retail-only
TObjectType& TObjectType::setTriggerMask(const std::bitset<48>& mask)
{
    triggerMask = mask & ~passableMask;
    hasTrigger = triggerMask.any();
    if (hasTrigger) {
        for (int y = 0;; ++y) {
            for (unsigned x = 0; x < 8; ++x) {
                if (triggerMask.test(CObjectType::_getBitPos(x, y))) {
                    triggerCell.x = x;
                    triggerCell.y = y;
                    return *this;
                }
            }
        }
    } else {
        int noTriggerX = gNoTriggerCell.x;
        int noTriggerY = gNoTriggerCell.y;
        triggerCell.x = noTriggerX;
        triggerCell.y = noTriggerY;
    }
    return *this;
}

// Retail 0x517780 is the nine-block Dinkumware tree-successor walk, reached
// from two other bodies in this span (0x516bdb and 0x5170b7). The 0x24-byte
// node the registry's constructor buys is what settles the specialization:
// 36 bytes is a tree header plus pair<const string, int>, so this is the
// image-name MAP's iterator, not a set<string>'s (whose node is 32).
// Dreamcast's generic STLport _M_increment at dc 0x64214 independently
// corroborates the source helper boundary and its nine-block control flow.
VA_COMPGEN(0x00517780, 0xA3, TREE_CONST_ITERATOR_INC, string)

// Minimum ODR use needed to retain the real VC6/Dinkumware COMDAT. This
// wrapper is not a retail claim and adds no target/report row.
void __fastcall EmitObjectImageNameIndexIncrement(
    TObjectImageNameTable::TNameIndex::const_iterator* it)
{
    ++*it;
}
