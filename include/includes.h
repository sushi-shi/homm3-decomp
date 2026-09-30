// includes.h - shared source helpers recovered from Dreamcast CodeView.
#ifndef HOMM3_INCLUDES_H
#define HOMM3_INCLUDES_H

#include "DC_precompiledheaders.h"
#include "va.h"

#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif

#include <vector>

// Unqualified min/max also reach std's templates (VC6's library has none).
using namespace std;

// E:\gamedcs\includes.h:97 The wrapper owns argument
// copies, then dereferences the selector's returned argument address.
DC_ADDRESS(0x01ef28, 0x34)
inline int max(int left, int right)
{
    return cppMax(left, right);
}

// E:\gamedcs\includes.h:114
DC_ADDRESS(0x002da4, 0x28)
inline int min(int left, int right)
{
    return cppMin(left, right);
}

// Original: min; includes.h:117
DC_ADDRESS(0x04c9c0, 0x2c)
inline double min(double left, double right)
{
    return cppMin(left, right);
}

// E:\gamedcs\includes.h:124 CodeView types all three
// parameters and the return as const references. The retained retail body and
// ordinary limit expansions use maximum < value for the upper clamp. Retail
// expands this helper through the by-value limit wrapper in the adventure and
// small-window TUs.
template <class T>
DC_ADDRESS(0x020d2c, 0x38)
inline const T& tLimit(const T& minimum, const T& value,
                       const T& maximum)
{
    if (value < minimum) {
        return minimum;
    } else if (maximum < value) {
        return maximum;
    } else {
        return value;
    }
}
// E:\gamedcs\includes.h:134
DC_ADDRESS(0x01ef5c, 0x38)
inline int limit(int minimum, int value, int maximum)
{
    return tLimit(minimum, value, maximum);
}

// The no-repeat random picker. Retail's ctor/Pick pair byte-proves the
// The same vector<bool> declaration selects VC6's generic byte container
// (allocator byte, _First, _Last, _End) and CodeWarrior's packed-bit
// specialization (three words). Native Mac pick calls the word-mask
// proxies at 0x99fb8/0xe73b8; no target-specific declaration is needed.
// Original CodeView fields: Low, NumbersLeft, Available. Project spelling
// follows the m_ scope prefix and lowerCamelCase convention.
class TPickANumber {
protected:
    int m_low;

public:
    int m_numbersLeft;
    std::vector<bool> m_available;
    TPickANumber(int lowBound, int high);
    // Original: TPickANumber::IsAvailable; includes.h:166
    DC_ADDRESS(0x0fe374, 0x24)
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
    // game.cpp's array initializer takes this constructor's address.
    DC_ADDRESS(0x0bc7c8, 0x24)
    VA(0x004caa10, 0x10)
    TPickRandomTownName() : TPickANumber(0, 15) {}
    // E:\gamedcs\includes.h:178
    DC_ADDRESS(0x0bc7ec, 0x7c)
    void reset()
    {
        for (int i = 0; i < m_available.size(); ++i)
            m_available[i] = 1;
        m_numbersLeft = m_available.size();
    }
};

#endif
