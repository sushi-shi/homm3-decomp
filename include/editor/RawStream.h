// RawStream.h - the editors' unformatted streams over a streambuf. Loki's
// h3maped exports TRawIStream(streambuf*) and TRawOStream(streambuf*)
// (19 bytes each, one member), the member templates operator>> <T> and
// operator<< <T> for the scalars and char arrays of the map format, and the
// nested failures TRawIStream::TReadFailure and TRawOStream::TWriteFailure
// (RTTI .?AVTReadFailure@TRawIStream@@, .?AVTWriteFailure@TRawOStream@@ in
// both Windows editors).
#ifndef HOMM3_EDITOR_RAWSTREAM_H
#define HOMM3_EDITOR_RAWSTREAM_H

#include <algorithm>
#include <bitset>
#include <streambuf>
#include <string>
#include <vector>

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

    TRawIStream& read(char* pData, unsigned int count)
    {
        int nRead = m_pStreamBuf->sgetn(pData, count);
        if (nRead < count)
            throw TReadFailure(nRead);
        return *this;
    }

private:
    std::streambuf* m_pStreamBuf;
};

// A string: its length, then its characters through a 512-byte buffer
// (h3maped 0x4190cb, cdecl; h3ccmped expands it in the campaign readers).
inline TRawIStream& operator>>(TRawIStream& stream, std::string& value)
{
    value.erase(value.begin(), value.end());
    long size;
    stream >> size;
    value.resize(size);
    std::string::size_type remaining = size;
    for (std::string::iterator dest = value.begin(); remaining > 0;) {
        char buffer[512];
        std::string::size_type count = remaining < sizeof(buffer) ? remaining : sizeof(buffer);
        stream.read(buffer, count);
        std::copy(buffer, buffer + count, dest);
        dest += count;
        remaining -= count;
    }
    return stream;
}

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

    TRawOStream& write(const char* pData, unsigned int count)
    {
        int nWritten = m_pStreamBuf->sputn(pData, count);
        if (nWritten < count)
            throw TWriteFailure(nWritten);
        return *this;
    }

private:
    std::streambuf* m_pStreamBuf;
};

// A string: its length, then its characters (h3maped 0x428afc, cdecl).
inline TRawOStream& operator<<(TRawOStream& stream, const std::string& value)
{
    stream << static_cast<int>(value.length());
    stream.write(value.c_str(), value.length());
    return stream;
}

// A bit set as the map format packs it: (N + 7) / 8 bytes, bit 0 of byte 0
// first (the map's artifact and hero masks, a hero's spells: h3maped
// 0x433db5, 0x434288).
template <size_t N>
void readBitset(TRawIStream& stream, std::bitset<N>& bits)
{
    unsigned char aByte[(N + 7) / 8];
    stream >> aByte;
    for (size_t i = 0; i < N; i++)
        bits.set(i, (aByte[i / 8] & (1 << (i % 8))) != 0);
}

template <size_t N>
void writeBitset(TRawOStream& stream, const std::bitset<N>& bits)
{
    unsigned char aByte[(N + 7) / 8] = { 0 };
    for (size_t i = 0; i < N; i++)
        if (bits.test(i))
            aByte[i / 8] |= 1 << (i % 8);
    stream << aByte;
}

// A counted sequence: the count, then each item (the map's rumors, 0x433f90).
template <class T>
void readContainer(TRawIStream& stream, std::vector<T>& aItem)
{
    aItem.erase(aItem.begin(), aItem.end());
    unsigned int numItems;
    stream >> numItems;
    for (unsigned int i = 0; i < numItems; i++) {
        T item;
        stream >> item;
        aItem.push_back(item);
    }
}

template <class T>
void writeContainer(TRawOStream& stream, const std::vector<T>& aItem)
{
    stream << static_cast<unsigned int>(aItem.size());
    for (std::vector<T>::const_iterator pItem = aItem.begin(); pItem != aItem.end(); ++pItem)
        stream << *pItem;
}

#endif  /* HOMM3_EDITOR_RAWSTREAM_H */
