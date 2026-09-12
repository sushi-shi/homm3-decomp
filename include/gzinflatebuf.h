// gzinflatebuf.h - the gzip-inflating stream buffer shared by Complete's
// campaign loaders. Its retail band lies between gametypewindow and hero;
// the class has no Dreamcast CodeView counterpart. The campaign-only
// TAbstractFile adapter is defined with its callers in customcampaign.cpp.
#ifndef HOMM3_GZINFLATEBUF_H
#define HOMM3_GZINFLATEBUF_H

#include <streambuf>
#include <zlib.h>


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

    // Before normalization: source.
    std::streambuf* m_source;      // +0x38
    // zlib 1.1.3's z_stream, 56 B: next_in/avail_in at +0x3c/+0x40 are what
    // the get-byte helper refills, next_out/avail_out at +0x48/+0x4c are the
    // window underflow drains, and the destructor calls inflateEnd on it.
    // The vendored zlib-1.1.3 IS retail's library (it matches 100%), so its
    // own header is the record - cc_wrap puts that directory on INCLUDE.
    // Before normalization: stream.
    z_stream m_stream;             // +0x3c
    // Before normalization: buffer.
    unsigned char* m_buffer;       // +0x74, new[0x400]
    // Before normalization: out_buffer.
    unsigned char* m_outBuffer;   // +0x78, buffer + 0x200
    // Before normalization: crc.
    unsigned long m_crc;           // +0x7c
    // Before normalization: ok.
    unsigned char m_ok;            // +0x80
    // Before normalization: source_eof.
    unsigned char m_sourceEof;    // +0x81
    // Before normalization: inflating.
    unsigned char m_inflating;     // +0x82
    // Before normalization: pad_83; reference member TGzInflateBuf::m_open.
    char m_open;

private:
    // Two private readers the bodies need. 0x4d5fd0 refills next_in from the
    // source buffer and returns the next byte or -1; 0x4d6ba0 is the same
    // read with the malformed-member throw attached, which retail keeps out
    // of line at four of underflow's eight trailer reads.
    // Before normalization (function): TGzInflateBuf::get_byte.
    int getByte();              // 0x4d5fd0
    // Before normalization (function): TGzInflateBuf::read_byte.
    int readByte();             // 0x4d6ba0
};
SIZE(TGzInflateBuf, 0x84);

#endif  /* HOMM3_GZINFLATEBUF_H */
