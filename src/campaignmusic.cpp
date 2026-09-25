// campaignmusic.cpp - the game's music-cue table: one pass over CmpMusic.txt
// that copies its forty-nine track names into a single pooled allocation and
// hangs them off the cue rows at 0x66c090.

// THIS COMPILAND IS ABSENT FROM THE DREAMCAST ROSTER; see campaignmusic.h for
// why the name is an inference the link order bounds rather than proves. Its
// whole .text contribution is the one body below plus the compiler-generated
// static-destructor wrapper at 0x45e3b0, bracketed by static-initializer runs
// of the shape netmsg.obj and campaign.obj carry (a lone 32-byte guard row at
// 0x45e230, and 32/89/96/97 plus seven ~95-byte bitset initializers at
// 0x45e3d0..0x45e7bf). Those are the excluded initializer class.

// The body is campaignmap.obj's InitializeCampaignMapTraitsTable (0x45dee0)
// down to the register: the same TResourcePtr guard, the same
// TAutoArrayPtr<char> function-local static with its atexit wrapper eight
// bytes past its own guard byte, the same measure-then-copy pair of passes.
#include "va.h"

#include <string.h>

#include "campaignmusic.h"

#include "ownership.h"
#include "resourcemanager.h"
#include "textresource.h"

// Retail initial data; dimensions follow the typed table consumers.
DATA(0x0066c090) SCampaignMusicCue g_campaignMusicCues[49] = {
    { "CampainMusic01", 0 },
    { "CampainMusic02", 0 },
    { "CampainMusic03", 0 },
    { "CampainMusic04", 0 },
    { "CampainMusic05", 0 },
    { "CampainMusic06", 0 },
    { "CampainMusic07", 0 },
    { "CampainMusic08", 0 },
    { "CampainMusic09", 0 },
    { "AiTheme0", 0 },
    { "AiTheme1", 0 },
    { "AiTheme2", 0 },
    { "Combat01", 0 },
    { "Combat02", 0 },
    { "Combat03", 0 },
    { "Combat04", 0 },
    { "CstleTown", 0 },
    { "TowerTown", 0 },
    { "Rampart", 0 },
    { "InfernoTown", 0 },
    { "NecroTown", 0 },
    { "Dungeon", 0 },
    { "Stronghold", 0 },
    { "FortressTown", 0 },
    { "ElemTown", 0 },
    { "Dirt", 0 },
    { "Sand", 0 },
    { "Grass", 0 },
    { "Snow", 0 },
    { "Swamp", 0 },
    { "Rough", 0 },
    { "Underground", 0 },
    { "Lava", 0 },
    { "Water", 0 },
    { "GoodTheme", 0 },
    { "NeutralTheme", 0 },
    { "EvilTheme", 0 },
    { "SecretTheme", 0 },
    { "LoopLepr", 0 },
    { "MainMenu", 0 },
    { "Win Scenario", 0 },
    { "CampainMusic10", 0 },
    { "BladeABCampaign", 0 },
    { "BladeDBCampaign", 0 },
    { "BladeDSCampaign", 0 },
    { "BladeFLCampaign", 0 },
    { "BladeFWCampaign", 0 },
    { "BladePFCampaign", 0 },
    { "CampainMusic11", 0 }
};

VA(0x0045e250, 0x160) MAC_ADDRESS(0x21dd80, 0x1c4)
unsigned char initializeCampaignMusicTable()
{
    TResourcePtr<TTextResource> textResource(
        ResourceManager::getText(
            DATA_COMPGEN(0x0066c484, campaignMusicTextName, "CmpMusic.txt")));
    if (!textResource.get())
        return 0;

    unsigned strSize = 0;
    unsigned cue;
    for (cue = 0; cue < CAMPAIGN_MUSIC_CUE_COUNT; ++cue)
        strSize += strlen(textResource->getText(cue)) + 1;

    DATA_COMPGEN_GUARD(0x00694e18, campaignMusicTracksGuard, campaignMusicTracks)
    VA_COMPGEN(0x0045e3b0, 0x16, STATIC_DTOR, campaignMusicTracks)
    DATA(0x00694e20)
    static TAutoArrayPtr<char> campaignMusicTracks(new char[strSize]);
    if (!campaignMusicTracks.get())
        return 0;

    char* destination = campaignMusicTracks.get();
    for (cue = 0; cue < CAMPAIGN_MUSIC_CUE_COUNT; ++cue) {
        const char* source = textResource->getText(cue);
        unsigned length = strlen(source) + 1;
        memcpy(destination, source, length);
        g_campaignMusicCues[cue].m_track = destination;
        destination += length;
    }
    return 1;
}
