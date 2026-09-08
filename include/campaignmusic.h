// campaignmusic.h - prototypes of campaignmusic.cpp.
//
// THE COMPILAND IS ABSENT FROM THE DREAMCAST ROSTER and its name is an
// inference the link order bounds rather than proves (the netmsg.obj
// precedent): retail's .text is strictly alphabetical by compiland, and this
// object's single body sits between campaignmap.obj (whose last row is the
// static-dtor wrapper 0x45e1f0) and campaignwindow.obj (whose first row is
// 0x45e7c0). `campaignmusic` is the spelling the loaded file, CmpMusic.txt,
// suggests inside that interval. HAND-OWNED, provisional.
#ifndef HOMM3_CAMPAIGNMUSIC_H
#define HOMM3_CAMPAIGNMUSIC_H

#include <va.h>

// One row of the game's music-cue table: the cue's engine name and the track
// name CmpMusic.txt supplies for it. Retail .data 0x66c090, forty-nine
// eight-byte rows ending exactly on 0x66c218 - the loader steps the second
// dword from 0x66c094 by eight while `ecx < 0x66c21c`, and the FIRST dword of
// every row is already a string pointer in the image ("CampainMusic01" ..
// "CampainMusic11", the nine AI/combat themes, the nine town themes, the nine
// terrain themes, the four alignment themes, LoopLepr, MainMenu,
// "Win Scenario" and the six Armageddon's Blade campaign cues). Names
// INVENTED - no Dreamcast row covers this table.
struct SCampaignMusicCue {
    // Before normalization: name.
    const char* m_name;
    // Before normalization: track.
    char* m_track;
};
SIZE(SCampaignMusicCue, 8);

// Forty-nine cues, and the loader's own loop bound twice over: its first pass
// walks the text resource to a byte offset of 0xc4 (49 * 4) and its second
// stops the table cursor at 0x66c21c (0x66c094 + 49 * 8).
enum ECampaignMusicConstants {
    CAMPAIGN_MUSIC_CUE_COUNT = 49
};

// Retail .data 0x66c090. The row NAMES are part of the image's initialized
// data and this compiland's own static initializer emits them; declared here
// without a definition (the bitNumber pattern) so the loader's relocations
// have source authority without fabricating the initializer.
// Before normalization: gCampaignMusicCues.
extern SCampaignMusicCue g_campaignMusicCues[CAMPAIGN_MUSIC_CUE_COUNT];

// Retail 0x45e250, called once from kb.obj's EarlySetup between
// InitializeHeroSpecificAbilitiesTable and InterpretCommandLine. Name
// INVENTED, in the family spelling EarlySetup's other callees use.
// Before normalization (function): InitializeCampaignMusicTable.
unsigned char initializeCampaignMusicTable();

#endif  /* HOMM3_CAMPAIGNMUSIC_H */
