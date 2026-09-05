// gzinflatebuf.h - the gzip-inflating streambuf and its TAbstractFile
// adapter that Complete's campaign loader reads .h3c payloads through.
//
// NEITHER CLASS HAS A DREAMCAST ROW. TGzInflateBuf's bodies sit in the
// gametypewindow..hero link-order bracket (the compiland opened by cinit
// 0x4d5f70..0x4d5fb0: 0x4d5fd0 get-byte helper, 0x4d6050 constructor,
// 0x4d65e0 error ctor, 0x4d6690, 0x4d67f0 scalar deleting dtor,
// 0x4d6820 destructor, 0x4d6920 underflow, 0x4d6b80 / 0x4d6ba0), one
// object ahead of TGzFile's (savegame.h). TStreamBufFile's two virtual
// bodies are emitted at the head of customcampaign.obj (0x483f10 /
// 0x483f30), which is where they are claimed. Names PROVISIONAL, taken
// from the decorated vftable symbols retail keeps (??_7TGzInflateBuf@@6B@
// at 0x63e710, ??_7TStreamBufFile@@6B@ at 0x63dacc).
#ifndef HOMM3_GZINFLATEBUF_H
#define HOMM3_GZINFLATEBUF_H

#include <stdexcept>
#include <streambuf>
#include <zlib.h>

#include "abstractfile.h"

// A std::streambuf that inflates a gzip member out of another streambuf.
// LAYOUT BYTE-PROVEN by the constructor 0x4d6050 and destructor 0x4d6820:
// the Dinkumware basic_streambuf<char> base is 0x38 (its locale at +0x34),
// the source buffer pointer sits at +0x38, zlib 1.1.3's 56-byte z_stream
// at +0x3c (next_in/avail_in at +0x3c/+0x40 are what the get-byte helper
// 0x4d5fd0 refills; the destructor calls inflateEnd on &this[0x3c]), the
// 0x400-byte work buffer at +0x74 with its output half at +0x78, the
// running crc32 at +0x7c and three status bytes from +0x80. Every caller
// gives the object exactly 0x84 stack bytes (customcampaign's two loaders
// put it at ebp-0xb0 and ebp-0xd0 with the next local 0x84 above).
// The vftable is basic_streambuf's thirteen slots with only the deleting
// destructor and underflow overridden.
class TGzInflateBuf : public std::streambuf {
public:
    TGzInflateBuf(std::streambuf* source);  // 0x4d6050
    virtual ~TGzInflateBuf();               // 0x4d6820
    virtual int underflow();                // 0x4d6920

    // Thrown out of the constructor and out of underflow whenever the gzip
    // member is malformed. Retail's throw record names it
    // `.?AVTDataError@TGzInflateBuf@@` over `.?AVruntime_error@std@@` over
    // `.?AVexception@@`, which is what makes the 0x63e704 vftable a
    // three-slot copy of Dinkumware's {deleting dtor, what, _Doraise}.
    class TDataError;

    std::streambuf* source;      // +0x38
    // zlib 1.1.3's z_stream, 56 B: next_in/avail_in at +0x3c/+0x40 are what
    // the get-byte helper refills, next_out/avail_out at +0x48/+0x4c are the
    // window underflow drains, and the destructor calls inflateEnd on it.
    // The vendored zlib-1.1.3 IS retail's library (it matches 100%), so its
    // own header is the record - cc_wrap puts that directory on INCLUDE.
    z_stream stream;             // +0x3c
    unsigned char* buffer;       // +0x74, new[0x400]
    unsigned char* out_buffer;   // +0x78, buffer + 0x200
    unsigned long crc;           // +0x7c
    unsigned char ok;            // +0x80
    unsigned char source_eof;    // +0x81
    unsigned char inflating;     // +0x82
    char pad_83;

private:
    // Two private readers the bodies need. 0x4d5fd0 refills next_in from the
    // source buffer and returns the next byte or -1; 0x4d6ba0 is the same
    // read with the malformed-member throw attached, which retail keeps out
    // of line at four of underflow's eight trailer reads.
    int get_byte();              // 0x4d5fd0
    int read_byte();             // 0x4d6ba0
};
SIZE(TGzInflateBuf, 0x84);

// std::runtime_error's string constructor is an in-class inline, so the
// message form expands at its three throw sites while the message-less
// form stays out of line at 0x4d65e0.
class TGzInflateBuf::TDataError : public std::runtime_error {
public:
    TDataError();  // 0x4d65e0
    TDataError(const std::string& text) : std::runtime_error(text) {}
};

// The TAbstractFile view of a streambuf: Read is sgetn, Write is sputn.
// Size 8 is byte-proven by every stack instance (vftable, streambuf*).
class TStreamBufFile : public TAbstractFile {
public:
    TStreamBufFile(std::streambuf* newBuffer) : buffer(newBuffer) {}
    virtual int Read(void* data, int size);         // 0x483f10
    virtual int Write(const void* data, int size);  // 0x483f30

    std::streambuf* buffer;  // +4
};

#endif  /* HOMM3_GZINFLATEBUF_H */
