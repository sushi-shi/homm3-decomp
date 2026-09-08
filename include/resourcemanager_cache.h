#ifndef HOMM3_RESOURCEMANAGER_CACHE_H
#define HOMM3_RESOURCEMANAGER_CACHE_H

#include <map>
#include <va.h>

class resource;
namespace ResourceManager {

// Dreamcast resourcemanager.cpp:121/126 proves the class, constructor and
// ordinary const operator< (original field: name). Retail 0x55ac20 copies
// twelve bytes and terminates byte 12; 0x55ebd0 compares these keys with
// _stricmp. Complete's map node has its key at +0xc and resource* at +0x1c.
class TCacheMapKey {
public:
    char m_name[13];
    TCacheMapKey(const char* name);
    bool operator<(const TCacheMapKey& other) const;
};

typedef std::map<TCacheMapKey, resource*> TCacheMap;
SIZE(TCacheMapKey, 13);
SIZE(TCacheMap, 16);

}

#endif
