// seerhut.h - canonical retail quest-guard and seer-hut layouts.
#ifndef HOMM3_SEERHUT_H
#define HOMM3_SEERHUT_H

#include <string>
#include <va.h>

class type_quest;

#pragma pack(push, 1)

// Retail's constructor family and NewfullMap vector walks prove the packed
// five-byte base: a quest pointer followed by the visited-player mask.
class TAbstractFile;

class TQuestGuard {
public:
    type_quest* quest;
    unsigned char visitedPlayers;

    // NOT inline for this view: readObject (0x502e00) CALLS the constructor
    // at 0x572b50 on its quest-guard local instead of expanding the two
    // stores, so the declaration retail's mapcell.cpp saw was this one and
    // the definition lived in seerhut.cpp. The inline body stays for the
    // views whose own call sites do expand it.
    TQuestGuard();

    // The h3m reader, reached from readObject's QUEST_GUARD arm with the
    // stream as its one argument. DECLARED, not defined: the body is an
    // unclaimed carve row outside this compiland.
    int read(TAbstractFile* infile);

    // TWO TEXT BUILDERS, NOT ONE (2026-08-20). 0x572e40 and 0x573040 are
    // 511 B each and byte-identical apart from ONE relocation - the
    // separator literal they append, "\n\n" (0x6603b0) against " "
    // (0x660330). advManager::QuickInfo calls the FIRST and
    // advManager::SetRolloverText the second; the carve's own caller-derived
    // names say so (`game_137c0_sub00_172e40` against
    // `game_b150_sub07_173040`, 0x137c0 being QuickInfo and 0xb150
    // SetRolloverText) and both bodies confirm it by their separator. The
    // TSeerHut pair below splits the same way, crosswise.
    std::string QuestGuardFn_00572E40(int player);
    std::string QuestGuardFn_00573040(int player);
    // 0x572d60, 224 B, carved and unclaimed. NULLARY where the pair above
    // takes a player: TQuestLogWindow::UpdateQuestLocator pushes only the
    // hidden return buffer and calls it on the QuestGuardList element,
    // then strcpy's its c_str() into gText. Provisional name.
    std::string QuestGuardFn_00572D60();
    // Reached from NewfullMap::Save, which calls it on every QuestGuardList
    // element with the stream as its one argument. DECLARED, not defined:
    // the body is an unclaimed carve row outside this compiland.
    int save(TAbstractFile* outfile);
#ifdef HOMM3_QUEST_GUARD_LOAD_DECLS
    int load(TAbstractFile* infile, int saveVersion);
#endif

protected:
    // TSeerHut initializes the shared bytes in its own body; retail's store
    // order proves that its reward constructor runs before those assignments.
    explicit TQuestGuard(int) {}
};
SIZE(TQuestGuard, 0x5);

// Bytes +5..+0x10 of TSeerHut. The constructor initializes the common type
// word; the remaining eight bytes are the selected reward's payload.
struct TSeerReward {
    int rewardType;
    union {
        char payload[8];
        int dwords[2];
    } value;

    TSeerReward() : rewardType(0) {}
};
SIZE(TSeerReward, 0xc);

// Retail indexes NewfullMap::SeerHutList with a 0x13 stride and the constructor
// writes every named state byte below. This is the one class layout used by
// seerhut, mapcell, and advmgr; there is no TU-private vector projection.
class TSeerHut : private TQuestGuard {
    // readObject's SEER arm tests the base's `quest` pointer on the local it
    // just deserialized, before deciding whether to register it in the
    // +0xb0 pool. Friendship rather than a public base: only the map reader
    // reaches across, and everything else here still goes through TSeerHut's
    // own surface.
    friend class NewfullMap;

public:
    TSeerReward reward;
    signed char NameIndex;
    unsigned char field_12;

    TSeerHut();

    // The SeerHutList twin of TQuestGuard::read, reached the same way from
    // readObject's SEER arm. Declared separately because the TQuestGuard
    // base is private here.
    int read(TAbstractFile* infile);
    // 0x574a90, `ret 8` - the savegame reader, called by NewfullMap::Load on
    // every element of the list it has just resized.
    int load(TAbstractFile* infile, int saveVersion);

    // The TQuestGuard pair's twin, and it splits CROSSWISE: 0x5741b0 and
    // 0x5743e0 are 556 B each and differ only in the separator relocation,
    // with 0x5741b0 taking " " (SetRolloverText) and 0x5743e0 taking "\n\n"
    // (QuickInfo). Same carve-name evidence: `game_b150_sub08_1741b0`
    // against `game_137c0_sub01_1743e0`.
    std::string SeerHutFn_005741B0(int player);
    std::string SeerHutFn_005743E0(int player);
    // 0x574070, 312 B, carved and unclaimed - the SeerHutList twin of
    // TQuestGuard::QuestGuardFn_00572D60 and reached from the same
    // quest-log body, on the other arm of its SeerHutList/QuestGuardList
    // split. Provisional name.
    std::string SeerHutFn_00574070();
    // 0x573fd0, the SeerHutList twin of TQuestGuard::save and reached the
    // same way from NewfullMap::Save. Declared, not defined - and declared
    // separately because the TQuestGuard base is private here.
    int save(TAbstractFile* outfile);
};
SIZE(TSeerHut, 0x13);

#pragma pack(pop)

#endif  // HOMM3_SEERHUT_H
