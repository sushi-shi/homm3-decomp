#ifndef HOMM3_BITSET_ITERATOR_H
#define HOMM3_BITSET_ITERATOR_H

#include <bitset>

// The PC standard library's bitset has no iterator surface.  The game uses
// this two-word adapter where a run of bits is traversed as a range; its
// pointer/offset layout is also the layout of bitset<N>::reference.
template <size_t N>
class bitset_iterator {
public:
    bitset_iterator()
        : bits_(0), position_(0)
    {
    }

    bitset_iterator(std::bitset<N>& bits, size_t position)
        : bits_(&bits), position_(position)
    {
    }

    typename std::bitset<N>::reference operator*() const;

    bitset_iterator& operator++()
    {
        ++position_;
        return *this;
    }

    bool operator!=(const bitset_iterator& other) const
    {
        return bits_ != other.bits_ || position_ != other.position_;
    }

private:
    std::bitset<N>* bits_;
    size_t position_;
};

template <size_t N>
typename std::bitset<N>::reference bitset_iterator<N>::operator*() const
{
    return (*bits_)[position_];
}

#endif
