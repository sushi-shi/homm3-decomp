// MFCFileBuf.cpp - a stream buffer over an MFC CFile (h3maped
// 0x486b96..0x486f98; h3ccmped 0x428ae0..0x429081). CFile failures are
// caught as CFileException* (each handler's catch type) and reported as EOF.
#include "editor/stdafx.h"

#include "exceptions.h"
#include "va.h"
#include "editor/MFCFileBuf.h"

VA(0x00486b96, 0xbb)
TMFCFileBuf::TMFCFileBuf(CFile* pFile) : _m_pFile(pFile), _m_pBuf(NULL)
{
    _m_pBuf = new char[s_kBufSize];
    if (_m_pBuf == NULL)
        throw TAllocationFailure();
    setg(_m_pBuf, _m_pBuf + s_kBufSize, _m_pBuf + s_kBufSize);
    setp(_m_pBuf, _m_pBuf + s_kBufSize);
}

VA_COMPGEN(0x00486c51, 0x1c, SCALAR_DELETING_DTOR, TMFCFileBuf)

VA(0x00486c6d, 0x62)
TMFCFileBuf::~TMFCFileBuf()
{
    sync();
    if (gptr() < egptr())
        _m_pFile->Seek(gptr() - egptr(), CFile::current);
    delete[] _m_pBuf;
}

VA(0x00486ccf, 0x112)
std::streampos TMFCFileBuf::seekoff(std::streamoff off, std::ios_base::seekdir way, std::ios_base::openmode)
{
    sync();
    if (gptr() < egptr()) {
        if (way == std::ios_base::cur)
            off -= egptr() - gptr();
        setg(_m_pBuf, _m_pBuf + s_kBufSize, _m_pBuf + s_kBufSize);
    }
    UINT from;
    switch (way) {
    case std::ios_base::beg:
        from = CFile::begin;
        break;
    case std::ios_base::cur:
        from = CFile::current;
        break;
    case std::ios_base::end:
        from = CFile::end;
        break;
    }
    LONG pos;
    try {
        pos = _m_pFile->Seek(off, from);
    } catch (CFileException* e) {
        e->Delete();
        return std::streampos(-1);
    }
    return pos;
}

VA(0x00486de1, 0x2e)
std::streampos TMFCFileBuf::seekpos(std::streampos pos, std::ios_base::openmode which)
{
    return seekoff(pos, std::ios_base::beg, which);
}

VA(0x00486e0f, 0x7d)
int TMFCFileBuf::sync()
{
    if (pptr() > pbase()) {
        try {
            _m_pFile->Write(pbase(), pptr() - pbase());
            _m_pFile->Flush();
        } catch (CFileException* e) {
            e->Delete();
            return EOF;
        }
        setp(_m_pBuf, _m_pBuf + s_kBufSize);
    }
    return 0;
}

VA(0x00486e8c, 0x79)
int TMFCFileBuf::underflow()
{
    try {
        setg(_m_pBuf, _m_pBuf, _m_pBuf + _m_pFile->Read(eback(), s_kBufSize));
    } catch (CFileException* e) {
        e->Delete();
        return EOF;
    }
    if (egptr() > eback())
        return (unsigned char)*gptr();
    return EOF;
}

VA(0x00486f05, 0x93)
int TMFCFileBuf::overflow(int c)
{
    if (c != EOF) {
        if (pptr() > pbase()) {
            try {
                _m_pFile->Write(pbase(), pptr() - pbase());
            } catch (CFileException* e) {
                e->Delete();
                return EOF;
            }
            setp(_m_pBuf, _m_pBuf + s_kBufSize);
        }
        *pptr() = c;
        pbump(1);
        return c;
    }
    return 0;
}
