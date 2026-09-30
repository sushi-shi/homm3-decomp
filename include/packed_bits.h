#ifndef HOMM3_PACKED_BITS_H
#define HOMM3_PACKED_BITS_H

#include "abstractfile.h"
#include <algorithm>
#include <bitset>

// Decode into an existing mask; the stream reader below owns its returned
// value and packed buffer. Both loops are expanded in the Mac callers.
template <size_t N>
void decodePackedBits(const unsigned char* packed, std::bitset<N>& result)
{
    for (unsigned int index = 0; index < N; ++index) {
        result[index] = (packed[index >> 3] & (1 << (index & 7))) != 0;
    }
}

// Complete's map and campaign readers deserialize packed planes through a
// returned bitset temporary. The source name and header location are inferred.
// The reader delegates its loop to decodePackedBits (both are expanded in the
// Mac callers). Controls: a private named-proxy loop here raises
// NewSMapHeader::read to 94.92% (92.61% delegating); a named proxy inside
// decodePackedBits lowers it to 89.83%.
template <size_t N>
std::bitset<N> readPackedBits(TAbstractFile* infile)
{
    std::bitset<N> result;
    unsigned char packed[(N + 7) / 8];
    infile->read(packed, sizeof(packed));
    decodePackedBits(packed, result);
    return result;
}

// Mac expands this encoder and its stream writer in both random-map output
// (0x24f83c..0x24f890, among other widths) and NewSMapHeader::save
// (0xdbe4c..0xdbeb0). Windows 0x4c544d..0x4c5497 likewise packs an indexed
// byte array before writing it. Cross-TU expansions support this shared
// template body; its original name and header are unknown. playerData::save
// expands the same writer at Mac 0xcd1f0..0xcd258; fill_n retains its repeated
// const-zero loads without the extra CRT call introduced by memset.
template <size_t N>
void encodePackedBits(const std::bitset<N>& bits, unsigned char* packed)
{
    std::fill_n(packed, (N + 7) / 8, 0);
    for (unsigned int index = 0; index < N; ++index) {
        if (bits.test(index))
            packed[index >> 3] |= 1 << (index & 7);
    }
}

template <size_t N>
int writePackedBits(TAbstractFile* outfile, const std::bitset<N>& bits)
{
    unsigned char packed[(N + 7) / 8];
    encodePackedBits(bits, packed);
    return outfile->write(packed, sizeof(packed));
}

#endif
