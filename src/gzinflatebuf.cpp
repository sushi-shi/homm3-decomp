// gzinflatebuf.cpp - the gzip streambufs: the deflating one Loki's map
// editor saves with, and the inflating one Complete reads .h3c campaign
// payloads and compressed maps through. The editor's GzBuf.cpp is this
// file; the game links the deflating half unreferenced, so /OPT:REF drops
// its code while its zlib references still pull deflate.obj and crc32.obj
// ahead of inflate.obj, as retail's zlib order shows.

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

#include <algorithm>
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
DATA(0x0063e6fc) static const int g_gzMagic[2] = {0x1f, 0x8b};

// Mac uses two 4096-byte windows; Windows uses two 512-byte windows.
// Refill, output bounds, allocation and CRC spans all use the same capacity.
#if defined(HOMM3_TARGET_MAC)
#define GZ_WINDOW_SIZE 4096
#else
#define GZ_WINDOW_SIZE 512
#endif

// The deflating half views its char windows as zlib's bytes through one
// helper, with Loki GzBuf.cpp's `reinterpret_cast< Bytef * >` spelling.
static Bytef* zlibBytes(char* window)
{
    return reinterpret_cast<Bytef*>(window);
}

// Loki GzBuf.cpp defines the deflating buffer first. Its asserts are
// compiled out of the release game, like every assert in this build.
TGzDeflateBuf::TGzDeflateBuf(std::streambuf* pDestBuf, int level, int strategy)
    : _m_pDestBuf(pDestBuf),
      _m_pInBuf(0),
      _m_pOutBuf(0),
      m_crc(crc32(0, 0, 0))
{
    _m_pInBuf = new char[2 * GZ_WINDOW_SIZE];
    if (_m_pInBuf == 0)
        throw TAllocationFailure();
    TAutoArrayPtr<char> ownedBuffer(_m_pInBuf);
    _m_pOutBuf = _m_pInBuf + GZ_WINDOW_SIZE;

    setg(0, 0, 0);
    setp(_m_pInBuf, _m_pInBuf + GZ_WINDOW_SIZE);

    _m_zstream.next_in = zlibBytes(_m_pInBuf);
    _m_zstream.avail_in = 0;
    _m_zstream.next_out = zlibBytes(_m_pOutBuf);
    _m_zstream.avail_out = GZ_WINDOW_SIZE;
    _m_zstream.zalloc = 0;
    _m_zstream.zfree = 0;

    int result = deflateInit2(&_m_zstream, level, Z_DEFLATED, -MAX_WBITS, 8, strategy);
    if (result == Z_MEM_ERROR)
        throw TAllocationFailure();

    char header[10];
    std::fill_n(header, sizeof(header), '\0');
    header[0] = static_cast<char>(g_gzMagic[0]);
    header[1] = static_cast<char>(g_gzMagic[1]);
    header[2] = Z_DEFLATED;
    header[9] = 0x0b;   // OS_CODE: Win32
    _m_pDestBuf->sputn(header, sizeof(header));

    ownedBuffer.release();
}

TGzDeflateBuf::~TGzDeflateBuf()
{
    _m_zstream.next_in = zlibBytes(_m_pInBuf);
    _m_zstream.avail_in = pptr() - _m_pInBuf;
    m_crc = crc32(m_crc, _m_zstream.next_in, _m_zstream.avail_in);

    int result;
    while ((result = deflate(&_m_zstream, Z_FINISH)) == Z_OK || result == Z_BUF_ERROR) {
        _m_pDestBuf->sputn(_m_pOutBuf, GZ_WINDOW_SIZE - _m_zstream.avail_out);
        _m_zstream.next_out = zlibBytes(_m_pOutBuf);
        _m_zstream.avail_out = GZ_WINDOW_SIZE;
    }
    _m_pDestBuf->sputn(_m_pOutBuf, GZ_WINDOW_SIZE - _m_zstream.avail_out);

    _putLong(m_crc);
    _putLong(_m_zstream.total_in);

    deflateEnd(&_m_zstream);

    delete[] _m_pInBuf;
}

