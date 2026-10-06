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

// Project-inferred signed-index conversions used by map spell and object
// masks. Keep division/modulo and direct bitset proxies from those callers;
// the existing unsigned decoder above has separate native source evidence.
template <size_t N>
void decodeMapBits(const unsigned char* packed, std::bitset<N>& result)
{
    for (int index = 0; index < static_cast<int>(N); ++index)
        result[index] = (packed[index / 8] & (1 << (index % 8))) != 0;
}

// Complete's map and campaign readers deserialize packed planes through a
// returned bitset temporary. The source name and header location are inferred.
// The reader keeps its own decode loop (Mac NewSMapHeader::load expands it at
// 0xdc678..0xdc6c0 as operator[] plus set). The Mac callers expand both loops, so
// no build shows it delegating to decodePackedBits, and the delegated form's
// extra nested inline level starves retail's caller budgets. The own loop
// recovers retail's operator[] expansions in ScenarioStruct::read (84.44 ->
// 89.67), game::loadMap (85.03 -> 88.60), NewSMapHeader::read (94.09 ->
// 97.03) and hero::load (94.92 -> 100); Mac pair scores are unchanged.
// A named proxy inside decodePackedBits lowered NewSMapHeader::read to 89.83%.
template <size_t N>
std::bitset<N> readPackedBits(TAbstractFile* infile)
{
    std::bitset<N> result;
    unsigned char packed[(N + 7) / 8];
    infile->read(packed, sizeof(packed));
    for (unsigned int index = 0; index < N; ++index)
        result[index] = (packed[index >> 3] & (1 << (index & 7))) != 0;
    return result;
}

#endif
