// gzinflatebuf.cpp - the gzip-inflating streambuf Complete reads .h3c
// campaign payloads and compressed maps through.

// THIS COMPILAND IS ABSENT FROM THE DREAMCAST ROSTER. Retail's object is
// the one the cinit run 0x4d5f70..0x4d5fb0 opens and the next unit's cinit
// at 0x4d6c30 (gzfile.obj) closes, which brackets exactly the nine bodies
// below. It sits one object ahead of gzfile.obj in the
// gametypewindow..hero link-order bracket.

// EVERY NAME HERE IS RETAIL-PROVEN RTTI, not a guess: the two throw
// records this unit references spell `.?AVTDataError@TGzInflateBuf@@` over
// `.?AVruntime_error@std@@` (0x64e1a8) and `.?AVTAllocationFailure@@` over
// `.?AVTRuntimeError@@` over `.?AVTDebugBreak@@` over
// `.?AVruntime_error@std@@` (0x6486c0). That settles three things the
// disassembly alone could not: the 0x63e704 / 0x645640 / 0x63aba8 vftables
// are Dinkumware's three-slot {deleting dtor, what, _Doraise} shape, the
// 354-byte body at 0x41ba90 is `std::runtime_error::runtime_error(const
// string&)`'s COMDAT, and the 249-byte body at 0x49a0c0 - carried as
// "substantial, unidentified" in dxplay.cpp's next-lane note - is
// `TRuntimeError::TRuntimeError(const char*)`, not a dxplay method.

// The header-check failures throw a plain `bool` (throw record 0x64e1b8
// names the type `._N`) and are caught in the constructor itself, which is
// what leaves the stream in raw pass-through mode with ok == 0.
// Retail's basic_streambuf constructor calls std::_Lockit around
// _Locimp::_Init; the /MT game profile exposes that external-lock view.
#include "va.h"

#include <stdexcept>
#include <string>

#include "gzinflatebuf.h"

#include "autoarrayptr.h"
#include "exceptions.h"

class TGzInflateBuf::TDataError : public std::runtime_error {
public:
    TDataError();
};

// Retail .rdata 0x63e6fc, immediately ahead of this unit's two vftables.
// zlib's own gzio.c spells the pair exactly this way, and the constructor
// LOADS both rather than testing immediates, which is what proves it is a
// table rather than two literals.
DATA(0x0063e6fc) static int g_gzMagic[2] = {0x1f, 0x8b};

// 0x4d5fd0: refill next_in from the source streambuf when it is empty and
// hand back the next byte, or -1 at end of source.
VA(0x004d5fd0, 0x74) MAC_ADDRESS(0x220a18, 0xb0)
int TGzInflateBuf::getByte()
{
    if (m_stream.avail_in == 0) {
        if (m_sourceEof)
            return -1;
        int count = m_source->sgetn(
            static_cast<char*>(static_cast<void*>(m_buffer)), 0x200);
        if (count < 0x200)
            m_sourceEof = 1;
        m_stream.next_in = m_buffer;
        m_stream.avail_in = count;
        if (count == 0)
            return -1;
    }
    unsigned char c = *m_stream.next_in;
    ++m_stream.next_in;
    --m_stream.avail_in;
    return c;
}

// CodeWarrior retains this helper immediately after getByte and calls it
// twice from the constructor's gzip-magic fallback. Its byte argument is
// passed at both call sites but the helper only rewinds the input cursor.
MAC_ADDRESS(0x220ac8, 0x1c)
void TGzInflateBuf::ungetByte(signed char)
{
    --m_stream.next_in;
    ++m_stream.avail_in;
}

// 0x4d6050: build the window, then walk the gzip member header exactly as
// zlib's gzio.c check_header does. A failed magic pair is caught here and
// demotes the stream to raw pass-through (ok = 0) rather than propagating.

// The temporary buffer owner is the ordinary TAutoArrayPtr: Mac stores
// owns/pointer at stack+0x268/+0x26c, clears owns on release, and calls
// array delete at 0x2212f4. Windows unwind 0x62c913 -> 0x4b7040 uses
// the same flag/pointer pair; that body alone did not distinguish auto_ptr.

// The header walk's two LOOP FORMS, 87.7808 -> 91.6008 in two doses, and
// the second only pays after the first:
//  - the six reserved bytes are a counted `for (int skip = 6; skip > 0;
//    --skip)`, not `do { } while (--skip > 0)`. Measured: for-down 90.3211,
//    for-up / while-up / braced-for 89.8207 all three to the digit,
//    do-while(--skip != 0) 87.7038, six unrolled read_byte() calls 74.9020.
//  - the two name/comment skips are then `while (1) { if (read_byte() == 0)
//    break; }`. Written `while (read_byte() != 0) { }` VC6 PEELS the
//    condition - it emits read_byte once as the guard and again at the
//    bottom - which is why the call census carried one surplus get_byte.
//    This is wingraph DDBlit's lever, and it MEASURED NEGATIVE before the
//    skip loop moved (85.93 for both, 86.83/86.89 for either alone against
//    an 87.78 baseline) and +1.28 after it: a rejected knob is only
//    rejected for the inline structure it was measured in.

