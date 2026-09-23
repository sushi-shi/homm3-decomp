// includes.h - shared source helpers recovered from Dreamcast CodeView.
#ifndef HOMM3_INCLUDES_H
#define HOMM3_INCLUDES_H

#include "DC_precompiledheaders.h"

#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif

#include <vector>

#include "inline/max_int.inl"

// E:\gamedcs\includes.h:114, dc 0x2da4.
inline int min(int left, int right)
{
    return cppMin(left, right);
}

// Original: min; includes.h:117, dc 0x4c9c0.
inline double min(double left, double right)
{
    return cppMin(left, right);
}

#include "inline/t_limit.inl"
#include "inline/limit.inl"

// The no-repeat random picker. Retail's ctor/Pick pair byte-proves the
// VC6 generic vector<unsigned char> representation at +8: allocator
// byte, _First, _Last, _End. Dreamcast instead instantiated STLport's
// vector<bool>, a platform-library divergence rather than x86 evidence.
// Original CodeView fields: Low, NumbersLeft, Available. Project spelling
// follows the m_ scope prefix and lowerCamelCase convention.
class TPickANumber {
protected:
    int m_low;

public:
    int m_numbersLeft;
    std::vector<unsigned char> m_available;
    TPickANumber(int lowBound, int high);
    // Original: TPickANumber::IsAvailable; includes.h:166, dc 0xfe374.
    unsigned char isAvailable(int number) const
    {
        return m_available[number - m_low];
    }
    int pick();
    void markOut(int number);
};

// E:\gamedcs\includes.h:175/178. The written inline constructor
// passes [0, 15] to the base; Reset is expanded into ProcessOnMapTowns.
// game.obj emits the Dreamcast copies but does not own their source bodies.
class TPickRandomTownName : public TPickANumber {
public:
    TPickRandomTownName() : TPickANumber(0, 15) {}
    // E:\gamedcs\includes.h:178, dc 0xbc7ec
    void reset()
    {
        for (int i = 0; i < m_available.size(); ++i)
            m_available[i] = 1;
        m_numbersLeft = m_available.size();
    }
};

#endif
