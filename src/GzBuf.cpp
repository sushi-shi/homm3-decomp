// GzBuf.cpp - Loki h3maped object 68: the gzip deflating and inflating
// stream buffers over libio's streambuf. The #line directives place each
// assert and TAllocationFailure(__FILE__, __LINE__) on the line its retail
// immediate records; the original's comments and spacing are not known.
#include "GzBuf.h"

#include <assert.h>
#include <algorithm>

#include "autoarrayptr.h"
#include "exceptions.h"

static const int gz_magic[2] = { 0x1f, 0x8b };

int not_eof(int c)
{
    if (c == EOF)
        return 0;
    return c;
}

TGzDeflateBuf::TGzDeflateBuf(streambuf* pDestBuf, int level, int strategy)
    : _m_pDestBuf(pDestBuf),
      _m_pInBuf(NULL),
      _m_pOutBuf(NULL),
      _m_crc(crc32(0L, Z_NULL, 0))
{
#line 65
    assert(pDestBuf != NULL);

    _m_pInBuf = new char[2 * GZBUF_SIZE];
#line 69
    if (_m_pInBuf == NULL)
        throw TAllocationFailure(__FILE__, __LINE__);
    TAutoArrayPtr<char> pBuf(_m_pInBuf);
    _m_pOutBuf = _m_pInBuf + GZBUF_SIZE;

    setg(NULL, NULL, NULL);
    setp(_m_pInBuf, _m_pInBuf + GZBUF_SIZE);

    _m_zstream.next_in = reinterpret_cast< Bytef * >( _m_pInBuf );
    _m_zstream.avail_in = 0;
    _m_zstream.next_out = reinterpret_cast< Bytef * >( _m_pOutBuf );
    _m_zstream.avail_out = GZBUF_SIZE;
    _m_zstream.zalloc = NULL;
    _m_zstream.zfree = NULL;

    int result = deflateInit2(&_m_zstream, level, Z_DEFLATED, -MAX_WBITS, 8, strategy);
#line 87
    if (result == Z_MEM_ERROR)
        throw TAllocationFailure(__FILE__, __LINE__);
    assert(result == Z_OK);

    char header[10];
    fill_n(header, sizeof(header), '\0');
    header[0] = gz_magic[0];
    header[1] = gz_magic[1];
    header[2] = Z_DEFLATED;
    header[9] = 0x03;   // OS_CODE: Unix
    _m_pDestBuf->sputn(header, sizeof(header));

    pBuf.release();
}

TGzDeflateBuf::~TGzDeflateBuf()
{
#line 106
    assert(pptr() >= _m_pInBuf && pptr() <= _m_pInBuf + GZBUF_SIZE);

    _m_zstream.next_in = reinterpret_cast< Bytef * >( _m_pInBuf );
    _m_zstream.avail_in = pptr() - _m_pInBuf;
    _m_crc = crc32(_m_crc, _m_zstream.next_in, _m_zstream.avail_in);

    int result;
    while ((result = deflate(&_m_zstream, Z_FINISH)) == Z_OK || result == Z_BUF_ERROR) {
#line 117
        assert(result == Z_OK || ( result == Z_BUF_ERROR && _m_zstream.avail_out == 0 ));
        _m_pDestBuf->sputn(_m_pOutBuf, GZBUF_SIZE - _m_zstream.avail_out);
        _m_zstream.next_out = reinterpret_cast< Bytef * >( _m_pOutBuf );
        _m_zstream.avail_out = GZBUF_SIZE;
    }
    assert(result == Z_STREAM_END);
    _m_pDestBuf->sputn(_m_pOutBuf, GZBUF_SIZE - _m_zstream.avail_out);

    _putLong(_m_crc);
    _putLong(_m_zstream.total_in);

    result = deflateEnd(&_m_zstream);
#line 131
    assert(result == Z_OK);

    delete[] _m_pInBuf;
}

void TGzDeflateBuf::_putLong(uLong x)
{
    _m_pDestBuf->sputc(static_cast< char >( x ));
    _m_pDestBuf->sputc(static_cast< char >( x >> 8 ));
    _m_pDestBuf->sputc(static_cast< char >( x >> 16 ));
    _m_pDestBuf->sputc(static_cast< char >( x >> 24 ));
}

