#ifndef HOMM3_BITSET_ITERATOR_H
#define HOMM3_BITSET_ITERATOR_H

#include "va.h"

#include <bitset>

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

    bitset_iterator(std::bitset<N>& bits, size_t position)
        : m_bits(&bits), m_position(position)
    {
    }

    typename std::bitset<N>::reference operator*() const;

    bitset_iterator& operator++()
    {
        ++m_position;
        return *this;
    }

    bool operator!=(const bitset_iterator& other) const
    {
        return m_bits != other.m_bits || m_position != other.m_position;
    }

private:
    std::bitset<N>* m_bits;
    size_t m_position;
};

template <size_t N>
// VA instance: bitset_iterator<144>::operator*
VA(0x0048eb40, 0x14)  // retained caller in ScenarioStruct::read
// VA instance: bitset_iterator<145>::operator*
VA(0x004d4ca0, 0x14)  // retained caller in game::getRandomMonster
typename std::bitset<N>::reference bitset_iterator<N>::operator*() const
{
    // Named pointer/position aliases perturb this call's inlining, but lower
    // NewSMapHeader::read and the RMG writer and suppress an exact game
    // COMDAT. Keep the canonical direct dereference.
    return (*m_bits)[m_position];
}

#endif
