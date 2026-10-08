// T16bppPalette.cpp - Loki h3maped object 6 (file name follows the class).
#include "exceptions.h"
#include "editor/T16bppPalette.h"

T16bppPalette::T16bppPalette(const TRGB* rgb)
{
    for (unsigned short* entry = m_entries; entry < m_entries + T8bppBitmapBase<unsigned char>::kNumColors; entry++)
        *entry = rgbToEntry(*rgb++);
}
