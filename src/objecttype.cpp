// objecttype.cpp - Complete object-template and image-name registry support.
//
// This compiland is absent from the Dreamcast roster. Retail groups
// TObjectType::setImageName, TObjectTypeTable::load and the global image-name
// registry here; the registry's tree nodes hold a VC6 std::string at +0x0c.
#include <va.h>
#define _MT
#include <yvals.h>
#undef _MT
#include <set>
#include <string>

typedef std::set<std::string> TObjectImageNameSet;

// Retail 0x517780 is the nine-block Dinkumware tree-successor walk. Its node
// teardown calls string::_Tidy for the value at +0x0c, frees a 0x24-byte
// node, and reads the tree color at +0x20, proving this specialization.
// Dreamcast's generic STLport _M_increment at dc 0x64214 independently
// corroborates the source helper boundary and its nine-block control flow.
VA_COMPGEN(0x00517780, 0xA3, TREE_CONST_ITERATOR_INC, string)

// Minimum ODR use needed to retain the real VC6/Dinkumware COMDAT. This
// wrapper is not a retail claim and adds no target/report row.
void __fastcall EmitObjectImageNameSetIncrement(
    TObjectImageNameSet::const_iterator* it)
{
    ++*it;
}
