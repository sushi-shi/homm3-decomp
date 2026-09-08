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
    TAutoPtr(T* ptr = 0) : m_owns(ptr != 0), m_ptr(ptr) {}
    TAutoPtr(const TAutoPtr& rhs)
        : m_owns(rhs.m_owns), m_ptr(rhs.m_ptr)
    {
        rhs.m_owns = 0;
    }
    ~TAutoPtr() { if (m_owns) delete m_ptr; }

    T* get() const { return m_ptr; }
    T* operator->() const { return m_ptr; }

private:
    // Before normalization: _m_bOwns.
    mutable unsigned char m_owns;
    // Before normalization: _m_ptr.
    T* m_ptr;
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
    TScopedResourcePtr(T* ptr = 0) : m_ptr(ptr) {}
    ~TScopedResourcePtr()
    {
        if (m_ptr)
            m_ptr->dispose();
    }

    T* get() const { return m_ptr; }

private:
    // Before normalization: _m_ptr.
    T* m_ptr;
};

#endif
