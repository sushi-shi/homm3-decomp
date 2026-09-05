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

// The registry is a function-local static of an INLINE ACCESSOR, not of
// either consumer, and retail's guard bytes prove it: GetImageName (0x514960)
// and setImageName (0x514610) each test 0x69cb64 with mask 1 for the same
// object, while GetImageName's own empty-name static gets a SECOND byte
// (0x69cb70), also with mask 1. Two statics declared in one body share a
// single guard byte with masks 1 and 2, which is what a shared accessor rules
// out. NAME PROVISIONAL - nothing attests it; only the guard-byte layout and
// the shared 0x69cb80 object are retail-proven.
inline TObjectImageNameTable& GetObjectImageNames()
{
    static TObjectImageNameTable imageNames;
    return imageNames;
}


// The per-row parser TObjectTypeTable::load runs over each objects.txt
// line, retail 0x514b80. Free and therefore __fastcall under /Gr: the
// stream arrives in ECX and the record in EDX, and it answers the stream
// so the caller can chain.
std::istream& operator>>(std::istream& is, TObjectType& objectType);

#endif  /* HOMM3_OBJECTTYPE_H */
