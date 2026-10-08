// ResourceQuantities.cpp - Loki h3maped object 25: the validated accessors
// and the binary form (seven longs in resource order).
#include "editor/stdafx.h"

#include <assert.h>

#include "editor/RawStream.h"
#include "editor/ResourceQuantities.h"

void TResourceQuantities::set(TGameResourceType resourceType, int newQuantity)
{
#line 28
    assert(resourceType >= 0 && resourceType < kNumGameResourceTypes);
    assert(newQuantity >= s_kMin && newQuantity <= s_kMax);
    _m_quantities[resourceType] = newQuantity;
}

int TResourceQuantities::get(TGameResourceType resourceType) const
{
#line 37
    assert(resourceType >= 0 && resourceType < kNumGameResourceTypes);
    return _m_quantities[resourceType];
}

TRawOStream& operator<<(TRawOStream& stream, const TResourceQuantities& quantities)
{
    for (unsigned int i = 0; i < kNumGameResourceTypes; i++)
        stream << static_cast<long>(quantities.get(TGameResourceType(i)));
    return stream;
}

TRawIStream& operator>>(TRawIStream& stream, TResourceQuantities& quantities)
{
    for (unsigned int i = 0; i < kNumGameResourceTypes; i++) {
        long quantity;
        stream >> quantity;
        quantities.set(TGameResourceType(i), quantity);
    }
    return stream;
}
