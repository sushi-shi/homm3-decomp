// RawStream.h - the editors' unformatted streams over a streambuf. Loki's
// h3maped exports TRawIStream(streambuf*) and TRawOStream(streambuf*)
// (19 bytes each, one member), the member templates operator>> <T> and
// operator<< <T> for the scalars and char arrays of the map format, and the
// nested failures TRawIStream::TReadFailure and TRawOStream::TWriteFailure
// (RTTI .?AVTReadFailure@TRawIStream@@, .?AVTWriteFailure@TRawOStream@@ in
// both Windows editors).
#ifndef HOMM3_EDITOR_RAWSTREAM_H
#define HOMM3_EDITOR_RAWSTREAM_H

#include <streambuf>
#include <string>

#include "exceptions.h"

class TRawIStream {
public:
    // h3ccmped 0x4011b0: the default runtime error, then the count read.
    class TReadFailure : public TRuntimeError {
    public:
        explicit TReadFailure(int nRead) : m_nRead(nRead) {}
        int m_nRead;
    };

    explicit TRawIStream(std::streambuf* pStreamBuf) : m_pStreamBuf(pStreamBuf) {}

    template <class T>
    TRawIStream& operator>>(T& value)
    {
        int nRead = m_pStreamBuf->sgetn(reinterpret_cast<char*>(&value), sizeof(T));
        if (nRead < sizeof(T))
            throw TReadFailure(nRead);
        return *this;
    }

private:
    std::streambuf* m_pStreamBuf;
};

// A string: its length, then its characters (h3maped 0x4190cb, cdecl).
TRawIStream& operator>>(TRawIStream& stream, std::string& value);

class TRawOStream {
public:
    // h3ccmped 0x401280: the default runtime error, then the count written.
    class TWriteFailure : public TRuntimeError {
    public:
        explicit TWriteFailure(int nWritten) : m_nWritten(nWritten) {}
        int m_nWritten;
    };

    explicit TRawOStream(std::streambuf* pStreamBuf) : m_pStreamBuf(pStreamBuf) {}

    template <class T>
    TRawOStream& operator<<(const T& value)
    {
        int nWritten = m_pStreamBuf->sputn(reinterpret_cast<const char*>(&value), sizeof(T));
        if (nWritten < sizeof(T))
            throw TWriteFailure(nWritten);
        return *this;
    }

private:
    std::streambuf* m_pStreamBuf;
};

#endif  /* HOMM3_EDITOR_RAWSTREAM_H */
