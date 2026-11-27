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
    { "CampainMusic01", NULL },
    { "CampainMusic02", NULL },
    { "CampainMusic03", NULL },
    { "CampainMusic04", NULL },
    { "CampainMusic05", NULL },
    { "CampainMusic06", NULL },
    { "CampainMusic07", NULL },
    { "CampainMusic08", NULL },
    { "CampainMusic09", NULL },
    { "AiTheme0", NULL },
    { "AiTheme1", NULL },
    { "AiTheme2", NULL },
    { "Combat01", NULL },
    { "Combat02", NULL },
    { "Combat03", NULL },
    { "Combat04", NULL },
    { "CstleTown", NULL },
    { "TowerTown", NULL },
    { "Rampart", NULL },
    { "InfernoTown", NULL },
    { "NecroTown", NULL },
    { "Dungeon", NULL },
    { "Stronghold", NULL },
    { "FortressTown", NULL },
    { "ElemTown", NULL },
    { "Dirt", NULL },
    { "Sand", NULL },
    { "Grass", NULL },
    { "Snow", NULL },
    { "Swamp", NULL },
    { "Rough", NULL },
    { "Underground", NULL },
    { "Lava", NULL },
    { "Water", NULL },
    { "GoodTheme", NULL },
    { "NeutralTheme", NULL },
    { "EvilTheme", NULL },
    { "SecretTheme", NULL },
    { "LoopLepr", NULL },
    { "MainMenu", NULL },
    { "Win Scenario", NULL },
    { "CampainMusic10", NULL },
    { "BladeABCampaign", NULL },
    { "BladeDBCampaign", NULL },
    { "BladeDSCampaign", NULL },
    { "BladeFLCampaign", NULL },
    { "BladeFWCampaign", NULL },
    { "BladePFCampaign", NULL },
    { "CampainMusic11", NULL }
};

DATA(0x0066c218) const SCampaignMusicCue* g_campaignMusicTraits = g_campaignMusicCues;

VA(0x0045e250, 0x160)
MAC_ADDRESS(0x21dd80, 0x1c4)
unsigned char initializeCampaignMusicTable()
{
    TResourcePtr<TTextResource> textResource(
        ResourceManager::GetText(
            DATA_COMPGEN(0x0066c484, campaignMusicTextName, "CmpMusic.txt")));
    if (!textResource.get())
        return 0;

    unsigned strSize = 0;
    unsigned cue;
    for (cue = 0; cue < CAMPAIGN_MUSIC_CUE_COUNT; ++cue)
        strSize += strlen(textResource->GetText(cue)) + 1;

    DATA_COMPGEN_GUARD(0x00694e18, campaignMusicTracksGuard, campaignMusicTracks)

    VA_COMPGEN(0x0045e3b0, 0x16, STATIC_DTOR, campaignMusicTracks)
    DATA(0x00694e20)
    static TAutoArrayPtr<char> campaignMusicTracks(new char[strSize]);
    if (!campaignMusicTracks.get())
        return 0;

    char* destination = campaignMusicTracks.get();
    for (cue = 0; cue < CAMPAIGN_MUSIC_CUE_COUNT; ++cue) {
        const char* source = textResource->GetText(cue);
        unsigned length = strlen(source) + 1;
        memcpy(destination, source, length);
        g_campaignMusicCues[cue].m_track = destination;
        destination += length;
    }
    return 1;
}
