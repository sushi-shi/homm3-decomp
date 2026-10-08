// DIBSection.h - a 16-bit bitmap whose pixels are a GDI DIB section
// (DIBSection.cpp, name inferred from its alphabetical slot). RTTI names
// T16bppDIBSection with two bases: T16bppBitmapBase<unsigned long> first
// (its primary vtable is ??_7T16bppDIBSection@@6B?$T16bppBitmapBase@K@@@),
// then CBitmap at +0x14, which holds the section's handle. Its
// vtable adds a sixth slot, the assignment from another DIB section.
#ifndef HOMM3_EDITOR_DIBSECTION_H
#define HOMM3_EDITOR_DIBSECTION_H

#include "editor/T16bppBitmap.h"

class T16bppDIBSection : public T16bppBitmapBase<unsigned long>, public CBitmap {
public:
    T16bppDIBSection();
    T16bppDIBSection(unsigned int width, unsigned int height);
    virtual ~T16bppDIBSection();

    virtual T16bppDIBSection& operator=(const T16bppDIBSection& other);

    virtual unsigned short* allocate(unsigned int width, unsigned int height);
    virtual void free(unsigned short* pPixels);
};

#endif  /* HOMM3_EDITOR_DIBSECTION_H */