void TGzDeflateBuf::_putLong(unsigned long x)
{
    _m_pDestBuf->sputc(static_cast<char>(x));
    _m_pDestBuf->sputc(static_cast<char>(x >> 8));
    _m_pDestBuf->sputc(static_cast<char>(x >> 16));
    _m_pDestBuf->sputc(static_cast<char>(x >> 24));
}

int TGzDeflateBuf::sync()
{
    _m_zstream.next_in = zlibBytes(_m_pInBuf);
    _m_zstream.avail_in = pptr() - _m_pInBuf;
    m_crc = crc32(m_crc, _m_zstream.next_in, _m_zstream.avail_in);

    for (;;) {
        deflate(&_m_zstream, Z_SYNC_FLUSH);
        if (_m_zstream.avail_in == 0)
            break;
        if (_m_pDestBuf->sputn(_m_pOutBuf, GZ_WINDOW_SIZE - _m_zstream.avail_out)
                < static_cast<int>(GZ_WINDOW_SIZE - _m_zstream.avail_out))
            return traits_type::eof();
        _m_zstream.next_out = zlibBytes(_m_pOutBuf);
        _m_zstream.avail_out = GZ_WINDOW_SIZE;
    }
    if (_m_pDestBuf->sputn(_m_pOutBuf, GZ_WINDOW_SIZE - _m_zstream.avail_out)
            < static_cast<int>(GZ_WINDOW_SIZE - _m_zstream.avail_out))
        return traits_type::eof();
    _m_zstream.next_out = zlibBytes(_m_pOutBuf);
    _m_zstream.avail_out = GZ_WINDOW_SIZE;

    if (_m_pDestBuf->pubsync() == traits_type::eof())
        return traits_type::eof();
    setp(_m_pInBuf, _m_pInBuf + GZ_WINDOW_SIZE);
    return 0;
}

int TGzDeflateBuf::overflow(int c)
{
    if (c != traits_type::eof()) {
        _m_zstream.next_in = zlibBytes(_m_pInBuf);
        _m_zstream.avail_in = pptr() - _m_pInBuf;
        m_crc = crc32(m_crc, _m_zstream.next_in, _m_zstream.avail_in);

        for (;;) {
            deflate(&_m_zstream, Z_NO_FLUSH);
            if (_m_zstream.avail_in == 0)
                break;
            if (_m_pDestBuf->sputn(_m_pOutBuf, GZ_WINDOW_SIZE - _m_zstream.avail_out)
                    < static_cast<int>(GZ_WINDOW_SIZE - _m_zstream.avail_out))
                return traits_type::eof();
            _m_zstream.next_out = zlibBytes(_m_pOutBuf);
            _m_zstream.avail_out = GZ_WINDOW_SIZE;
        }

        setp(_m_pInBuf, _m_pInBuf + GZ_WINDOW_SIZE);
        *_m_pInBuf = static_cast<char>(c);
        pbump(1);
    }
    return traits_type::not_eof(c);
}

// Refill from the source streambuf and return the next byte. Loki's
// `Bytef c = *next_in++; int result = c;` capture is retail's: it stores the
// advanced cursor before widening, and its inline cost is what leaves the
// constructor's last FHCRC read as the one TDataError expansion.
VA(0x004d5fd0, 0x74)
MAC_ADDRESS(0x220a18, 0xb0)
int TGzInflateBuf::_getC()
{
    if (_m_zstream.avail_in == 0) {
        if (m_sourceEof)
            return -1;
        int count = _m_pSrcBuf->sgetn(
            _m_pInBuf, GZ_WINDOW_SIZE);
        if (count < GZ_WINDOW_SIZE)
            m_sourceEof = true;
        _m_zstream.next_in = reinterpret_cast<Bytef*>(_m_pInBuf);
        _m_zstream.avail_in = count;
        // Mac 0x220a80 reloads the unsigned stream member for this guard.
        if (_m_zstream.avail_in == 0)
            return -1;
    }
    Bytef c = *_m_zstream.next_in++;
    int result = c;
    --_m_zstream.avail_in;
    return result;
}

