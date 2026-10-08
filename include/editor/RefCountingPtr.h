// RefCountingPtr.h - a copy-on-write handle (Loki h3maped).
// The handle points at a wrapper that holds the reference count and the
// owned object. Copies share the wrapper; the non-const accessors split a
// shared wrapper first, the const ones never do.
//
// Windows keeps the object inside the wrapper, after the count: h3maped's
// TGameMap::TLayer::getCell (0x41e82b) reads the layer's cell grid handle
// at wrapper + 8 (the implementation's second field) and the grid itself
// at that handle's wrapper + 4, and the cell lookup (0x42a4d0) reaches each
// segment at its wrapper + 4. Loki's port holds a pointer there instead.
// The release throws the bare TAllocationFailure.
#ifndef HOMM3_EDITOR_REFCOUNTINGPTR_H
#define HOMM3_EDITOR_REFCOUNTINGPTR_H

#include <stddef.h>

#include "exceptions.h"

// The count and the object. A namespace-scope template rather than Loki's
// nested _TWrapper: VC6 completes a nested class with its enclosing
// template, and the handle is a member of the classes whose implementation
// it holds, before that implementation is complete. The name is not proven.
template<class T>
struct TRefCountingWrapper {
    TRefCountingWrapper() : m_refCnt(1) {}
    TRefCountingWrapper(const T& object) : m_refCnt(1), m_object(object) {}

    unsigned int m_refCnt;
    T m_object;
};

template<class T>
class TRefCountingPtr {
public:
    TRefCountingPtr() : _m_pWrapper(new _TWrapper)
    {
        if (_m_pWrapper == NULL)
            _fail();
    }

    TRefCountingPtr(const T& object) : _m_pWrapper(new _TWrapper(object))
    {
        if (_m_pWrapper == NULL)
            _fail();
    }

    TRefCountingPtr(const TRefCountingPtr& other) : _m_pWrapper(other._m_pWrapper)
    {
        ++_m_pWrapper->m_refCnt;
    }

    ~TRefCountingPtr()
    {
        if (--_m_pWrapper->m_refCnt == 0)
            delete _m_pWrapper;
    }

    TRefCountingPtr& operator=(const TRefCountingPtr& other)
    {
        ++other._m_pWrapper->m_refCnt;
        if (--_m_pWrapper->m_refCnt == 0)
            delete _m_pWrapper;
        _m_pWrapper = other._m_pWrapper;
        return *this;
    }

    T* get()
    {
        if (_m_pWrapper->m_refCnt > 1)
            _split();
        return &_m_pWrapper->m_object;
    }
    const T* get() const { return &_m_pWrapper->m_object; }

    T* operator->() { return get(); }
    const T* operator->() const { return get(); }
    T& operator*() { return *get(); }
    const T& operator*() const { return *get(); }

private:
    typedef TRefCountingWrapper<T> _TWrapper;

    void _fail()
    {
        throw TAllocationFailure();
    }

    void _split();

    _TWrapper* _m_pWrapper;
};

template <class T>
void TRefCountingPtr<T>::_split()
{
    _TWrapper* pNewWrapper = new _TWrapper(_m_pWrapper->m_object);
    if (pNewWrapper == NULL)
        _fail();
    --_m_pWrapper->m_refCnt;
    _m_pWrapper = pNewWrapper;
}

#endif  /* HOMM3_EDITOR_REFCOUNTINGPTR_H */
