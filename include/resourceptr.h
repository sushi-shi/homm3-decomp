// resourceptr.h - canonical CodeView owner of TResourcePtr.
#ifndef HOMM3_RESOURCEPTR_H
#define HOMM3_RESOURCEPTR_H

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

#endif
