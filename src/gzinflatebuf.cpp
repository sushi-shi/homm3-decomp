// gzinflatebuf.cpp - the gzip-inflating streambuf Complete reads .h3c
// campaign payloads and compressed maps through.
//
// THIS COMPILAND IS ABSENT FROM THE DREAMCAST ROSTER. Retail's object is
// the one the cinit run 0x4d5f70..0x4d5fb0 opens and the next unit's cinit
// at 0x4d6c30 (gzfile.obj) closes, which brackets exactly the nine bodies
// below. It sits one object ahead of gzfile.obj in the
// gametypewindow..hero link-order bracket.
//
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
//
// The header-check failures throw a plain `bool` (throw record 0x64e1b8
// names the type `._N`) and are caught in the constructor itself, which is
// what leaves the stream in raw pass-through mode with ok == 0.
#include <va.h>
// Retail's basic_streambuf constructor calls std::_Lockit around
// _Locimp::_Init; that is the external-lock view of <yvals.h>, so expose it
// while this TU is parsed exactly as game.obj does. The pinned /ML runtime
// is unchanged.
#if defined(_MSC_VER) && !defined(__clang__)
#define _MT
#endif
#include <stdexcept>
#include <string>

#include "exceptions.h"
#include "gzinflatebuf.h"

// Retail .rdata 0x63e6fc, immediately ahead of this unit's two vftables.
// zlib's own gzio.c spells the pair exactly this way, and the constructor
// LOADS both rather than testing immediates, which is what proves it is a
// table rather than two literals.
DATA(0x0063e6fc) static int gz_magic[2] = {0x1f, 0x8b};

// 0x4d5fd0: refill next_in from the source streambuf when it is empty and
// hand back the next byte, or -1 at end of source.
VA(0x004d5fd0, 0x74)  // anchor-bracket, retail-only
int TGzInflateBuf::get_byte()
{
    if (stream.avail_in == 0) {
        if (source_eof)
            return -1;
        int count = source->sgetn(
            static_cast<char*>(static_cast<void*>(buffer)), 0x200);
        if (count < 0x200)
            source_eof = 1;
        stream.next_in = buffer;
        stream.avail_in = count;
        if (count == 0)
            return -1;
    }
    unsigned char c = *stream.next_in;
    ++stream.next_in;
    --stream.avail_in;
    return c;
}

// 0x4d6050: build the window, then walk the gzip member header exactly as
// zlib's gzio.c check_header does. A failed magic pair is caught here and
// demotes the stream to raw pass-through (ok = 0) rather than propagating.
VA(0x004d6050, 0x58A)  // anchor-vtable ??_7TGzInflateBuf@@6B@ + anchor-import @inflateInit2_@16, retail-only
TGzInflateBuf::TGzInflateBuf(std::streambuf* newSource)
    : source(newSource),
      buffer(0),
      out_buffer(0),
      crc(crc32(0, 0, 0)),
      ok(1),
      source_eof(0),
      inflating(0)
{
    buffer = new unsigned char[0x400];
    if (buffer == 0)
        throw TAllocationFailure();
    out_buffer = buffer + 0x200;
    setg(static_cast<char*>(static_cast<void*>(out_buffer)),
         static_cast<char*>(static_cast<void*>(out_buffer)),
         static_cast<char*>(static_cast<void*>(out_buffer)));
    setp(0, 0);
    stream.next_out = out_buffer;
    stream.next_in = buffer;
    stream.avail_in = 0;
    stream.avail_out = 0x200;
    stream.zalloc = 0;
    stream.zfree = 0;
    try {
        int magic = get_byte();
        if (magic == -1)
            throw false;
        if (magic != gz_magic[0])
            throw false;
        magic = get_byte();
        if (magic == -1)
            throw false;
        if (magic != gz_magic[1]) {
            --stream.next_in;
            ++stream.avail_in;
            throw false;
        }
    } catch (bool) {
        ok = 0;
    }
    if (ok) {
        int method = read_byte();
        if (method != Z_DEFLATED)
            throw TDataError(std::string());
        int flags = read_byte();
        if ((flags & 0xe0) != 0)
            throw TDataError(std::string());
        int skip = 6;
        do {
            read_byte();
        } while (--skip > 0);
        if ((flags & 4) != 0) {
            int low = read_byte();
            unsigned extra = (read_byte() << 8) + low;
            while (extra-- != 0)
                read_byte();
        }
        if ((flags & 8) != 0) {
            while (read_byte() != 0) {
            }
        }
        if ((flags & 0x10) != 0) {
            while (read_byte() != 0) {
            }
        }
        if ((flags & 2) != 0) {
            read_byte();
            if (get_byte() == -1)
                throw TDataError(std::string());
        }
        if (inflateInit2(&stream, -MAX_WBITS) == Z_MEM_ERROR)
            throw TAllocationFailure();
        inflating = 1;
    }
}

