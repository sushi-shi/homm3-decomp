#ifndef HOMM3_PACKED_BITS_H
#define HOMM3_PACKED_BITS_H

#include "abstractfile.h"
#include <bitset>
#include <algorithm>

// Decode into an existing mask; the stream reader below owns its returned
// value and packed buffer. Both loops are expanded in the Mac callers.
template <size_t N>
inline void decodePackedBits(const unsigned char* packed, std::bitset<N>& result)
{
    for (unsigned int index = 0; index < N; ++index) {
        result[index] = (packed[index >> 3] & (1 << (index & 7))) != 0;
    }
}

// Complete's map and campaign readers deserialize packed planes through a
// returned bitset temporary. The source name and header location are inferred.
// Mac hero::load (-O1, 0xf3400..0xf3478) expands this reader and its decode
// loop, leaving the bitset<48>::set call: the reader is inline and owns the
// loop directly. An out-of-line reader leaves a readPackedBits call, and a
// nested decodePackedBits leaves that call instead of set. The owned loop
// takes VC6 hero::load 94.92 -> 100%, NewSMapHeader::read 94.09 -> 97.03%,
// game::loadMap 85.03 -> 88.60% and ScenarioStruct::read 84.44 -> 89.67%;
// the inline keyword alone is VC6 byte-flat.
// Probe (2026-10-08): result.set(index, bit) in the loop lifts loadMap
// 88.60 -> 93.14 but drops NewSMapHeader::read/load, ScenarioStruct::read
// and hero::load and leaves four retained set/reference bodies unemitted.
template <size_t N>
inline std::bitset<N> readPackedBits(TAbstractFile* infile)
{
    std::bitset<N> result;
    unsigned char packed[(N + 7) / 8];
    infile->read(packed, sizeof(packed));
    for (unsigned int index = 0; index < N; ++index)
        result[index] = (packed[index >> 3] & (1 << (index & 7))) != 0;
    return result;
}

// The same LSB-first writer expands in playerData::save (Mac 0xcd1f0)
// and hero::save (Mac 0xf3d84). Cross-TU use owns this shared header body.
// Scalar output uses the canonical writeValue in abstractfile.h. The retained
// editor writers at 0x4aed89..0x4af09b own their packed
// bytes, checked bit tests and stream write, then return whether the complete
// byte count was written. Loki's writeBitset independently keeps these three
// operations in one helper. The former separate encoder had no native claim
// or independent caller and incorrectly split that operation. All six editor
// instances match retail with the counted zero fill also expanded by Mac.
template <size_t N>
int writePackedBits(TAbstractFile* outfile, const std::bitset<N>& bits)
{
    unsigned char packed[(N + 7) / 8];
    std::fill_n(packed, sizeof(packed), 0);
    for (unsigned int index = 0; index < N; ++index) {
        if (bits[index])
            packed[index >> 3] |= 1 << (index & 7);
    }
    return outfile->write(packed, sizeof(packed)) == sizeof(packed);
}

#endif
