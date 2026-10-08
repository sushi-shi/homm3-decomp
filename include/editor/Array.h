// Array.h - a fixed-size array with an STL face (Loki h3maped).
// Every member is inline; the instantiations are linkonce bodies owned by
// the first object that uses them (TArray<TCreatureStack, 7> in Army.cpp,
// TArray<int, 7> in BlackBox.cpp, the TCell grids in GameMap.cpp). The
// accessors carry empty exception specifications (`__check_eh_spec` in
// every begin/end/operator[]/size). The comparison is a member template
// over the other array's element type (`__eq__H1Zi_Ct6TArray2ZiUi7...`).
// The header's file name and the element member's name are not proven:
// no assert in the class survives.
#ifndef HOMM3_EDITOR_ARRAY_H
#define HOMM3_EDITOR_ARRAY_H

#include <algorithm>
#include <new>

template<class T, unsigned int N>
class TArray {
public:
    typedef T value_type;
    typedef T* iterator;
    typedef const T* const_iterator;
    typedef T& reference;
    typedef const T& const_reference;
    typedef unsigned int size_type;

    // Windows' array is its first element, a member, followed by storage
    // for the rest, which the class constructs and destroys itself.
    // h3maped proves it in every constructor and destructor: the cell
    // segment's default constructor (0x43900a) constructs element 0 and
    // only then registers its unwind (~T on this), placement-news elements
    // 1..N-1 in a null-checked loop (placement delete on unwind), and its
    // destructor (0x439067) destroys N-1..1 in reverse before the member
    // element 0; the fill (player bookkeeping 0x438696, the resource
    // dialog 0x4b34a0) and the copy (TTimedEvent 0x417060) store element 0
    // first, then loop; a POD array's destructor (0x438787) leaves the
    // dead `end()` of its emptied loop. Loki's port assigns into a plain
    // array.
    TArray() { for (iterator p = begin() + 1; p != end(); ++p) new (p) T; }
    explicit TArray(const T& value) : _m_first(value) { uninitialized_fill(begin() + 1, end(), value); }
    TArray(const TArray& other) : _m_first(other._m_first)
    {
        uninitialized_copy(other.begin() + 1, other.end(), begin() + 1);
    }
    ~TArray()
    {
        for (iterator p = end(); p != begin() + 1; ) {
            --p;
            p->~T();
        }
    }

    TArray& operator=(const TArray& other)
    {
        if (this != &other)
            copy(other.begin(), other.end(), begin());
        return *this;
    }

    iterator begin() throw() { return &_m_first; }
    const_iterator begin() const throw() { return &_m_first; }
    iterator end() throw() { return begin() + N; }
    const_iterator end() const throw() { return begin() + N; }
    static size_type size() throw() { return N; }

    reference operator[](size_type i) throw() { return begin()[i]; }
    const_reference operator[](size_type i) const throw() { return begin()[i]; }

    template<class U>
    bool operator==(const TArray<U, N>& other) const
    {
        return equal(begin(), end(), other.begin());
    }

private:
    T _m_first;
    // Elements 1..N-1, raw until the constructors build them in place.
    char _m_aRestStorage[(N - 1) * sizeof(T)];
};

#endif  /* HOMM3_EDITOR_ARRAY_H */
