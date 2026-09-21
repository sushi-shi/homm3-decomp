// netmsg.cpp - the complex wire-message bridge: the serializable message
// base t_complex_net_message and the in-memory stream it serializes
// through.

// THIS COMPILAND IS ABSENT FROM THE DREAMCAST ROSTER, and its NAME is an
// inference the link order bounds rather than proves. Retail's .text is
// laid out in strict alphabetical compiland order - all 116 identified
// units are in order, mousemgr < multiplayerwindow < ... < newgame <
// objecttype < overview - so this object's name sorts strictly between
// `multiplayerwindow` and `newgame`. It serializes CNetMsg, which DC homes
// in E:\gamedcs\netmsg.h; the new complex-message base itself is not named
// by CodeView. That header supplies the inferred module spelling.

// The compiland's whole .text contribution is the nine bodies below,
// bracketed by two cinit runs of the same shape (32/89/96/97 bytes then
// eight ~95-byte initializers at 0x5126e0..0x512a70, and the same run
// again at 0x512ec0..0x513250). Those are the excluded initializer class
// and are not claimed.
#include <string.h>

#include "netmsg.h"

#include "abstractfile.h"
#include "remote.h"
#include "remotedlg.h"
#include "va.h"

// Complete adds this module-local memory stream to the wire-message bridge.
// The class name is provisional. Retail vtable 0x640264 proves the three
// TAbstractFile slots; constructor expansions at 0x512cad and 0x512e28 prove
// the owned and borrowed forms. No original header location is established.
class t_memory_file : public TAbstractFile {
public:
    // Both constructors are defined inline in this module-local class.
    // Retail expands
    // the owning form into 0x512c80 / 0x512d40 (vptr, ownsBuffer, the
    // hundred-byte allocation, capacity, position) and the borrowing form
    // into 0x512e00, over a buffer it must not free.

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
    virtual int read(void* data, int size);
    virtual int write(const void* data, int size);

    unsigned char m_ownsBuffer;  // +0x04
    char* m_buffer;              // +0x08
    // UNSIGNED, from the growth test's `jbe` in both members: a signed
    // `position + size > capacity` could only emit `jg`.
    unsigned int m_capacity;     // +0x0c
    unsigned int m_position;     // +0x10
};
SIZE(t_memory_file, 0x14);

VA(0x00512b00, 0x24)
t_memory_file::~t_memory_file()
{
    if (m_ownsBuffer) {
        delete[] m_buffer;
    }
}

VA(0x00512b30, 0x43)
int t_memory_file::read(void* data, int size)
{
    if (m_position + size > m_capacity) {
        size = m_capacity - m_position;
    }
    memcpy(data, m_buffer + m_position, size);
    m_position += size;
    return size;
}

VA(0x00512b80, 0x9D)
int t_memory_file::write(const void* data, int size)
{
    if (m_position + size > m_capacity) {
        unsigned int grown = m_capacity + 100;
        if (m_position + size > grown) {
            grown = m_position + size;
        }
        char* buffer = new char[grown];
        memcpy(buffer, m_buffer, m_position);
        if (m_ownsBuffer) {
            delete[] m_buffer;
        }
        m_buffer = buffer;
        m_ownsBuffer = 1;
        m_capacity = grown;
    }
    memcpy(m_buffer + m_position, data, size);
    m_position += size;
    return size;
}

VA(0x00512c20, 0x22)  // anchor-vtable 0x640270; anchor-caller 0x5887a0
t_complex_net_message::t_complex_net_message()
    : m_netmsg(RS_GAME_TRANSMIT_INIT, 0)
{
}

VA(0x00512c50, 0x27)  // anchor-vtable 0x640270; anchor-caller 0x4aeb50
t_complex_net_message::t_complex_net_message(eRS_Messages subType)
    : m_netmsg(subType, 0)
{
}

VA(0x00512c80, 0xBA)
unsigned char t_complex_net_message::remoteFn00512C80(
    unsigned long dpid, bool compressMsg, bool guaranteed)
{
    t_memory_file outfile;
    outfile.write(&m_netmsg, sizeof(CNetMsg));
    write(&outfile);
    CNetMsg* wire = static_cast<CNetMsg*>(static_cast<void*>(outfile.m_buffer));
    wire->m_size = outfile.m_position;
    return transmitRemoteDataDPID(wire, dpid, compressMsg, guaranteed) != 0;
}

VA(0x00512d40, 0xBA)
unsigned char t_complex_net_message::remoteFn00512D40(
    int toWho, bool compressMsg, bool guaranteed)
{
    t_memory_file outfile;
    outfile.write(&m_netmsg, sizeof(CNetMsg));
    write(&outfile);
    CNetMsg* wire = static_cast<CNetMsg*>(static_cast<void*>(outfile.m_buffer));
    wire->m_size = outfile.m_position;
    return transmitRemoteData(wire, toWho, compressMsg, guaranteed) != 0;
}

VA(0x00512e00, 0xBF)
unsigned char t_complex_net_message::remoteFn00512E00(CNetMsg* netMsg)
{
    t_memory_file infile(static_cast<char*>(static_cast<void*>(netMsg)),
                         netMsg->m_size);
    infile.read(&m_netmsg, sizeof(CNetMsg));
    if (!read(&infile)) {
        return 0;
    }
    return 1;
}

VA_COMPGEN(0x00512ad0, 0x21, SCALAR_DELETING_DTOR, t_memory_file)
