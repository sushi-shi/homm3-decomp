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

template<class T, unsigned int N>
class TArray {
public:
    typedef T value_type;
    typedef T* iterator;
    typedef const T* const_iterator;
    typedef T& reference;
    typedef const T& const_reference;
    typedef unsigned int size_type;

    TArray() {}
    explicit TArray(const T& value) { fill(begin(), end(), value); }

    TArray& operator=(const TArray& other)
    {
        if (this != &other)
            copy(other.begin(), other.end(), begin());
        return *this;
    }

    iterator begin() throw() { return _m_elements; }
    const_iterator begin() const throw() { return _m_elements; }
    iterator end() throw() { return _m_elements + N; }
    const_iterator end() const throw() { return _m_elements + N; }
    size_type size() throw() { return N; }

    reference operator[](size_type i) throw() { return _m_elements[i]; }
    const_reference operator[](size_type i) const throw() { return _m_elements[i]; }

    template<class U>
    bool operator==(const TArray<U, N>& other) const
    {
        return equal(begin(), end(), other.begin());
    }

private:
    T _m_elements[N];
};

#endif  /* HOMM3_EDITOR_ARRAY_H */
