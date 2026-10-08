// RefCountingPtr.h - a copy-on-write handle (Loki h3maped).
// The handle points at a wrapper that holds the reference count and the
// owned object. Copies share the wrapper; the non-const accessors split a
// shared wrapper first, the const ones never do. A failed allocation throws
// TAllocationFailure from this file (RefCountingPtr.h:67 in the wrapper,
// :75 in the handle). Every member is inline; the instantiations are
// linkonce bodies of their first users (TBlackBox::TContents in
// GUIGameObject.cpp and BlackBox.cpp, the map's implementation objects in
// GameMap.cpp). No assert names the members; their names are not recorded.
#ifndef HOMM3_EDITOR_REFCOUNTINGPTR_H
#define HOMM3_EDITOR_REFCOUNTINGPTR_H

#include <stddef.h>

#include "exceptions.h"

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
        return _m_pWrapper->m_pObject;
    }
    const T* get() const { return _m_pWrapper->m_pObject; }

    T* operator->() { return get(); }
    const T* operator->() const { return get(); }
    T& operator*() { return *get(); }
    const T& operator*() const { return *get(); }

private:
    struct _TWrapper {
        _TWrapper() : m_refCnt(1)
        {
            if ((m_pObject = new T) == NULL)
                _fail();
        }

        _TWrapper(const T& object) : m_refCnt(1)
        {
            if ((m_pObject = new T(object)) == NULL)
                _fail();
        }

        ~_TWrapper() { delete m_pObject; }

        void _fail()
        {
#line 67 "RefCountingPtr.h"
            throw TAllocationFailure(__FILE__, __LINE__);
        }

        unsigned int m_refCnt;
        T* m_pObject;
    };

    void _fail()
    {
#line 75 "RefCountingPtr.h"
        throw TAllocationFailure(__FILE__, __LINE__);
    }

    void _split()
    {
        _TWrapper* pNewWrapper = new _TWrapper(*_m_pWrapper->m_pObject);
        if (pNewWrapper == NULL)
            _fail();
        --_m_pWrapper->m_refCnt;
        _m_pWrapper = pNewWrapper;
    }

    _TWrapper* _m_pWrapper;
};

#endif  /* HOMM3_EDITOR_REFCOUNTINGPTR_H */