// 0x4d65e0: the message-less form. `std::runtime_error`'s inline string
// constructor expands into it, which is the whole 175-byte body.
VA(0x004d65e0, 0xAF)  // anchor-bracket, called from 0x4d6050 / 0x4d6920, retail-only
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
VA(0x004d6820, 0xF6)  // anchor-import @inflateEnd@4, retail-only
TGzInflateBuf::~TGzInflateBuf()
{
    if (stream.avail_in > 0) {
        source->pubseekoff(
            -static_cast<long>(stream.avail_in),
            std::ios_base::cur, std::ios_base::in);
    }
    if (ok) {
        if (inflating)
            inflateEnd(&stream);
    } else if (egptr() > gptr()) {
        source->pubseekoff(
            gptr() - egptr(), std::ios_base::cur, std::ios_base::in);
    }
    delete buffer;
}

// 0x4d6920: drain the source into the 0x200-byte output half, either
// through inflate or, for a non-gzip member, by straight copy.
VA(0x004d6920, 0x251)  // anchor-vtable ??_7TGzInflateBuf@@6B@ slot 4 + anchor-import @inflate@8, retail-only
int TGzInflateBuf::underflow()
{
    while (stream.avail_out > 0) {
        if (stream.avail_in == 0 && source_eof)
            break;
        if (stream.avail_in == 0) {
            int count = source->sgetn(
                static_cast<char*>(static_cast<void*>(buffer)), 0x200);
            if (count < 0x200)
                source_eof = 1;
            stream.avail_in = count;
            stream.next_in = buffer;
        }
        if (stream.avail_in > 0) {
            if (ok) {
                if (inflating) {
                    int status = inflate(&stream, Z_SYNC_FLUSH);
                    if (status == Z_MEM_ERROR)
                        throw TAllocationFailure();
                    if (status == Z_DATA_ERROR)
                        throw TDataError();
                    crc = crc32(crc,
                                stream.next_out + stream.avail_out - 0x200,
                                0x200 - stream.avail_out);
                    if (status == Z_STREAM_END) {
                        inflateEnd(&stream);
                        inflating = 0;
                        read_byte();
                        read_byte();
                        read_byte();
                        read_byte();
                        read_byte();
                        read_byte();
                        read_byte();
                        read_byte();
                        break;
                    }
                }
            } else {
                unsigned count = stream.avail_in;
                if (count > stream.avail_out)
                    count = stream.avail_out;
                memcpy(stream.next_out, stream.next_in, count);
                stream.next_in += count;
                stream.avail_in -= count;
                stream.next_out += count;
                stream.avail_out -= count;
            }
        }
    }
    setg(static_cast<char*>(static_cast<void*>(out_buffer)),
         static_cast<char*>(static_cast<void*>(out_buffer)),
         static_cast<char*>(static_cast<void*>(out_buffer))
             + 0x200 - stream.avail_out);
    stream.next_out = out_buffer;
    stream.avail_out = 0x200;
    if (egptr() > eback())
        return static_cast<unsigned char>(*gptr());
    return -1;
}

// 0x4d6b80: `TRuntimeError`'s `const char*` constructor is the out-of-line
// 0x49a0c0 call; this body is the vftable swap on top of it.
VA(0x004d6b80, 0x17)  // anchor-callee 0x49a0c0 + anchor-vtable 0x63aba8, retail-only
TAllocationFailure::TAllocationFailure()
    : TRuntimeError("Allocation failure.")
{
}

// 0x4d6ba0: get_byte with the malformed-member throw attached.
VA(0x004d6ba0, 0x81)  // anchor-bracket, called from 0x4d6920, retail-only
int TGzInflateBuf::read_byte()
{
    int c = get_byte();
    if (c == -1)
        throw TDataError();
    return c;
}
