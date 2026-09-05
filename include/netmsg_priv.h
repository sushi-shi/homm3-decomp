// netmsg_priv.h - the growable in-memory TAbstractFile the wire-message
// bridge serializes through.
//
// PC-ONLY and absent from the Dreamcast roster; the class NAME is
// provisional, everything else is retail-proven. Its three-slot vftable at
// 0x640264 sits directly on TAbstractFile's shape - deleting destructor
// 0x512ad0, Read 0x512b30, Write 0x512b80 - and the destructor's closing
// `mov [this], 0x63dac0` is the base-class vptr restore that names the base.
// The four members come out of the two constructors t_complex_net_message's
// transports expand in place and out of Read/Write's own addressing.
//
// PRIVATE to netmsg.cpp on purpose: it is the only consumer, and the
// alternative - a class in netmsg.h - would perturb the include closure of
// the eighteen headers and twelve sources that carry the message ladder.
#ifndef HOMM3_NETMSG_PRIV_H
#define HOMM3_NETMSG_PRIV_H

#include <va.h>

#include "abstractfile.h"

class t_memory_file : public TAbstractFile {
public:
    // Both constructors are header inlines in the original: retail expands
    // the owning form into 0x512c80 / 0x512d40 (vptr, ownsBuffer, the
    // hundred-byte allocation, capacity, position) and the borrowing form
    // into 0x512e00, over a buffer it must not free.
    //
    // The four members are assigned in the BODY, not in a member-initialiser
    // list, and the difference is visible: an initialiser list emits the
    // derived vptr store AFTER the members, which leaves the base class's
    // own store live across the allocation call and gives TWO vptr stores.
    // Assigning in the body puts the two stores adjacent, VC6 drops the
    // base one, and what is left is retail's single `mov [this], 0x640264`
    // ahead of the flag.
    t_memory_file()
    {
        m_ownsBuffer = 1;
        m_buffer = new char[100];
        m_capacity = 100;
        m_position = 0;
    }
    t_memory_file(char* buffer, unsigned int capacity)
    {
        m_ownsBuffer = 0;
        m_buffer = buffer;
        m_capacity = capacity;
        m_position = 0;
    }
    virtual ~t_memory_file();
    virtual int Read(void* data, int size);
    virtual int Write(const void* data, int size);

    unsigned char m_ownsBuffer;  // +0x04
    char* m_buffer;              // +0x08
    // UNSIGNED, from the growth test's `jbe` in both members: a signed
    // `position + size > capacity` could only emit `jg`.
    unsigned int m_capacity;     // +0x0c
    unsigned int m_position;     // +0x10
};
SIZE(t_memory_file, 0x14);

#endif  /* HOMM3_NETMSG_PRIV_H */