VA(0x004d6050, 0x58A) MAC_ADDRESS(0x220ae4, 0x82c)  // anchor-vtable ??_7TGzInflateBuf@@6B@ + anchor-import @inflateInit2_@16, retail-only
TGzInflateBuf::TGzInflateBuf(std::streambuf* newSource)
    : m_source(newSource),
      m_buffer(0),
      m_outBuffer(0),
      m_crc(crc32(0, 0, 0)),
      m_ok(1),
      m_sourceEof(0),
      m_inflating(0)
{
    m_buffer = new unsigned char[0x400];
    if (m_buffer == 0)
        throw TAllocationFailure();
    TAutoArrayPtr<unsigned char> ownedBuffer(m_buffer);
    m_outBuffer = m_buffer + 0x200;
    setg(static_cast<char*>(static_cast<void*>(m_outBuffer)),
         static_cast<char*>(static_cast<void*>(m_outBuffer)),
         static_cast<char*>(static_cast<void*>(m_outBuffer)));
    setp(0, 0);
    m_stream.next_out = m_outBuffer;
    m_stream.next_in = m_buffer;
    m_stream.avail_in = 0;
    m_stream.avail_out = 0x200;
    m_stream.zalloc = 0;
    m_stream.zfree = 0;
    try {
        int magic = getByte();
        if (magic == -1)
            throw false;
        try {
            if (magic != g_gzMagic[0])
                throw false;
            // Mac preserves the first byte in r20 for the catch below;
            // the second byte in r19 is passed to ungetByte at 0x220d0c.
            int nextMagic = getByte();
            if (nextMagic == -1)
                throw false;
            if (nextMagic != g_gzMagic[1]) {
                ungetByte(static_cast<signed char>(nextMagic));
                throw false;
            }
        } catch (bool) {
            ungetByte(static_cast<signed char>(magic));
            throw;
        }
    } catch (bool) {
        m_ok = 0;
    }
    if (m_ok) {
        int method = readByte();
        if (method != Z_DEFLATED)
            throw TDataError();
        int flags = readByte();
        if ((flags & 0xe0) != 0)
            throw TDataError();
        for (int skip = 6; skip > 0; --skip)
            readByte();
        if ((flags & 4) != 0) {
            int low = readByte();
            unsigned extra = (readByte() << 8) + low;
            while (extra-- != 0)
                readByte();
        }
        if ((flags & 8) != 0) {
            while (1) {
                if (readByte() == 0)
                    break;
            }
        }
        if ((flags & 0x10) != 0) {
            while (1) {
                if (readByte() == 0)
                    break;
            }
        }
        if ((flags & 2) != 0) {
            readByte();
            readByte();
        }
        if (inflateInit2(&m_stream, -MAX_WBITS) == Z_MEM_ERROR)
            throw TAllocationFailure();
        m_inflating = 1;
    }
    ownedBuffer.release();
}

// 0x4d65e0: the message-less form. `std::runtime_error`'s inline string
// constructor expands into it, which is the whole 175-byte body.
// Mac expands this constructor: its retained 0x221994 body is the
// runtime_error(string) base constructor: r4 is a prebuilt string, copied
// at 0x2219c0. Callers destroy the temporary then install the derived vptr
// (e.g. 0x221660 -> 0x22166c -> 0x22167c).
VA(0x004d65e0, 0xAF)
TGzInflateBuf::TDataError::TDataError()
    : std::runtime_error(std::string())
{
}

// __CxxThrowException's catchable-type record for the tag; the copy is what
// the throw makes into the exception object.
VA_COMPGEN(0x004d6690, 0x157, IMPLICIT_COPY_CTOR, TDataError)

VA_COMPGEN(0x004d67f0, 0x21, SCALAR_DELETING_DTOR, TGzInflateBuf)

// 0x4d6820: hand the source stream back whatever this object read ahead -
// the raw bytes still in next_in, or, when the member was never a gzip
// member, the undrained tail of the output window.
VA(0x004d6820, 0xF6) MAC_ADDRESS(0x221394, 0x12c)
TGzInflateBuf::~TGzInflateBuf()
{
    if (m_stream.avail_in > 0) {
        m_source->pubseekoff(
            -static_cast<long>(m_stream.avail_in),
            std::ios_base::cur, std::ios_base::in);
    }
    if (m_ok) {
        if (m_inflating)
            inflateEnd(&m_stream);
    } else if (egptr() > gptr()) {
        m_source->pubseekoff(
            gptr() - egptr(), std::ios_base::cur, std::ios_base::in);
    }
    delete[] m_buffer;
}

// 0x4d6920: drain the source into the 0x200-byte output half, either
// through inflate or, for a non-gzip member, by straight copy.

