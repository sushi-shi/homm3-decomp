// autoarrayptr.h - canonical CodeView owner of TAutoArrayPtr.
#ifndef HOMM3_AUTOARRAYPTR_H
#define HOMM3_AUTOARRAYPTR_H

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
    mutable unsigned char m_owns;
    T* m_ptr;
};

#endif
