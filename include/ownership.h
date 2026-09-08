// Compatibility include for loader ownership wrappers. CodeView-proven
// templates retain their original headers; the remaining retail wrappers
// await a proven source identity.
#ifndef HOMM3_OWNERSHIP_H
#define HOMM3_OWNERSHIP_H

#include "autoarrayptr.h"

// Scalar twin of TAutoArrayPtr. ResourceManager's temporary 24-bit bitmap
// guard proves the same owns-byte-plus-pointer layout, but its cleanup invokes
// the scalar deleting destructor with flag 1 rather than resource::Dispose.
template<class T>
class TAutoPtr {
public:
    TAutoPtr(T* ptr = 0) : _m_bOwns(ptr != 0), _m_ptr(ptr) {}
    TAutoPtr(const TAutoPtr& rhs)
        : _m_bOwns(rhs._m_bOwns), _m_ptr(rhs._m_ptr)
    {
        rhs._m_bOwns = 0;
    }
    ~TAutoPtr() { if (_m_bOwns) delete _m_ptr; }

    T* get() const { return _m_ptr; }
    T* operator->() const { return _m_ptr; }

private:
    mutable unsigned char _m_bOwns;
    T* _m_ptr;
};

#include "resourceptr.h"

// Retail ResourceManager's font-stream helper proves a second, pointer-only
// resource guard: its unwind state stores one pointer and releases it through
// resource::Dispose.  The Dreamcast TResourcePtr above is independently
// attested as the larger owns-byte-plus-pointer type, so keep the two surfaces
// distinct until a source name for this retail-only wrapper is recovered.
template<class T>
class TScopedResourcePtr {
public:
    TScopedResourcePtr(T* ptr = 0) : _m_ptr(ptr) {}
    ~TScopedResourcePtr()
    {
        if (_m_ptr)
            _m_ptr->Dispose();
    }

    T* get() const { return _m_ptr; }

private:
    T* _m_ptr;
};

#endif
