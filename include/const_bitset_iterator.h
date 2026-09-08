#ifndef HOMM3_CONST_BITSET_ITERATOR_H
#define HOMM3_CONST_BITSET_ITERATOR_H

#include <bitset>

// Retail 0x44d063..0x44d0bf compares both owner and offset with an end
// iterator, searches through bitset::test, copies the found offset, then
// checks the end again. This is a const traversal twin of bitset_iterator;
// its original source name is unknown. The combination pass is Complete-only.
template<size_t N>
class TConstBitsetIterator {
public:
    TConstBitsetIterator(const std::bitset<N>& bits, size_t position)
        : m_bits(&bits), m_position(position) {}
    bool operator*() const { return m_bits->test(m_position); }
    TConstBitsetIterator& operator++()
    {
        ++m_position;
        return *this;
    }
    bool operator!=(const TConstBitsetIterator& other) const
    {
        return m_bits != other.m_bits || m_position != other.m_position;
    }
    size_t position() const { return m_position; }
private:
    const std::bitset<N>* m_bits;
    size_t m_position;
};

// Identity predicate for the set-bit search. Retail tests the returned bool
// at 0x44d077; std::find(..., true) instead emits cmp al,1 / je.
struct TBitIsSet {
    bool operator()(bool value) const { return value; }
};

#endif
