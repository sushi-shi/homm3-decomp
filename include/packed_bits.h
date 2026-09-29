#ifndef HOMM3_PACKED_BITS_H
#define HOMM3_PACKED_BITS_H

#include "abstractfile.h"
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
// The loop stays in this body with a named bit proxy: delegating it to
// decodePackedBits changes every Windows expansion (hero::load 99.99 ->
// 94.92%, NewSMapHeader::read 94.91 -> 88.11%, ScenarioStruct::read
// 90.62 -> 84.44%).
template <size_t N>
std::bitset<N> readPackedBits(TAbstractFile* infile)
{
    std::bitset<N> result;
    unsigned char packed[(N + 7) / 8];
    infile->read(packed, sizeof(packed));
    for (unsigned int index = 0; index < N; ++index) {
        typename std::bitset<N>::reference bit = result[index];
        bit = (packed[index >> 3] & (1 << (index & 7))) != 0;
    }
    return result;
}

#endif
