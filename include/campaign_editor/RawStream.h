// RawStream.h - the campaign editor's copy of the map editor's stream header
// (include/editor/RawStream.h). It keeps that file's include guard, as a
// copied header does: a campaign editor object includes this copy before any
// map editor header, and the map editor headers' own include of RawStream.h
// then adds nothing.
//
// The copy differs in two ways the campaign editor's code shows:
// - Each scalar read and write goes through a second member template. The
//   campaign editor calls the per-type body (h3ccmped 0x416dd0 reads a byte,
//   0x40ffd0 a dword) where the outer operator is expanded, so its readers
//   call the early reads out of line and expand later ones, and the map
//   header reader (0x422b70) calls every one. The map editor's operators
//   hold the body themselves (Loki's __rs__H1ZSc_11TRawIStream).
// - The bit set helpers index the set (bitset::reference) instead of calling
//   bitset::set and test: the crossover options (0x407ee0) and the map header
//   reader (0x422b70) expand them that way.
#ifndef HOMM3_EDITOR_RAWSTREAM_H
#define HOMM3_EDITOR_RAWSTREAM_H

#include <algorithm>
#include <bitset>
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
        return _read(value);
    }

    TRawIStream& read(char* pData, unsigned int count)
    {
        int nRead = m_pStreamBuf->sgetn(pData, count);
        if (nRead < count)
            throw TReadFailure(nRead);
        return *this;
    }

private:
    template <class T>
    TRawIStream& _read(T& value)
    {
        int nRead = m_pStreamBuf->sgetn(reinterpret_cast<char*>(&value), sizeof(T));
        if (nRead < sizeof(T))
            throw TReadFailure(nRead);
        return *this;
    }

    std::streambuf* m_pStreamBuf;
};

#include "editor/RawStreamValue.h"

// Loki readToIter<char*> (0x08346820) retains this iterator-pair reader.
// Complete TScenarioRecord::read 0x412522..0x412533 likewise caches the range
// end and advances the current iterator before each scalar read.
template <class Iter>
TRawIStream& readToIter(TRawIStream& stream, Iter first, Iter last)
{
    while (first != last)
        stream >> *first++;
    return stream;
}

// A string: its length, then its characters through a 512-byte buffer.
// Loki delegates to readToIter (0x08346820). Counted-template and byte-overload
// probes retain five calls in GameMapHeader where Complete expands these loops;
// the corresponding Complete buffered helper boundary remains unproven.
// Complete evaluates the erase range in different orders in the map-header
// and prologue readers; leave both iterator calls in the expression. The
// serialized count is saved before resizing in the map-header reader.
inline TRawIStream& operator>>(TRawIStream& stream, std::string& value)
{
    value.erase(value.begin(), value.end());
    long size = readValue<long>(stream);
    std::string::size_type remaining = size;
    value.resize(size);
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
        return _write(value);
    }

    TRawOStream& write(const char* pData, unsigned int count)
    {
        int nWritten = m_pStreamBuf->sputn(pData, count);
        if (nWritten < count)
            throw TWriteFailure(nWritten);
        return *this;
    }

private:
    template <class T>
    TRawOStream& _write(const T& value)
    {
        int nWritten = m_pStreamBuf->sputn(reinterpret_cast<const char*>(&value), sizeof(T));
        if (nWritten < sizeof(T))
            throw TWriteFailure(nWritten);
        return *this;
    }

    std::streambuf* m_pStreamBuf;
};

// Loki writeFromIter<const char*> (0x0835fc90) retains this range writer.
// Complete TScenarioRecord::write 0x412aee..0x412b05 caches the range end
// and advances the current iterator before each scalar write.
template <class Iter>
TRawOStream& writeFromIter(TRawOStream& stream, Iter first, Iter last)
{
    while (first != last)
        stream << *first++;
    return stream;
}

// Loki writeContainer (0x0835c728) materializes a long length. Complete
// retains bulk character output rather than the older per-character loop.
inline TRawOStream& operator<<(TRawOStream& stream, const std::string& value)
{
    long size = value.size();
    stream << size;
    stream.write(value.c_str(), value.length());
    return stream;
}

// A bit set as the map format packs it: (N + 7) / 8 bytes, bit 0 of byte 0
// first.
// Loki 0x0837b920 retains readBitset<9> with a bitset output pointer.
template <size_t N>
inline void readBitset(TRawIStream& stream, std::bitset<N>* bits)
{
    unsigned char bytes[(N + 7) / 8];
    stream >> bytes;
    for (unsigned int i = 0; i < N; i++)
        (*bits)[i] = (bytes[i / 8] & (1 << (i % 8))) != 0;
}

// Loki 0x0837b580 clears the full buffer before const-subscript packing.
// Complete 0x408250 likewise clears each whole buffer before reading bits.
template <size_t N>
inline void writeBitset(TRawOStream& stream, const std::bitset<N>& bits)
{
    unsigned char bytes[(N + 7) / 8];
    std::fill_n(bytes, sizeof(bytes), 0);
    for (unsigned int i = 0; i < N; i++) {
        if (bits[i])
            bytes[i / 8] |= 1 << (i % 8);
    }
    stream << bytes;
}

#endif  /* HOMM3_EDITOR_RAWSTREAM_H */