int TGzDeflateBuf::sync()
{
#line 148
    assert(pptr() >= _m_pInBuf && pptr() <= _m_pInBuf + GZBUF_SIZE);

    _m_zstream.next_in = reinterpret_cast< Bytef * >( _m_pInBuf );
    _m_zstream.avail_in = pptr() - _m_pInBuf;
    _m_crc = crc32(_m_crc, _m_zstream.next_in, _m_zstream.avail_in);

    int result;
    for (;;) {
        result = deflate(&_m_zstream, Z_SYNC_FLUSH);
        if (_m_zstream.avail_in == 0)
            break;
        assert(result == Z_OK || ( result == Z_BUF_ERROR && _m_zstream.avail_out == 0 ));
        if (_m_pDestBuf->sputn(_m_pOutBuf, GZBUF_SIZE - _m_zstream.avail_out) < GZBUF_SIZE - _m_zstream.avail_out)
            return EOF;
        _m_zstream.next_out = reinterpret_cast< Bytef * >( _m_pOutBuf );
        _m_zstream.avail_out = GZBUF_SIZE;
    }
    assert(result == Z_OK || ( result == Z_BUF_ERROR && _m_zstream.avail_out == 0 ));
    if (_m_pDestBuf->sputn(_m_pOutBuf, GZBUF_SIZE - _m_zstream.avail_out) < GZBUF_SIZE - _m_zstream.avail_out)
        return EOF;
    _m_zstream.next_out = reinterpret_cast< Bytef * >( _m_pOutBuf );
    _m_zstream.avail_out = GZBUF_SIZE;

    if (_m_pDestBuf->sync() == EOF)
        return EOF;
    setp(_m_pInBuf, _m_pInBuf + GZBUF_SIZE);
    return 0;
}

int TGzDeflateBuf::overflow(int c)
{
#line 186
    assert(pptr() >= _m_pInBuf && pptr() <= _m_pInBuf + GZBUF_SIZE);

    if (c != EOF) {
        _m_zstream.next_in = reinterpret_cast< Bytef * >( _m_pInBuf );
        _m_zstream.avail_in = pptr() - _m_pInBuf;
        _m_crc = crc32(_m_crc, _m_zstream.next_in, _m_zstream.avail_in);

        int result;
        for (;;) {
            result = deflate(&_m_zstream, Z_NO_FLUSH);
            if (_m_zstream.avail_in == 0)
                break;
#line 199
            assert(result == Z_OK || ( result == Z_BUF_ERROR && _m_zstream.avail_out == 0 ));
            if (_m_pDestBuf->sputn(_m_pOutBuf, GZBUF_SIZE - _m_zstream.avail_out) < GZBUF_SIZE - _m_zstream.avail_out)
                return EOF;
            _m_zstream.next_out = reinterpret_cast< Bytef * >( _m_pOutBuf );
            _m_zstream.avail_out = GZBUF_SIZE;
        }
        assert(result == Z_OK || ( result == Z_BUF_ERROR && _m_zstream.avail_out == 0 ));

        setp(_m_pInBuf, _m_pInBuf + GZBUF_SIZE);
        *_m_pInBuf = c;
        pbump(1);
    }
    return not_eof(c);
}

int TGzInflateBuf::_getC()
{
    if (_m_zstream.avail_in == 0) {
        if (_m_bSrcEOF)
            return EOF;
        int count = _m_pSrcBuf->sgetn(_m_pInBuf, GZBUF_SIZE);
        if (count < GZBUF_SIZE)
            _m_bSrcEOF = true;
        _m_zstream.next_in = reinterpret_cast< Bytef * >( _m_pInBuf );
        _m_zstream.avail_in = count;
        if (_m_zstream.avail_in == 0)
            return EOF;
    }
    Bytef c = *_m_zstream.next_in++;
    int result = c;
    --_m_zstream.avail_in;
    return result;
}

void TGzInflateBuf::_putBackC(char c)
{
#line 251
    assert(_m_zstream.next_in > reinterpret_cast< Bytef * >( _m_pInBuf ));
    assert(static_cast< char >( *( _m_zstream.next_in - 1 ) ) == c);
    --_m_zstream.next_in;
    ++_m_zstream.avail_in;
}

