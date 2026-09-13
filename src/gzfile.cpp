// gzfile.cpp - the zlib-backed TAbstractFile every savegame, map and
// campaign write goes through.

// THIS COMPILAND IS ABSENT FROM THE DREAMCAST ROSTER (the Dreamcast port
// has no gz stream at all). Retail's object is the one opened by the
// cinit at 0x4d6c30 and closed before the next unit's cinit at 0x4d6dc0,
// which brackets exactly the eight bodies below: the constructor, the two
// gz virtual slots, the destructor and the four compiler-generated
// thunks the `throw TOpenFailure()` forces out. It sits one object behind
// TGzInflateBuf's in the gametypewindow..hero link-order bracket.

// The class, its layout and its TOpenFailure tag are modelled in
// savegame.h off the retail bytes; this unit only supplies the bodies.
// Retail's zlib is FASTCALL (`@gzopen@8`, `@gzread@12`), which is what the
// vendored zlib-1.1.3 header emits under this profile's /Gr - so <zlib.h>
// resolves here from vendor/zlib-1.1.3, the exact library retail links.
#include <va.h>
#include <zlib.h>
#include "savegame.h"

VA(0x004d6c50, 0x76)
TGzFile::TGzFile(const char* path, const char* mode)
    : m_file(gzopen(path, mode))
{
    if (m_file == 0)
        throw TOpenFailure();
}

VA_COMPGEN(0x004d6cd0, 0x21, SCALAR_DELETING_DTOR, TGzFile)
VA_COMPGEN(0x004d6d00, 0x5, IMPLICIT_DTOR, TOpenFailure)
VA_COMPGEN(0x004d6d10, 0x1C, IMPLICIT_COPY_CTOR, TOpenFailure)
VA_COMPGEN(0x004d6d30, 0x21, SCALAR_DELETING_DTOR, TOpenFailure)

VA(0x004d6d60, 0x19)
TGzFile::~TGzFile()
{
    gzclose(m_file);
}

VA(0x004d6d80, 0x16)
int TGzFile::read(void* data, int size)
{
    return gzread(m_file, data, size);
}

VA(0x004d6da0, 0x16)
int TGzFile::write(const void* data, int size)
{
    return gzwrite(m_file, const_cast<void*>(data), size);
}
