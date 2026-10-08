// RawStream.h - binary map serialization over a streambuf (Loki h3maped).
// Every body is inline or a template (13 RawStream.h assert strings survive
// only as orphans in the objects that include it); the instantiations live
// in the objects that first need them. This header declares the interface
// the matched units call; the member templates are filled in with the
// objects that own their instantiations.
#ifndef HOMM3_EDITOR_RAWSTREAM_H
#define HOMM3_EDITOR_RAWSTREAM_H

#include <exception>
#include <string>
#include <streambuf.h>

class TRawIStream {
public:
    class TReadFailure : public exception {
    };

    TRawIStream(streambuf* pStreamBuf);

    template<class T> TRawIStream& operator>>(T& value);

private:
    streambuf* _m_pStreamBuf;
};

class TRawOStream {
public:
    class TWriteFailure : public exception {
    };

    TRawOStream(streambuf* pStreamBuf);

    template<class T> TRawOStream& operator<<(const T& value);

private:
    streambuf* _m_pStreamBuf;
};

TRawIStream& operator>>(TRawIStream& stream, string& value);
TRawOStream& operator<<(TRawOStream& stream, const string& value);

#endif  /* HOMM3_EDITOR_RAWSTREAM_H */
