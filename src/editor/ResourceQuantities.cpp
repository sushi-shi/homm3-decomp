// ResourceQuantities.cpp - the resource amounts' accessors and binary form
// (h3maped 0x4b33d4..0x4b3442; Loki h3maped object 25): seven longs in
// resource order. Release drops Loki's range asserts. /OPT:ICF folded the
// accessors onto identical bodies in earlier objects.
#include "editor/stdafx.h"

#include "va.h"
#include "editor/RawStream.h"
#include "editor/ResourceQuantities.h"

VA(0x0044a367, 0xe)
void TResourceQuantities::set(TGameResourceType resourceType, int newQuantity)
{
    _m_quantities[resourceType] = newQuantity;
}

VA(0x004b33f0, 0x29)
TRawOStream& operator<<(TRawOStream& stream, const TResourceQuantities& quantities)
{
    for (unsigned int i = 0; i < kNumGameResourceTypes; i++)
        stream << static_cast<long>(quantities.get(TGameResourceType(i)));
    return stream;
}

VA(0x004b3419, 0x29)
TRawIStream& operator>>(TRawIStream& stream, TResourceQuantities& quantities)
{
    for (unsigned int i = 0; i < kNumGameResourceTypes; i++) {
        long quantity;
        stream >> quantity;
        quantities.set(TGameResourceType(i), quantity);
    }
    return stream;
}

VA(0x004c1904, 0xa)
int TResourceQuantities::get(TGameResourceType resourceType) const
{
    return _m_quantities[resourceType];
}
