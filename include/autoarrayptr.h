// autoarrayptr.h - canonical CodeView owner of TAutoArrayPtr.
#ifndef HOMM3_AUTOARRAYPTR_H
#define HOMM3_AUTOARRAYPTR_H

template<class T>
// Before normalization (type): TAutoArrayPtr.
#ifndef AutoArrayPtr
#define AutoArrayPtr TAutoArrayPtr
#endif
class AutoArrayPtr {
public:
    AutoArrayPtr(T* ptr = 0) : m_owns(ptr != 0), m_ptr(ptr) {}
    // CodeView 0x57b9 declares this ordinary copy constructor, but has no
    // procedure/source location for its body. Current consumers construct
    // directly from pointers; retain the declaration without guessing a
    // transfer implementation from the other ownership operations.
    AutoArrayPtr(const AutoArrayPtr& rhs);
    ~AutoArrayPtr() { if (m_owns) delete [] m_ptr; }
    AutoArrayPtr& operator=(const AutoArrayPtr& rhs)
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
