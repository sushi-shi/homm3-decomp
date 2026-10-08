// ResourceQuantities.h - an amount of each of the seven resources
// (Loki h3maped ResourceQuantities.cpp; 0x1c bytes inside TTimedEvent).
// The constructor, assignment and comparison are inline (linkonce bodies
// owned by BlackBox.cpp): a TArray<int, 7> filled with zero, copied and
// compared through TArray. set/get assert the type and the
// [s_kMin, s_kMax] range. The member's name is not recorded.
#ifndef HOMM3_EDITOR_RESOURCEQUANTITIES_H
#define HOMM3_EDITOR_RESOURCEQUANTITIES_H

#include "editor/Array.h"
#include "editor/GameResource.h"

class TRawIStream;
class TRawOStream;

class TResourceQuantities {
public:
    static const int s_kMin = -99999;
    static const int s_kMax = 99999;

    TResourceQuantities() : _m_quantities(0) {}

    void set(TGameResourceType resourceType, int newQuantity);
    int get(TGameResourceType resourceType) const;

    friend bool operator==(const TResourceQuantities& lhs, const TResourceQuantities& rhs)
    {
        return lhs._m_quantities == rhs._m_quantities;
    }
    friend bool operator!=(const TResourceQuantities& lhs, const TResourceQuantities& rhs)
    {
        return !(lhs == rhs);
    }

private:
    TArray<int, kNumGameResourceTypes> _m_quantities;
};

TRawOStream& operator<<(TRawOStream& stream, const TResourceQuantities& quantities);
TRawIStream& operator>>(TRawIStream& stream, TResourceQuantities& quantities);

#endif  /* HOMM3_EDITOR_RESOURCEQUANTITIES_H */
