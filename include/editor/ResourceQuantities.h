// ResourceQuantities.h - an amount of each of the seven resources
// (ResourceQuantities.cpp; Loki h3maped). A TArray<int, 7> filled with
// zero (the resource dialog's constructor 0x4b34a0 inlines the fill),
// copied and compared through TArray. The member's name is not recorded.
#ifndef HOMM3_EDITOR_RESOURCEQUANTITIES_H
#define HOMM3_EDITOR_RESOURCEQUANTITIES_H

#include "editor/Array.h"
#include "editor/GameResource.h"

class TResourceQuantities {
public:
    enum { s_kMin = -99999, s_kMax = 99999 };

    TResourceQuantities() : _m_quantities(0) {}

    void set(TGameResourceType resourceType, int newQuantity);
    int get(TGameResourceType resourceType) const { return _m_quantities[resourceType]; }

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

#endif  /* HOMM3_EDITOR_RESOURCEQUANTITIES_H */
