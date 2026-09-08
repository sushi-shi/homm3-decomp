// resourceptr.h - canonical CodeView owner of TResourcePtr.
#ifndef HOMM3_RESOURCEPTR_H
#define HOMM3_RESOURCEPTR_H

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
    mutable unsigned char m_owns;
    T* m_ptr;
};

#endif
