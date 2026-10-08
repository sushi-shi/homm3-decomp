// RawStream.h - binary map serialization over a streambuf (Loki h3maped).
// Every body is inline or a template; the instantiations live in the
// objects that first need them (the exceptions and the TCreatureStack
// iterators in Army.cpp, the string reader in Event.cpp, the containers in
// GameMap.cpp). The exceptions are TRuntimeErrors built from this file's
// name and line (RawStream.h:45 and :126, "Input stream read failure.",
// "Output stream write failure."). Each element goes through sgetn/sputn
// with its own size; a short count throws. The helpers are inline: g++
// at -O0 keeps an inline function's parameters in registers.
#ifndef HOMM3_EDITOR_RAWSTREAM_H
#define HOMM3_EDITOR_RAWSTREAM_H

#include "editor/stdafx.h"

#include <bitset>
#include <string>
#include <string.h>
#include <streambuf.h>

#include "exceptions.h"

class TRawIStream {
public:
    class TReadFailure : public TRuntimeError {
    public:
#line 45 "RawStream.h"
        TReadFailure() : TRuntimeError(__FILE__, __LINE__, "Input stream read failure.") {}
    };

    TRawIStream(streambuf* pStreamBuf) : _m_pStreamBuf(pStreamBuf) {}

    template<class T>
    TRawIStream& operator>>(T& value)
    {
        int numRead = _m_pStreamBuf->sgetn(reinterpret_cast<char*>(&value), sizeof(T));
        if (numRead < sizeof(T))
            throw TReadFailure();
        return *this;
    }

private:
    streambuf* _m_pStreamBuf;
};

template<class OutputIterator>
inline TRawIStream& readToIter(TRawIStream& stream, OutputIterator first, OutputIterator last)
{
    while (first != last)
        stream >> *first++;
    return stream;
}

inline TRawIStream& operator>>(TRawIStream& stream, string& value)
{
    value.erase(value.begin(), value.end());
    long size;
    stream >> size;
    value.resize(size);
    return readToIter(stream, value.begin(), value.end());
}

class TRawOStream {
public:
    class TWriteFailure : public TRuntimeError {
    public:
#line 126 "RawStream.h"
        TWriteFailure() : TRuntimeError(__FILE__, __LINE__, "Output stream write failure.") {}
    };

    TRawOStream(streambuf* pStreamBuf) : _m_pStreamBuf(pStreamBuf) {}

    template<class T>
    TRawOStream& operator<<(const T& value)
    {
        int numWritten = _m_pStreamBuf->sputn(reinterpret_cast<const char*>(&value), sizeof(T));
        if (numWritten < sizeof(T))
            throw TWriteFailure();
        return *this;
    }

private:
    streambuf* _m_pStreamBuf;
};

template<class InputIterator>
inline TRawOStream& writeFromIter(TRawOStream& stream, InputIterator first, InputIterator last)
{
    while (first != last)
        stream << *first++;
    return stream;
}

template<class Container>
inline TRawOStream& writeContainer(TRawOStream& stream, const Container& container)
{
    stream << static_cast<long>(container.size());
    return writeFromIter(stream, container.begin(), container.end());
}

inline TRawOStream& operator<<(TRawOStream& stream, const string& value)
{
    return writeContainer(stream, value);
}

// A bitset<N> travels as (N + 7) / 8 bytes, bit 0 first (ObjectType.cpp
// owns the 48- and 9-bit instantiations).
template<size_t N>
void writeBitset(TRawOStream& stream, const bitset<N>& bits)
{
    unsigned char bytes[(N + 7) / 8];
    memset(bytes, 0, sizeof(bytes));
    for (size_t i = 0; i < N; i++)
        if (bits[i])
            bytes[i / 8] |= 1 << (i % 8);
    stream << bytes;
}

template<size_t N>
void readBitset(TRawIStream& stream, bitset<N>* pBits)
{
    unsigned char bytes[(N + 7) / 8];
    stream >> bytes;
    for (size_t i = 0; i < N; i++)
        (*pBits)[i] = (bytes[i / 8] >> (i % 8)) & 1;
}

#endif  /* HOMM3_EDITOR_RAWSTREAM_H */
