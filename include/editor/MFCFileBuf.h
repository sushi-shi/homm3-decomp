// MFCFileBuf.h - a stream buffer over an MFC CFile (MFCFileBuf.cpp; both
// editors link it: h3ccmped 0x428ae0..0x429081). A 512-byte buffer serves
// both areas; sync() writes the put area out, underflow() refills the get
// area, and the seeks resynchronise the file with what the buffers hold.
// CFile failures are caught and reported as the stream's EOF. Layout from
// the constructor: the file at +0x38 (after Dinkumware's basic_streambuf),
// the buffer at +0x3c.
#ifndef HOMM3_EDITOR_MFCFILEBUF_H
#define HOMM3_EDITOR_MFCFILEBUF_H

#include <streambuf>

class CFile;

class TMFCFileBuf : public std::streambuf {
public:
    explicit TMFCFileBuf(CFile* pFile);
    virtual ~TMFCFileBuf();

protected:
    virtual int overflow(int c = EOF);
    virtual int underflow();
    virtual std::streampos seekoff(std::streamoff off, std::ios_base::seekdir way,
                                   std::ios_base::openmode which = std::ios_base::in | std::ios_base::out);
    virtual std::streampos seekpos(std::streampos pos,
                                   std::ios_base::openmode which = std::ios_base::in | std::ios_base::out);
    virtual int sync();

private:
    enum { s_kBufSize = 512 };

    CFile* _m_pFile;
    char* _m_pBuf;
};

#endif  /* HOMM3_EDITOR_MFCFILEBUF_H */
