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