// The FIRST guard reads `avail_in <= 0`, not `== 0`: retail inverts it to
// `ja` (unsigned above) where `== 0` can only ever emit `jne`, and the two
// are the same test on zlib's `uInt`. 80.8063 -> 81.1188. The second guard
// really is `== 0` - retail emits `jne` there.

// Retail's raw-copy minimum uses a strict unsigned comparison: spelling
// avail_in < avail_out ? avail_in : avail_out restores jb and raises this
// caller from 81.1204% to 81.4346%. std::_cpp_min on copied local counts
// adds operand homes and scores 79.1832%; keep the direct value expression.

// Residual: retail expands readByte at trailer positions 1,2,5,6 and calls
// it at 3,4,7,8. The current body expands all eight. A passive VC6 trace
// measures caller cb=604, budget=1208, and readByte cb=56 before this fix.
// Two provisional four-byte readers, following vendored gzio.c::getLong,
// give a 3-expanded/1-called first group and a 2/2 second group (26.1623%).
// Addition, OR and separate byte locals emit identical bytes. Combining
// that helper with std::_cpp_min produces both 2/2 groups but moves the
// error blocks and adds reference-selection loads (24.4660%; local count
// copies 29.0419%). No such helper is retained: the repeated pattern is a
// hypothesis for its boundary, not proof of the original reader body.
// The candidate also shares its 0x20 exception slot where retail reserves
// 0x3c. An explicit refill-buffer local is byte-neutral; a separate CRC
// byte-count local scores 81.3560% and does not resolve the trailer calls.
VA(0x004d6920, 0x251) MAC_ADDRESS(0x2214c0, 0x4d4)  // anchor-vtable ??_7TGzInflateBuf@@6B@ slot 4 + anchor-import @inflate@8, retail-only
int TGzInflateBuf::underflow()
{
    while (m_stream.avail_out > 0) {
        if (m_stream.avail_in <= 0 && m_sourceEof)
            break;
        if (m_stream.avail_in == 0) {
            int count = m_source->sgetn(
                static_cast<char*>(static_cast<void*>(m_buffer)), 0x200);
            if (count < 0x200)
                m_sourceEof = 1;
            m_stream.avail_in = count;
            m_stream.next_in = m_buffer;
        }
        if (m_stream.avail_in > 0) {
            if (m_ok) {
                if (m_inflating) {
                    int status = inflate(&m_stream, Z_SYNC_FLUSH);
                    if (status == Z_MEM_ERROR)
                        throw TAllocationFailure();
                    if (status == Z_DATA_ERROR)
                        throw TDataError();
                    // This is the live z_stream output window's beginning.
                    // Retail +0x9b..+0xb3 loads next_out/avail_out and forms
                    // their sum minus 512, rather than loading m_outBuffer.
                    // Preserve that stream-state dependency, not a guessed
                    // cancellation through the separately cached pointer.
                    m_crc = crc32(m_crc,
                                m_stream.next_out + m_stream.avail_out - 0x200,
                                0x200 - m_stream.avail_out);
                    if (status == Z_STREAM_END) {
                        inflateEnd(&m_stream);
                        m_inflating = 0;
                        readByte();
                        readByte();
                        readByte();
                        readByte();
                        readByte();
                        readByte();
                        readByte();
                        readByte();
                        break;
                    }
                }
            } else {
                unsigned count = m_stream.avail_in < m_stream.avail_out
                    ? m_stream.avail_in : m_stream.avail_out;
                memcpy(m_stream.next_out, m_stream.next_in, count);
                m_stream.next_in += count;
                m_stream.avail_in -= count;
                m_stream.next_out += count;
                m_stream.avail_out -= count;
            }
        }
    }
    setg(static_cast<char*>(static_cast<void*>(m_outBuffer)),
         static_cast<char*>(static_cast<void*>(m_outBuffer)),
         static_cast<char*>(static_cast<void*>(m_outBuffer))
             + 0x200 - m_stream.avail_out);
    m_stream.next_out = m_outBuffer;
    m_stream.avail_out = 0x200;
    if (egptr() > eback())
        return static_cast<unsigned char>(*gptr());
    return -1;
}

// 0x4d6b80 calls TRuntimeError(const char*) at 0x49a0c0, then installs
// the derived vptr. Both its canonical body and VA are in exceptions.h;
// this object retains the out-of-line copy and calls it, while the
// objnames throw expands the shared derived initialization.

// 0x4d6ba0: get_byte with the malformed-member throw attached.
VA(0x004d6ba0, 0x81)
int TGzInflateBuf::readByte()
{
    int c = getByte();
    if (c == -1)
        throw TDataError();
    return c;
}

VA_COMPGEN(0x0041ba90, 0x162, CLASS_CTOR, runtime_error)
VA_COMPGEN(0x0041b7b0, 0x169, IMPLICIT_COPY_CTOR, TRuntimeError)
VA_COMPGEN(0x0041b920, 0x16F, IMPLICIT_COPY_CTOR, TAllocationFailure)
