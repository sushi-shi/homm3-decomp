#ifndef HOMM3_ARTIFACT_DATA_H
#define HOMM3_ARTIFACT_DATA_H

#include <bitset>

// The combination-artifact record. The 24-byte stride is byte-proven by
// IsWieldingArtifact's `lea r,[id + 2*id]` / `[base + 8*r]` chain, and
// +0x00 by the same body - it is the assembled artifact's own id, which
// IsWieldingArtifact recurses on. The remaining 20 bytes are the
// component mask the four retail-only combination bodies at
// 0x4dbe80..0x4dc100 walk as a bitset<144> (five dwords). This is the
// canonical record used by the artifact table and all its consumers.
struct TCombinationArtifact {
    // The cinit at 0x44c960 builds each of the twelve records in a 24-byte
    // stack temporary - the id dword stored FIRST, then the component
    // builder's five-word result copied in behind it - and only then
    // `rep movsd`s the whole temporary into the table. That is an inlined
    // two-argument constructor plus the implicit copy, and VC6 cannot
    // spell it any other way: brace initialization of a record carrying a
    // bitset member is a hard C2440 for this compiler.
    TCombinationArtifact(int id, const std::bitset<144>& usedComponents)
        : m_artifactId(id), m_components(usedComponents) {}

    int m_artifactId;             // +0x00
    std::bitset<144> m_components;
};

// One allowable-slot class: the slots an artifact of that class may occupy.
// Like TCombinationArtifact, each table entry is constructed from a
// separately built mask. The cinit at 0x44cc00 proves the one-argument
// constructor: its sixteen inline candidate sites divide the budget so that
// the empty entry's nested bitset::_Tidy stays a call (0x44cc09), which a
// plain bitset array expands. Mac 0x5ba74..0x5bbe0 reserves the matching
// second stack temporary per entry (0x108, 0x100, 0xf8, ...), as CodeWarrior
// does for this type and not for bitset elements. Readers use each entry as
// the bitset itself, as the game-context table's readers must (see
// gamecontext.cpp); a bitset member here leaves the hero and swapmgr readers
// unchanged. The original type name is unknown.
struct TArtifactSlotMask : public std::bitset<19> {
    TArtifactSlotMask(const std::bitset<19>& slots) : std::bitset<19>(slots) {}
};

#endif
