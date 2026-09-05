// objecttype.h - the Complete-only image-name registry shared by
// TObjectType::GetImageName and TObjectType::setImageName.
//
// Kept out of advmgr_objects.h deliberately: objecttype.cpp is the only
// consumer, and advmgr_objects.h reaches nine compilands through game.h and
// mapcell.h.
#ifndef HOMM3_OBJECTTYPE_H
#define HOMM3_OBJECTTYPE_H

#include <istream>
#include <map>
#include <string>
#include <vector>

#include "advmgr_objects.h"

// Retail publishes this class's whole layout at the .bss object 0x69cb80
// that both accessors address:
//   +0x00  a 16-byte Dinkumware _Tree - allocator byte, comparator byte,
//          _Head at +4, _Multi at +8, _Size at +0x0c - whose constructor
//          allocates 0x24-byte nodes, i.e. a tree header plus
//          pair<const string, int>;
//   +0x10  a 16-byte vector whose elements are FOUR bytes wide.
// The 4-byte element is the map's own ITERATOR: GetImageName reads
// `rows[i]` and adds 0x0c to reach the key string, and setImageName reads
// +0x1c from the same pointer to reach the mapped index - node+0x0c and
// node+0x1c are exactly `->first` and `->second` of that pair. The
// registry's growth path in setImageName confirms it from the other side:
// it inserts into the tree and then push_backs the RETURNED ITERATOR.
//
// NAMES ARE PROVISIONAL - nothing attests this class; only the offsets, the
// node size and the two accessors' arithmetic are retail-proven.
class TObjectImageNameTable {
public:
    typedef std::map<std::string, int> TNameIndex;

    TNameIndex nameIndex;
    std::vector<TNameIndex::iterator> rows;
};


// The per-row parser TObjectTypeTable::load runs over each objects.txt
// line, retail 0x514b80. Free and therefore __fastcall under /Gr: the
// stream arrives in ECX and the record in EDX, and it answers the stream
// so the caller can chain.
std::istream& operator>>(std::istream& is, TObjectType& objectType);

#endif  /* HOMM3_OBJECTTYPE_H */
