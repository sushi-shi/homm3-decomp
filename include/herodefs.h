// herodefs.h - prototypes of herodefs.cpp (compiland herodefs.obj)
#ifndef HOMM3_HERODEFS_H
#define HOMM3_HERODEFS_H

#include "sskilltraits.h"

namespace {

// Source-private owner used by all three retail trait-table loaders.
class TAutoStrPtr {
public:
    TAutoStrPtr() : m_str(0) {}
    ~TAutoStrPtr() { delete[] m_str; }
    void set(char* value) { m_str = value; }
    char* get() const { return m_str; }

private:
    char* m_str;
};

}

unsigned char initializeHeroTraitsTable();
unsigned char initializeHeroClassTraitsTable();
unsigned char initializeSSkillTraitsTable();

// --- globals ---
// CODEVIEW(E:\gamedcs\herodefs.cpp:409, dc 0xd5bc0) void InitializeHeroTraits(int id, const std::vector<char* resource);
// CODEVIEW(E:\gamedcs\herodefs.cpp:441, dc 0xd5d28) void InitializeHeroClassTraits(int id, const std::vector<char* resource);
// CODEVIEW(E:\gamedcs\herodefs.cpp:489, dc 0xd5ee8) void InitializeSSkillTraits(int id, const std::vector<char* resource);

// --- `anonymous namespace' ---
// CODEVIEW(E:\gamedcs\herodefs.cpp:391, dc 0xd60d4) void `anonymous namespace'::TAutoStrPtr::TAutoStrPtr();
// CODEVIEW(E:\gamedcs\herodefs.cpp:394, dc 0xd60dc) void `anonymous namespace'::TAutoStrPtr::~TAutoStrPtr();
// CODEVIEW(E:\gamedcs\herodefs.cpp:396, dc 0xd60f4) void `anonymous namespace'::TAutoStrPtr::set(char* pStr);
// CODEVIEW(E:\gamedcs\herodefs.cpp:398, dc 0xd60f8) char* `anonymous namespace'::TAutoStrPtr::get();

#endif  /* HOMM3_HERODEFS_H */
