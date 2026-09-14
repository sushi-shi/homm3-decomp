#ifndef HOMM3_BITSET_ITERATOR_H
#define HOMM3_BITSET_ITERATOR_H

#include <va.h>
#include <bitset>

// The PC standard library's bitset has no iterator surface.  The game uses
// this two-word adapter where a run of bits is traversed as a range; its
// pointer/offset layout is also the layout of bitset<N>::reference.
template <size_t N>
// Before normalization (type): bitset_iterator.
class BitsetIterator {
public:
    BitsetIterator()
        : m_bits(0), m_position(0)
    {
    }

    BitsetIterator(std::bitset<N>& bits, size_t position)
        : m_bits(&bits), m_position(position)
    {
    }

    typename std::bitset<N>::reference operator*() const;

    BitsetIterator& operator++()
    {
        ++m_position;
        return *this;
    }

    bool operator!=(const BitsetIterator& other) const
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
typename std::bitset<N>::reference BitsetIterator<N>::operator*() const
{
    return (*m_bits)[m_position];
}

#endif
