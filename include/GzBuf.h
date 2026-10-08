// GzBuf.h - gzip stream buffers over another streambuf: TGzInflateBuf reads
// a gzip member, TGzDeflateBuf writes one (the editor saves maps with it).
// Loki h3maped object 68 (`GzBuf.cpp`, 29 assert strings) is the record.
// The Loki port derives both from libio's `streambuf` (an _IO_FILE with the
// vptr at +0x94), so members start at +0x98. Member names come from the
// assert text (`_m_pInBuf`, `_m_pOutBuf`, `_m_zstream`, `_m_pSrcBuf`) and
// the constructor parameters (`pSrcBuf`, `pDestBuf`); the flag names are
// not recorded. The header's file name is inferred from GzBuf.cpp.
#ifndef HOMM3_GZBUF_H
#define HOMM3_GZBUF_H

#include <stdexcept>
#include <string>
#include <streambuf.h>
#include <zlib.h>

// Both halves of the work buffer; every assert spells the expanded 8192.
#define GZBUF_SIZE 8192

// libio's streambuf has no traits; the port keeps the Dinkumware spelling
// of underflow's result. __PRETTY_FUNCTION__ reads "int_type
// TGzInflateBuf::underflow()", which g++ 2.95 prints only for a
// namespace-scope typedef (a class member typedef prints as "int").
typedef int int_type;

class TGzInflateBuf : public streambuf {
public:
    // Thrown for a truncated or malformed gzip member.
    class TDataError : public runtime_error {
    public:
        TDataError() : runtime_error( string() ) {}
    };

    TGzInflateBuf(streambuf* pSrcBuf);
    virtual ~TGzInflateBuf();

protected:
    virtual int_type underflow();

private:
    int _getC();
    void _putBackC(char c);
    int _mustGetC();
    uLong _mustGetLong();

    streambuf* _m_pSrcBuf;      // +0x98
    z_stream _m_zstream;        // +0x9c
    char* _m_pInBuf;            // +0xd4, new char[2 * GZBUF_SIZE]
    char* _m_pOutBuf;           // +0xd8, _m_pInBuf + GZBUF_SIZE
    uLong _m_crc;               // +0xdc
    bool _m_bGzip;              // +0xe0, false: raw pass-through
    bool _m_bSrcEOF;            // +0xe1
    bool _m_bInflating;         // +0xe2, inflateInit2 succeeded
};

class TGzDeflateBuf : public streambuf {
public:
    TGzDeflateBuf(streambuf* pDestBuf, int level = Z_DEFAULT_COMPRESSION,
                  int strategy = Z_DEFAULT_STRATEGY);
    virtual ~TGzDeflateBuf();

protected:
    virtual int sync();
    virtual int overflow(int c);

private:
    void _putLong(uLong x);

    streambuf* _m_pDestBuf;     // +0x98
    z_stream _m_zstream;        // +0x9c
    char* _m_pInBuf;            // +0xd4, the put area
    char* _m_pOutBuf;           // +0xd8, deflate's output window
    uLong _m_crc;               // +0xdc
};

#endif  /* HOMM3_GZBUF_H */
