// BitsetIterator.h - an iterator over a bit set's bits, so the map's
// readers can copy an older edition's shorter mask into the current one
// with std::copy (h3maped 0x433e3b copies 129 artifact bits; each source bit
// reads through bitset::reference, 0x436f00, and each target bit sets
// through it).
#ifndef HOMM3_EDITOR_BITSETITERATOR_H
#define HOMM3_EDITOR_BITSETITERATOR_H

#include <bitset>
#include <iterator>

template <size_t N>
class TBitsetIterator {
public:
    typedef std::forward_iterator_tag iterator_category;
    typedef bool value_type;
    typedef ptrdiff_t difference_type;

    TBitsetIterator(std::bitset<N>& bits, size_t pos) : _m_pBits(&bits), _m_pos(pos) {}

    typename std::bitset<N>::reference operator*() const { return (*_m_pBits)[_m_pos]; }
    TBitsetIterator& operator++()
    {
        ++_m_pos;
        return *this;
    }
    TBitsetIterator operator++(int)
    {
        TBitsetIterator result = *this;
        ++_m_pos;
        return result;
    }
    bool operator==(const TBitsetIterator& other) const
    {
        return _m_pBits == other._m_pBits && _m_pos == other._m_pos;
    }
    bool operator!=(const TBitsetIterator& other) const { return !(*this == other); }

private:
    std::bitset<N>* _m_pBits;
    size_t _m_pos;
};

#endif  /* HOMM3_EDITOR_BITSETITERATOR_H */
