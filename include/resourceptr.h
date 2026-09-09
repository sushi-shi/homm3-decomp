// resourceptr.h - canonical CodeView owner of TResourcePtr.
#ifndef HOMM3_RESOURCEPTR_H
#define HOMM3_RESOURCEPTR_H

#include <va.h>

class TTextResource;

template<class T>
class TResourcePtr {
public:
    TResourcePtr(T* ptr = 0) : m_owns(ptr != 0), m_ptr(ptr) {}
    // CodeView 0x57a9 declares this ordinary copy constructor, but has no
    // procedure/source location for its body. Current consumers construct
    // directly from pointers; retain the declaration without guessing a
    // transfer implementation from the other ownership operations.
    TResourcePtr(const TResourcePtr& rhs);
    // CodeView ResourcePtr.h:44, dc 0x5b294: ownership byte +0, pointer +4.
    // Retail 0x41bd90 keeps those tests and dispatches virtual dispose;
    // objnames' state-0 unwind at 0x627890 calls this retained instance.
    // Previously modelled separately as TTextResourceGuard.
    // VA instance: TResourcePtr<TTextResource>::~TResourcePtr
    VA(0x0041bd90, 0x12)  // anchor-eh 0x627890 for 0x41b500
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
