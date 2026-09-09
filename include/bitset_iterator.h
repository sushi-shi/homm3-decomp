#ifndef HOMM3_BITSET_ITERATOR_H
#define HOMM3_BITSET_ITERATOR_H

#include <va.h>
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
    // Before normalization: bits_.
    std::bitset<N>* m_bits;
    // Before normalization: position_.
    size_t m_position;
};

// Retail 0x48eb40 copies this adapter's pointer and position into the
// two-word bitset reference returned to the legacy campaign-bit copy.
// Retail 0x4d4ca0 is the same operation for game::GetRandomMonster's
// 145-bit iterator, retained at call 0x4c931b. Both instances share this
// canonical generic operation; their comparison TUs own no copied body.
template <size_t N>
// VA instance: bitset_iterator<144>::operator*
VA(0x0048eb40, 0x14)  // retained caller in ScenarioStruct::read
// VA instance: bitset_iterator<145>::operator*
VA(0x004d4ca0, 0x14)  // retained caller in game::getRandomMonster
typename std::bitset<N>::reference bitset_iterator<N>::operator*() const
{
    return (*m_bits)[m_position];
}

#endif
