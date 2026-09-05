// netmsg.cpp - the complex wire-message bridge: the serializable message
// base t_complex_net_message and the in-memory stream it serializes
// through.
//
// THIS COMPILAND IS ABSENT FROM THE DREAMCAST ROSTER, and its NAME is an
// inference the link order bounds rather than proves. Retail's .text is
// laid out in strict alphabetical compiland order - all 116 identified
// units are in order, mousemgr < multiplayerwindow < ... < newgame <
// objecttype < overview - so this object's name sorts strictly between
// `multiplayerwindow` and `newgame`. It implements the class DC homes in
// E:\gamedcs\netmsg.h, which is where the spelling comes from.
//
// The compiland's whole .text contribution is the nine bodies below,
// bracketed by two cinit runs of the same shape (32/89/96/97 bytes then
// eight ~95-byte initializers at 0x5126e0..0x512a70, and the same run
// again at 0x512ec0..0x513250). Those are the excluded initializer class
// and are not claimed.
#include <string.h>

#include <va.h>

#include "abstractfile.h"
#include "netmsg.h"
#include "netmsg_priv.h"
#include "remote.h"
#include "remotedlg.h"

// Retail 0x512b00. The vptr store, the owned-buffer release and the base
// class's own vptr restore - the whole body of an empty destructor on a
// class whose only owned resource is the buffer.
VA(0x00512b00, 0x24)  // anchor-vtable 0x640264 slot 0's callee; retail-only
t_memory_file::~t_memory_file()
{
    if (m_ownsBuffer) {
        delete[] m_buffer;
    }
}

// Retail 0x512b30, slot 1 of 0x640264. A short read is not an error: the
// count is clamped to what is left and answered back.
VA(0x00512b30, 0x43)  // anchor-vtable 0x640264 slot 1; retail-only
int t_memory_file::Read(void* data, int size)
{
    if (m_position + size > m_capacity) {
        size = m_capacity - m_position;
    }
    memcpy(data, m_buffer + m_position, size);
    m_position += size;
    return size;
}

// Retail 0x512b80, slot 2 of 0x640264. The growth step is a hundred bytes
// or the exact shortfall, whichever is larger, and the grown buffer is
// always owned - a borrowed one is copied out and left alone.
VA(0x00512b80, 0x9D)  // anchor-vtable 0x640264 slot 2; retail-only
int t_memory_file::Write(const void* data, int size)
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

// Retail 0x512c20 and 0x512c50: the two constructors, both of which reach
// the message image through CNetMsg's own two-argument constructor - the
// five stores come out in its body order (subType, -1, size, 0, 0) with
// the vptr store scheduled last. The default form's subtype is the ladder's
// first rung, 1000.
VA(0x00512c20, 0x22)  // anchor-vtable 0x640270; anchor-caller 0x5887a0
t_complex_net_message::t_complex_net_message()
    : netmsg(RS_GAME_TRANSMIT_INIT, 0)
{
}

VA(0x00512c50, 0x27)  // anchor-vtable 0x640270; anchor-caller 0x4aeb50
t_complex_net_message::t_complex_net_message(eRS_Messages subType)
    : netmsg(subType, 0)
{
}

// Retail 0x512c80 and 0x512d40, the two send halves. Both build a
// hundred-byte owning stream, put the twenty-byte message image in front of
// whatever the virtual write() appends, patch the total length back into
// the image's own `size` field - the serialized buffer IS a CNetMsg, which
// is what both transports take - and answer whether the transport accepted
// it. They differ only in which transport they call.
VA(0x00512c80, 0xBA)  // anchor-callee TransmitRemoteDataDPID; retail-only
unsigned char t_complex_net_message::RemoteFn_00512C80(
    unsigned long dpid, bool compressMsg, bool guaranteed)
{
    t_memory_file outfile;
    outfile.Write(&netmsg, sizeof(CNetMsg));
    write(&outfile);
    CNetMsg* wire = static_cast<CNetMsg*>(static_cast<void*>(outfile.m_buffer));
    wire->size = outfile.m_position;
    return TransmitRemoteDataDPID(wire, dpid, compressMsg, guaranteed) != 0;
}

VA(0x00512d40, 0xBA)  // anchor-callee TransmitRemoteData; retail-only
unsigned char t_complex_net_message::RemoteFn_00512D40(
    int toWho, bool compressMsg, bool guaranteed)
{
    t_memory_file outfile;
    outfile.Write(&netmsg, sizeof(CNetMsg));
    write(&outfile);
    CNetMsg* wire = static_cast<CNetMsg*>(static_cast<void*>(outfile.m_buffer));
    wire->size = outfile.m_position;
    return TransmitRemoteData(wire, toWho, compressMsg, guaranteed) != 0;
}

// Retail 0x512e00, the receive half. The stream borrows the received
// message's own bytes, the twenty-byte image is read back through the
// stream (which is what clamps a short message and leaves the read position
// on the payload), and the rest is the virtual read()'s.
VA(0x00512e00, 0xBF)  // anchor-vtable 0x640270 slot 0; retail-only
unsigned char t_complex_net_message::RemoteFn_00512E00(CNetMsg* pNetMsg)
{
    t_memory_file infile(static_cast<char*>(static_cast<void*>(pNetMsg)),
                         pNetMsg->size);
    infile.Read(&netmsg, sizeof(CNetMsg));
    // TWO returns, not one: retail duplicates the stream's scope-exit
    // teardown into both arms of the read() test rather than sharing a
    // tail, and the FAILURE arm is the fall-through - `jne` past the
    // `return 0` block to the `return 1` one.
    if (!read(&infile)) {
        return 0;
    }
    return 1;
}

// The compiler-generated deleting destructor, slot 0 of 0x640264.
VA_COMPGEN(0x00512ad0, 0x21, SCALAR_DELETING_DTOR, t_memory_file)
