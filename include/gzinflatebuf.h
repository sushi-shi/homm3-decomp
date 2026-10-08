// gzinflatebuf.h - the gzip-inflating stream buffer shared by Complete's
// campaign loaders. Its retail band lies between gametypewindow and hero;
// the class has no Dreamcast CodeView counterpart. The campaign-only
// TAbstractFile adapter is defined with its callers in customcampaign.cpp.
#ifndef HOMM3_GZINFLATEBUF_H
#define HOMM3_GZINFLATEBUF_H

#include "va.h"

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

    // Original member, parameter and helper spellings: Loki h3maped's
    // assert text (`_m_pSrcBuf != __null`, `pSrcBuf != __null`,
    // `_m_zstream.next_in > reinterpret_cast< Bytef * >( _m_pInBuf )`,
    // `_m_zstream.next_out == reinterpret_cast< Bytef * >( _m_pOutBuf )`,
    // `static_cast< char >( *( _m_zstream.next_in - 1 ) ) == c`) and its
    // symbols `_getC__13TGzInflateBuf`, `_putBackC__13TGzInflateBufc` and
    // `_mustGetC__13TGzInflateBuf`.
    std::streambuf* _m_pSrcBuf;    // +0x38
    // zlib 1.1.3's z_stream, 56 B: next_in/avail_in at +0x3c/+0x40 are what
    // the get-byte helper refills, next_out/avail_out at +0x48/+0x4c are the
    // window underflow drains, and the destructor calls inflateEnd on it.
    // The vendored zlib-1.1.3 IS retail's library (it matches 100%), so its
    // own header is the record - cc_wrap puts that directory on INCLUDE.
    z_stream _m_zstream;           // +0x3c
    unsigned char* _m_pInBuf;      // +0x74, new[0x400]
    unsigned char* _m_pOutBuf;     // +0x78, buffer + 0x200
    unsigned long m_crc;           // +0x7c
    unsigned char m_ok;            // +0x80
    unsigned char m_sourceEof;    // +0x81
    unsigned char m_inflating;     // +0x82
    char m_open;

private:
    int _getC();                // 0x4d5fd0
    void _putBackC(signed char c); // Mac 0x220ac8
    int _mustGetC();            // 0x4d6ba0
    unsigned long _mustGetLong();
};
SIZE(TGzInflateBuf, 0x84);

#endif  /* HOMM3_GZINFLATEBUF_H */