TGzInflateBuf::TGzInflateBuf(streambuf* pSrcBuf)
    : _m_pSrcBuf(pSrcBuf),
      _m_pInBuf(NULL),
      _m_pOutBuf(NULL),
      _m_crc(crc32(0L, Z_NULL, 0)),
      _m_bGzip(true),
      _m_bSrcEOF(false),
      _m_bInflating(false)
{
#line 286
    assert(pSrcBuf != NULL);

    _m_pInBuf = new char[2 * GZBUF_SIZE];
#line 290
    if (_m_pInBuf == NULL)
        throw TAllocationFailure(__FILE__, __LINE__);
    TAutoArrayPtr<char> pBuf(_m_pInBuf);
    _m_pOutBuf = _m_pInBuf + GZBUF_SIZE;

    setg(_m_pOutBuf, _m_pOutBuf + GZBUF_SIZE, _m_pOutBuf + GZBUF_SIZE);
    setp(NULL, NULL);

    _m_zstream.next_in = reinterpret_cast< Bytef * >( _m_pInBuf );
    _m_zstream.avail_in = 0;
    _m_zstream.next_out = reinterpret_cast< Bytef * >( _m_pOutBuf );
    _m_zstream.avail_out = GZBUF_SIZE;
    _m_zstream.zalloc = NULL;
    _m_zstream.zfree = NULL;

    // Not a gzip member: put the bytes back and pass the source through.
    try {
        int c1 = _getC();
        if (c1 == EOF)
            throw false;
        try {
            if (c1 != gz_magic[0])
                throw false;
            int c2 = _getC();
            if (c2 == EOF)
                throw false;
            if (c2 != gz_magic[1]) {
                _putBackC(c2);
                throw false;
            }
        }
        catch (bool) {
            _putBackC(c1);
            throw;
        }
    }
    catch (bool) {
        _m_bGzip = false;
    }

    if (_m_bGzip) {
#line 343
        assert(_m_pSrcBuf != NULL);

        if (_mustGetC() != Z_DEFLATED)
            throw TDataError();
        int flags = _mustGetC();
        if (flags & 0xe0)
            throw TDataError();
        for (int i = 6; i > 0; --i)
            _mustGetC();
        if (flags & 0x04) {
            uInt len = _mustGetC();
            len += _mustGetC() << 8;
            while (len-- > 0)
                _mustGetC();
        }
        if (flags & 0x08) {
            while (_mustGetC() != 0) {
            }
        }
        if (flags & 0x10) {
            while (_mustGetC() != 0) {
            }
        }
        if (flags & 0x02) {
            _mustGetC();
            _mustGetC();
        }

        int result = inflateInit2(&_m_zstream, -MAX_WBITS);
#line 383
        if (result == Z_MEM_ERROR)
            throw TAllocationFailure(__FILE__, __LINE__);
        assert(result == Z_OK);
        _m_bInflating = true;
    }

    pBuf.release();
}

TGzInflateBuf::~TGzInflateBuf()
{
    if (_m_bInflating) {
        int result = inflateEnd(&_m_zstream);
#line 399
        assert(result == Z_OK);
    }
    delete[] _m_pInBuf;
}

int_type TGzInflateBuf::underflow()
{
#line 408
    assert(gptr() == egptr());
    assert(_m_zstream.next_out == reinterpret_cast< Bytef * >( _m_pOutBuf ));
    assert(_m_zstream.avail_out == GZBUF_SIZE);

    while (_m_zstream.avail_out != 0 && !( _m_zstream.avail_in == 0 && _m_bSrcEOF )) {
        if (_m_zstream.avail_in == 0) {
            int count = _m_pSrcBuf->sgetn(_m_pInBuf, GZBUF_SIZE);
            if (count < GZBUF_SIZE)
                _m_bSrcEOF = true;
            _m_zstream.next_in = reinterpret_cast< Bytef * >( _m_pInBuf );
            _m_zstream.avail_in = count;
        }
        if (_m_zstream.avail_in != 0) {
            if (_m_bGzip) {
                int result = inflate(&_m_zstream, Z_SYNC_FLUSH);
                switch (result) {
#line 437
                case Z_MEM_ERROR:
                    throw TAllocationFailure(__FILE__, __LINE__);
                case Z_DATA_ERROR:
                    throw TDataError();
                }
                _m_crc = crc32(_m_crc, _m_zstream.next_out + _m_zstream.avail_out - GZBUF_SIZE,
                               GZBUF_SIZE - _m_zstream.avail_out);
                if (result == Z_STREAM_END) {
                    result = inflateEnd(&_m_zstream);
#line 449
                    assert(result == Z_OK);
                    _m_bInflating = false;
                    _m_bGzip = false;
                    break;
                }
                else {
#line 458
                    assert(result == Z_OK);
                }
            }
            else {
                uInt count = _m_zstream.avail_in < _m_zstream.avail_out
                    ? _m_zstream.avail_in : _m_zstream.avail_out;
                memcpy(_m_zstream.next_out, _m_zstream.next_in, count);
                _m_zstream.next_in += count;
                _m_zstream.avail_in -= count;
                _m_zstream.next_out += count;
                _m_zstream.avail_out -= count;
            }
        }
        else {
#line 471
            assert(_m_pSrcBuf == NULL);
        }
    }

    setg(_m_pOutBuf, _m_pOutBuf, _m_pOutBuf + GZBUF_SIZE - _m_zstream.avail_out);
    _m_zstream.next_out = reinterpret_cast< Bytef * >( _m_pOutBuf );
    _m_zstream.avail_out = GZBUF_SIZE;

    return egptr() > eback() ? static_cast< unsigned char >( *gptr() ) : EOF;
}

int TGzInflateBuf::_mustGetC()
{
    int c = _getC();
    if (c == EOF)
        throw TDataError();
    return c;
}

uLong TGzInflateBuf::_mustGetLong()
{
    uLong x = _mustGetC();
    x += _mustGetC() << 8;
    x += _mustGetC() << 16;
    x += _mustGetC() << 24;
    return x;
}
