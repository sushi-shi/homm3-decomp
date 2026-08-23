// ownership.h - small header-only ownership wrappers used by game data loaders.
#ifndef HOMM3_OWNERSHIP_H
#define HOMM3_OWNERSHIP_H

template<class T>
class TAutoArrayPtr {
public:
    TAutoArrayPtr(T* ptr = 0) : _m_bOwns(ptr != 0), _m_ptr(ptr) {}
    TAutoArrayPtr(const TAutoArrayPtr& rhs)
        : _m_bOwns(rhs._m_bOwns), _m_ptr(rhs._m_ptr)
    {
        rhs._m_bOwns = 0;
    }
    ~TAutoArrayPtr() { if (_m_bOwns) delete [] _m_ptr; }

    TAutoArrayPtr& operator=(const TAutoArrayPtr& rhs)
    {
        if (_m_ptr != rhs._m_ptr) {
            if (_m_bOwns)
                delete [] _m_ptr;
            _m_bOwns = rhs._m_bOwns;
        } else if (rhs._m_bOwns) {
            _m_bOwns = 1;
        }
        _m_ptr = rhs._m_ptr;
        rhs._m_bOwns = 0;
        return *this;
    }

    T* get() const { return _m_ptr; }

private:
    mutable unsigned char _m_bOwns;
    T* _m_ptr;
};

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

template<class T>
class TResourcePtr {
public:
    TResourcePtr(T* ptr = 0) : _m_bOwns(ptr != 0), _m_ptr(ptr) {}
    TResourcePtr(const TResourcePtr& rhs)
        : _m_bOwns(rhs._m_bOwns), _m_ptr(rhs._m_ptr)
    {
        rhs._m_bOwns = 0;
    }
    ~TResourcePtr()
    {
        if (_m_bOwns && _m_ptr)
            _m_ptr->Dispose();
    }

    T* get() const { return _m_ptr; }
    T* operator->() const { return _m_ptr; }

private:
    mutable unsigned char _m_bOwns;
    T* _m_ptr;
};

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
