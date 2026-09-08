// autoarrayptr.h - canonical CodeView owner of TAutoArrayPtr.
#ifndef HOMM3_AUTOARRAYPTR_H
#define HOMM3_AUTOARRAYPTR_H

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

#endif
