// autoarrayptr.h - canonical CodeView owner of TAutoArrayPtr.
#ifndef HOMM3_AUTOARRAYPTR_H
#define HOMM3_AUTOARRAYPTR_H

#include "va.h"

template<class T>
class TAutoArrayPtr {
public:

    // Loki h3maped ctor/get/release have empty exception specifications
    // (__check_eh_spec with a zero type list at 0x833fe0a, 0x83401a3,
    // and 0x8340225). The destructor has no such specification.
    DC_ADDRESS(0x05b1f8, 0xe)
    TAutoArrayPtr(T* ptr = 0) throw() : _m_bOwns(ptr != 0), _m_ptr(ptr) {}
    // CodeView 0x57b9 declares this ordinary copy constructor, but has no
    // procedure/source location for its body. Current consumers construct
    // directly from pointers; retain the declaration without guessing a
    // transfer implementation from the other ownership operations.
    TAutoArrayPtr(const TAutoArrayPtr& rhs);

    DC_ADDRESS(0x05b208, 0x20)
    ~TAutoArrayPtr() { if (_m_bOwns) delete [] _m_ptr; }

    // Loki's h3maped form: an empty exception specification, the
    // self-assignment test and release().
    DC_ADDRESS(0x05b228, 0x58)
    TAutoArrayPtr& operator=(const TAutoArrayPtr& rhs) throw()
    {
        if (this != &rhs) {
            if (_m_ptr != rhs._m_ptr) {
                if (_m_bOwns)
                    delete [] _m_ptr;
                _m_bOwns = rhs._m_bOwns;
            } else if (rhs._m_bOwns) {
                _m_bOwns = 1;
            }
            _m_ptr = rhs.release();
        }
        return *this;
    }

    DC_ADDRESS(0x05b280, 0x4)
    T* get() const throw() { return _m_ptr; }

    // Original: TAutoArrayPtr<char>::release; AutoArrayPtr.h:77
    DC_ADDRESS(0x05b2d4, 0x8)
    T* release() const throw() { _m_bOwns = 0; return _m_ptr; }

private:
    // Original names: Dreamcast CodeView 0x57b7, offsets 0 and 4.
    // Primitive 0x20 does not distinguish unsigned char from lowered bool;
    // both reproduce the observed ownership byte operations.
    mutable unsigned char _m_bOwns;
    T* _m_ptr;
};

#endif
