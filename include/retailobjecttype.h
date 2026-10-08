// retailobjecttype.h - Complete's adventure-object types after the
// Dreamcast domain (adventureobjecttype.h): DispatchEvent's and the map
// loader's arms in advmgr, and the map editor's object tables, which use it
// without advmgr.h.
#ifndef HOMM3_RETAILOBJECTTYPE_H
#define HOMM3_RETAILOBJECTTYPE_H

enum EAdvmgrRetailObjectType {
    BORDER_GATE = 212,
    // 213: DispatchEvent's 0xd5 arm gates DoFreelancersGuild(hero*) on
    // human_player - the map-object entry of tradpost's guild pair.
    FREELANCERS_GUILD = 213,
    HERO_PLACEHOLDER = 214,
    QUEST_GUARD = 215,
    RANDOM_DWELLING = 216,
    RANDOM_DWELLING_LVL = 217,
    RANDOM_DWELLING_FACTION = 218,
    // The map editor's object caps pair 219 with GARRISON and 220 with MINE
    // (h3maped's capped-type tables at 0x5351d0/0x5351e8). The spellings
    // are H3API's; no original name survives.
    GARRISON2 = 219,
    ABANDONED_MINE = 220
};

#endif  // HOMM3_RETAILOBJECTTYPE_H
