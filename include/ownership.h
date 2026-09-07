// ownership.h - small header-only ownership wrappers used by game data loaders.
#ifndef HOMM3_OWNERSHIP_H
#define HOMM3_OWNERSHIP_H

template<class T>
class TAutoArrayPtr {
public:
    TAutoArrayPtr(T* ptr = 0) : m_owns(ptr != 0), m_ptr(ptr) {}
    TAutoArrayPtr(const TAutoArrayPtr& rhs)
        : m_owns(rhs.m_owns), m_ptr(rhs.m_ptr)
    {
        rhs.m_owns = 0;
    }
    ~TAutoArrayPtr() { if (m_owns) delete [] m_ptr; }

    TAutoArrayPtr& operator=(const TAutoArrayPtr& rhs)
    {
        if (m_ptr != rhs.m_ptr) {
            if (m_owns)
                delete [] m_ptr;
            m_owns = rhs.m_owns;
        } else if (rhs.m_owns) {
            m_owns = 1;
        }
        m_ptr = rhs.m_ptr;
        rhs.m_owns = 0;
        return *this;
    }

    T* get() const { return m_ptr; }

private:
    // Before normalization: _m_bOwns.
    mutable unsigned char m_owns;
    // Before normalization: _m_ptr.
    T* m_ptr;
};

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

template<class T>
class TResourcePtr {
public:
    TResourcePtr(T* ptr = 0) : m_owns(ptr != 0), m_ptr(ptr) {}
    TResourcePtr(const TResourcePtr& rhs)
        : m_owns(rhs.m_owns), m_ptr(rhs.m_ptr)
    {
        rhs.m_owns = 0;
    }
    ~TResourcePtr()
    {
        if (m_owns && m_ptr)
            m_ptr->dispose();
    }

    T* get() const { return m_ptr; }
    T* operator->() const { return m_ptr; }

private:
    // Before normalization: _m_bOwns.
    mutable unsigned char m_owns;
    // Before normalization: _m_ptr.
    T* m_ptr;
};

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
