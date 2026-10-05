#ifndef HOMM3_BITSET_ITERATOR_H
#define HOMM3_BITSET_ITERATOR_H

#include "va.h"

#include <bitset>
#include <stddef.h>

// The PC standard library's bitset has no iterator surface.  The game uses
// this two-word adapter where a run of bits is traversed as a range; its
// pointer/offset layout is also the layout of bitset<N>::reference.
template <size_t N>
class bitset_iterator {
public:
    bitset_iterator()
        : m_bits(0), m_position(0)
    {
    }

    bitset_iterator(std::bitset<N>& bits, size_t position = 0)
        : m_bits(&bits), m_position(position)
    {
    }

    // Mac getRandomMonster 0xdff38..0xdff6c advances a zero-offset temporary
    // before copying its result into the fill range. By-value addition keeps
    // that lifetime and value semantics; free-function placement is inferred
    // because no native member declaration survives.
    friend bitset_iterator operator+(bitset_iterator result, size_t offset)
    {
        result.m_position += offset;
        return result;
    }

    // Windows retains this dereference body; the Mac caller expands the
    // adapter before constructing its two-word bit reference.
    // VA instance: bitset_iterator<144>::operator*
    VA(0x0048eb40, 0x14)
    // VA instance: bitset_iterator<145>::operator*
    VA(0x004d4ca0, 0x14)
    typename std::bitset<N>::reference operator*() const
    {
        return (*m_bits)[m_position];
    }

    bitset_iterator& operator++()
    {
        ++m_position;
        return *this;
    }

    // Mac getRandomMonster 0xdffa0 compares owner before position. A
    // symmetric comparison preserves that operation and lets VC6 expand it
    // inside std::fill, as retail does, while retaining operator*'s call.
    // Free-function placement is inferred; no native declaration survives.
    friend bool operator!=(const bitset_iterator& left,
                           const bitset_iterator& right)
    {
        return left.m_bits != right.m_bits || left.m_position != right.m_position;
    }

private:
    std::bitset<N>* m_bits;
    size_t m_position;
};

#endif