// CodeWarrior retains this helper immediately after _getC and calls it
// twice from the constructor's gzip-magic fallback. Its byte argument is
// passed at both call sites but the helper only rewinds the input cursor.
MAC_ADDRESS(0x220ac8, 0x1c)
void TGzInflateBuf::_putBackC(char)
{
    --_m_zstream.next_in;
    ++_m_zstream.avail_in;
}

// 0x4d6050: build the window, then walk the gzip member header exactly as
// zlib's gzio.c check_header does. A failed magic pair is caught here and
// demotes the stream to raw pass-through (ok = 0) rather than propagating.

// The temporary buffer owner is the ordinary TAutoArrayPtr: Mac stores
// owns/pointer at stack+0x268/+0x26c, clears owns on release, and calls
// array delete at 0x2212f4. Windows unwind 0x62c913 -> 0x4b7040 uses
// the same flag/pointer pair; that body alone did not distinguish auto_ptr.

// Retail's reserved-byte loop counts down from six. Its extra-field loop
// tests the unsigned OLD count with jbe/ja, recovered by extra-- > 0.
// Loki h3maped's constructor (0x81d359b..0x81d35bf) accumulates the
// length in one local: extra = _mustGetC(); extra += _mustGetC() << 8.
// That spelling restores retail's call/expansion choice for the two FHCRC
// reads below (93.92% -> 98.92%); a separate low-byte local inverts it.
// The name/comment loops have peeled _getC sites at ctor+0x422/0x432
// and +0x47b/0x48b; while(_mustGetC()!=0) reproduces all twelve ordered
// _getC references and the exact B38..B50 countdown/name-loop blocks.
// The old while(1)/break model had ten references and incorrectly treated
// the missing guards as surplus calls. Mac retains one checked read per
// loop (0x2210dc/0x221154), then tests the decoded byte at 0x221140/0x2211b8;
// CodeWarrior does not peel these conditions. Both forms keep the canonical
// _mustGetC helper. The stream-traits byte snapshot below recovers 98.9360%:
// all 61 CFG blocks and the exception expansion sequence agree. VC6 gives a
// nested callee (caller budget - callee cb) / remaining sites; Loki's plain
// `if (flags & 0x04)` tests, unnamed method check and named inflateInit2
// result shrink this body so that only the second FHCRC read expands
// TDataError, as in retail.
VA(0x004d6050, 0x58A)
MAC_ADDRESS(0x220ae4, 0x82c)  // anchor-vtable ??_7TGzInflateBuf@@6B@ + anchor-import @inflateInit2_@16, retail-only
TGzInflateBuf::TGzInflateBuf(std::streambuf* pSrcBuf)
    : _m_pSrcBuf(pSrcBuf),
      _m_pInBuf(0),
      _m_pOutBuf(0),
      m_crc(crc32(0, 0, 0)),
      m_ok(true),
      m_sourceEof(false),
      m_inflating(false)
{
    _m_pInBuf = new char[2 * GZ_WINDOW_SIZE];
    if (_m_pInBuf == 0)
        throw TAllocationFailure();
    TAutoArrayPtr<char> ownedBuffer(_m_pInBuf);
    _m_pOutBuf = _m_pInBuf + GZ_WINDOW_SIZE;
    setg(_m_pOutBuf,
         _m_pOutBuf,
         _m_pOutBuf);
    setp(0, 0);
    _m_zstream.next_out = reinterpret_cast<Bytef*>(_m_pOutBuf);
    _m_zstream.next_in = reinterpret_cast<Bytef*>(_m_pInBuf);
    _m_zstream.avail_in = 0;
    _m_zstream.avail_out = GZ_WINDOW_SIZE;
    _m_zstream.zalloc = 0;
    _m_zstream.zfree = 0;
    try {
        int magic = _getC();
        if (magic == -1)
            throw false;
        try {
            if (magic != g_gzMagic[0])
                throw false;
            // Mac preserves the first byte in r20 for the catch below;
            // the second byte in r19 is passed to _putBackC at 0x220d0c.
            int nextMagic = _getC();
            if (nextMagic == -1)
                throw false;
            if (nextMagic != g_gzMagic[1]) {
                _putBackC(nextMagic);
                throw false;
            }
        } catch (bool) {
            _putBackC(magic);
            throw;
        }
    } catch (bool) {
        m_ok = false;
    }
    if (m_ok) {
        if (_mustGetC() != Z_DEFLATED)
            throw TDataError();
        int flags = _mustGetC();
        if (flags & 0xe0)
            throw TDataError();
        for (int skip = 6; skip > 0; --skip)
            _mustGetC();
        if (flags & 0x04) {
            unsigned extra = _mustGetC();
            extra += _mustGetC() << 8;
            while (extra-- > 0)
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
        if (result == Z_MEM_ERROR)
            throw TAllocationFailure();
        m_inflating = true;
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
VA(0x004d6820, 0xF6)
MAC_ADDRESS(0x221394, 0x12c)
TGzInflateBuf::~TGzInflateBuf()
{
    if (_m_zstream.avail_in > 0) {
        _m_pSrcBuf->pubseekoff(
            -static_cast<long>(_m_zstream.avail_in),
            std::ios_base::cur, std::ios_base::in);
    }
    if (m_ok) {
        if (m_inflating)
            inflateEnd(&_m_zstream);
    } else if (egptr() > gptr()) {
        _m_pSrcBuf->pubseekoff(
            gptr() - egptr(), std::ios_base::cur, std::ios_base::in);
    }
    delete[] _m_pInBuf;
}

// 0x4d6920: drain the source into the output half, either
// through inflate or, for a non-gzip member, by straight copy.
// Constructor visibility probes are Windows-flat. A temporary explicit
// inline _mustGetC suppresses its required Windows body and is rejected.
// CW auto/deferred/depth controls change the native retained-call pattern;
// no source qualifier or compiler override is inferred from those controls.
// Mac 0x221574..0x221588 dispatches inflate status with a range tree:
// compare -3, skip larger values, compare -4, then select the exceptions.
// A switch reproduces that tree at candidate +0xc0..+0xd4; sequential
// equality guards do not. Windows 81.4346% -> 81.90%, with every trailer
// _mustGetC call preserved. Exception homes and expansion decisions remain.

// The FIRST guard reads `avail_in <= 0`, not `== 0`: retail inverts it to
// `ja` (unsigned above) where `== 0` can only ever emit `jne`, and the two
// are the same test on zlib's `uInt`. 80.8063 -> 81.1188. The second guard
// really is `== 0` - retail emits `jne` there.

// Retail's raw-copy minimum uses a strict unsigned comparison: spelling
// avail_in < avail_out ? avail_in : avail_out restores jb and raises this
// caller from 81.1204% to 81.4346%. std::_cpp_min on copied local counts
// adds operand homes and scores 79.1832%; keep the direct value expression.

// The gzip trailer is two _mustGetLong reads, as Loki h3maped retains
// TGzInflateBuf::_mustGetLong (0x81d4070: four _mustGetC calls summed with
// shifts 0/8/16/24). Inlined twice, it gives retail's expanded/called
// _mustGetC pattern (expanded at trailer reads 1,2,5,6, called at 3,4,7,8)
// and its separate exception slots. Loki's refill also stores next_in
// before avail_in (0x81d3ba1..0x81d3bb0); with the helper, that order is
// retail's ecx load of the input buffer. 81.80% -> 100%. Mac expands all
// eight checked reads without a visible group boundary.
VA(0x004d6920, 0x251)
MAC_ADDRESS(0x2214c0, 0x4d4)  // anchor-vtable ??_7TGzInflateBuf@@6B@ slot 4 + anchor-import @inflate@8, retail-only
int TGzInflateBuf::underflow()
{
    while (_m_zstream.avail_out > 0) {
        if (_m_zstream.avail_in <= 0 && m_sourceEof)
            break;
        if (_m_zstream.avail_in == 0) {
            int count = _m_pSrcBuf->sgetn(
                _m_pInBuf, GZ_WINDOW_SIZE);
            if (count < GZ_WINDOW_SIZE)
                m_sourceEof = true;
            _m_zstream.next_in = reinterpret_cast<Bytef*>(_m_pInBuf);
            _m_zstream.avail_in = count;
        }
        if (_m_zstream.avail_in > 0) {
            if (m_ok) {
                if (m_inflating) {
                    int status = inflate(&_m_zstream, Z_SYNC_FLUSH);
                    switch (status) {
                    case Z_MEM_ERROR:
                        throw TAllocationFailure();
                    case Z_DATA_ERROR:
                        throw TDataError();
                    default:
                        break;
                    }
                    // This is the live z_stream output window's beginning.
                    // Retail +0x9b..+0xb3 loads next_out/avail_out and forms
                    // their sum minus 512, rather than loading _m_pOutBuf.
                    // Preserve that stream-state dependency, not a guessed
                    // cancellation through the separately cached pointer.
                    m_crc = crc32(m_crc,
                                _m_zstream.next_out + _m_zstream.avail_out - GZ_WINDOW_SIZE,
                                GZ_WINDOW_SIZE - _m_zstream.avail_out);
                    if (status == Z_STREAM_END) {
                        inflateEnd(&_m_zstream);
                        m_inflating = false;
                        _mustGetLong();
                        _mustGetLong();
                        break;
                    }
                }
            } else {
                unsigned count = _m_zstream.avail_in < _m_zstream.avail_out
                    ? _m_zstream.avail_in : _m_zstream.avail_out;
                memcpy(_m_zstream.next_out, _m_zstream.next_in, count);
                _m_zstream.next_in += count;
                _m_zstream.avail_in -= count;
                _m_zstream.next_out += count;
                _m_zstream.avail_out -= count;
            }
        }
    }
    setg(_m_pOutBuf,
         _m_pOutBuf,
         _m_pOutBuf
             + GZ_WINDOW_SIZE - _m_zstream.avail_out);
    _m_zstream.next_out = reinterpret_cast<Bytef*>(_m_pOutBuf);
    _m_zstream.avail_out = GZ_WINDOW_SIZE;
    // Mac 0x221970..0x221974 loads the get pointer then widens its byte
    // unsigned, the same canonical traits conversion used by _getC.
    if (egptr() > eback())
        return traits_type::to_int_type(*gptr());
    return -1;
}

// 0x4d6b80 calls TRuntimeError(const char*) at 0x49a0c0, then installs
// the derived vptr. Both its canonical body and VA are in exceptions.h;
// this object retains the out-of-line copy and calls it, while the
// objnames throw expands the shared derived initialization.

// 0x4d6ba0: get_byte with the malformed-member throw attached.
VA(0x004d6ba0, 0x81)
int TGzInflateBuf::_mustGetC()
{
    int c = _getC();
    if (c == -1)
        throw TDataError();
    return c;
}

// Loki h3maped 0x81d4070 retains this little-endian reader right after
// _mustGetC. Retail has no out-of-line copy: underflow expands both calls.
unsigned long TGzInflateBuf::_mustGetLong()
{
    unsigned long x = _mustGetC();
    x += _mustGetC() << 8;
    x += _mustGetC() << 16;
    x += _mustGetC() << 24;
    return x;
}

VA_COMPGEN(0x0041ba90, 0x162, CLASS_CTOR, runtime_error)

VA_COMPGEN(0x0041b7b0, 0x169, IMPLICIT_COPY_CTOR, TRuntimeError)

VA_COMPGEN(0x0041b920, 0x16F, IMPLICIT_COPY_CTOR, TAllocationFailure)
