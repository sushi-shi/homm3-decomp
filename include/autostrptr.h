// autostrptr.h - the source-private string owner the trait-table loaders
// share. HAND-OWNED.
//
// The Dreamcast roster attests `anonymous namespace'::TAutoStrPtr with the
// same four methods in THREE separate compilands - herodefs.cpp
// (dc 0xd60d4), spelldefs.cpp and creaturetype.cpp (dc 0x71eec..0x71f14) -
// so every one of them carries its own copy of the class, and retail folds
// all four methods into each loader.
//
// herodefs.h and spelldefs.h still spell their copies inline; those two are
// never in one translation unit together, but creaturetype.h is included by
// ten TUs (spells.cpp among them, which already sees spelldefs.h), so this
// compiland's copy lives in a header of its own rather than in
// creaturetype.h. Converging the other two onto this file is a later lane's
// change: it would move a declarator out of two headers and into one.
#ifndef HOMM3_AUTOSTRPTR_H
#define HOMM3_AUTOSTRPTR_H

namespace {

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

#endif  /* HOMM3_AUTOSTRPTR_H */
