// rmg.cpp - Complete-only random-map generator support.
//
// The Dreamcast build has no RMG compiland. Retail's direct caller graph
// reaches this library from TSingleSelectionWindow::GenerateRandomMap, and
// the tree node layout proves an eight-byte TPoint value ordered by y, then x.
#include <va.h>
#define _MT
#include <yvals.h>
#undef _MT
#include <set>
#include "rmg.h"

typedef std::set<TPoint> TRmgPointSet;

// Complete-only helper called by InitializeObjectGenerators at 0x538b10.
// The four argument loads, five stores, vtable relocation, and `ret 0x10`
// independently prove this constructor and the shared 0x14-byte prefix.
VA(0x00534160, 0x27)
RMGObjectGenerator::RMGObjectGenerator(
    int newObjectType, int newSubtype, int newValue, int newDensity)
{
    objectType = newObjectType;
    subtype = newSubtype;
    value = newValue;
    density = newDensity;
}

// Retail 0x5b8bc0 is the nine-block Dinkumware tree-successor walk. Its
// sentinel at 0x6a52c4 is shared only by the RMG set cluster, while the
// enclosing callers lead back to GenerateRandomMap. Dreamcast's STLport
// _M_increment at dc 0x64214 corroborates the helper boundary and CFG shape;
// the RMG type itself is retail-only.
VA_COMPGEN(0x005B8BC0, 0xA3, TREE_CONST_ITERATOR_INC, TPoint)

// Minimum ODR use needed to retain the real VC6/Dinkumware COMDAT. This
// wrapper is not a retail claim and adds no target/report row.
void __fastcall EmitRmgPointSetIncrement(TRmgPointSet::const_iterator* it)
{
    ++*it;
}
