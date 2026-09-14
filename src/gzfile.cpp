// gzfile.cpp - the zlib-backed TAbstractFile every savegame, map and
// campaign write goes through.

// Complete wraps gz files in TAbstractFile; Dreamcast calls gzopen/gzclose directly.

// The class, its layout and its TOpenFailure tag are modelled in
// gzfile.h off the retail bytes; this unit only supplies the bodies.
// Retail's zlib is FASTCALL (`@gzopen@8`, `@gzread@12`), which is what the
// vendored zlib-1.1.3 header emits under this profile's /Gr - so <zlib.h>
// resolves here from vendor/zlib-1.1.3, the exact library retail links.
#include <va.h>
#include <zlib.h>
#include "gzfile.h"

VA(0x004d6c50, 0x76)
GzFile::GzFile(const char* path, const char* mode)
    : m_file(gzopen(path, mode))
{
    if (m_file == 0)
        throw OpenFailure();
}

VA_COMPGEN(0x004d6cd0, 0x21, SCALAR_DELETING_DTOR, GzFile)
VA_COMPGEN(0x004d6d00, 0x5, IMPLICIT_DTOR, OpenFailure)
VA_COMPGEN(0x004d6d10, 0x1C, IMPLICIT_COPY_CTOR, OpenFailure)
VA_COMPGEN(0x004d6d30, 0x21, SCALAR_DELETING_DTOR, OpenFailure)

VA(0x004d6d60, 0x19)
GzFile::~GzFile()
{
    gzclose(m_file);
}

VA(0x004d6d80, 0x16)
int GzFile::read(void* data, int size)
{
    return gzread(m_file, data, size);
}

VA(0x004d6da0, 0x16)
int GzFile::write(const void* data, int size)
{
    return gzwrite(m_file, const_cast<void*>(data), size);
}
