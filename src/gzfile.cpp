// gzfile.cpp - the zlib-backed TAbstractFile every savegame, map and
// campaign write goes through.
//
// THIS COMPILAND IS ABSENT FROM THE DREAMCAST ROSTER (the Dreamcast port
// has no gz stream at all). Retail's object is the one opened by the
// cinit at 0x4d6c30 and closed before the next unit's cinit at 0x4d6dc0,
// which brackets exactly the eight bodies below: the constructor, the two
// gz virtual slots, the destructor and the four compiler-generated
// thunks the `throw TOpenFailure()` forces out. It sits one object behind
// TGzInflateBuf's in the gametypewindow..hero link-order bracket.
//
// The class, its layout and its TOpenFailure tag are modelled in
// savegame.h off the retail bytes; this unit only supplies the bodies.
// Retail's zlib is FASTCALL (`@gzopen@8`, `@gzread@12`), which is what the
// vendored zlib-1.1.3 header emits under this profile's /Gr - so <zlib.h>
// resolves here from vendor/zlib-1.1.3, the exact library retail links.
#include <va.h>
#include <zlib.h>
#include "savegame.h"

// 0x4d6c50 stores TAbstractFile's vftable, calls @gzopen@8 with the path
// in ecx and the mode in edx, parks the handle at +4, swaps in TGzFile's
// own vftable and throws the empty TOpenFailure tag when gzopen fails.
// The handle is a MEMBER INITIALISER: written as a body assignment VC6
// folds the two vptr stores into one ahead of the call (92.76%); the
// initialiser keeps the base store at the top and sinks TGzFile's own
// behind @gzopen@8, which is retail's order exactly.
VA(0x004d6c50, 0x76)  // anchor-import @gzopen@8 + anchor-vtable ??_7TGzFile@@6B@, retail-only
TGzFile::TGzFile(const char* path, const char* mode)
    : file(gzopen(path, mode))
{
    if (file == 0)
        throw TOpenFailure();
}

// The four thunks the class pair forces out of the compiler: TGzFile's
// deleting destructor fills the vftable slot the base declares virtual,
// and TOpenFailure's destructor / copy constructor / deleting destructor
// are what __CxxThrowException's catchable-type record points at.
VA_COMPGEN(0x004d6cd0, 0x21, SCALAR_DELETING_DTOR, TGzFile)
VA_COMPGEN(0x004d6d00, 0x5, IMPLICIT_DTOR, TOpenFailure)
VA_COMPGEN(0x004d6d10, 0x1C, IMPLICIT_COPY_CTOR, TOpenFailure)
VA_COMPGEN(0x004d6d30, 0x21, SCALAR_DELETING_DTOR, TOpenFailure)

VA(0x004d6d60, 0x19)  // anchor-import @gzclose@4, retail-only
TGzFile::~TGzFile()
{
    gzclose(file);
}

VA(0x004d6d80, 0x16)  // anchor-import @gzread@12, retail-only
int TGzFile::Read(void* data, int size)
{
    return gzread(file, data, size);
}

VA(0x004d6da0, 0x16)  // anchor-import @gzwrite@12, retail-only
int TGzFile::Write(const void* data, int size)
{
    return gzwrite(file, const_cast<void*>(data), size);
}
