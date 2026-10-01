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

// Retail .data 0x66b7d0 holds 101 {image name, 0} rows ("CslReG1a.pcx" ..
// "Csl2UAm.pcx", one per campaign scenario), a pointer to the table at
// 0x66baf8 (the g_campaignMusicTraits pattern) and then the rows' pooled
// names, all before the cue table below. No retail instruction or datum
// reaches the table or its pointer. The names, the row type and the owning
// compiland are inferred: the rows sit between campaignmap.obj's literals
// and this compiland's cue table.
DATA(0x0066b7d0) SCampaignScenarioImage g_campaignScenarioImages[101] = {
    { "CslReG1a.pcx", 0 },
    { "CslReG1b.pcx", 0 },
    { "CslReG1c.pcx", 0 },
    { "CslReE3a.pcx", 0 },
    { "CslReE3b.pcx", 0 },
    { "CslReE3c.pcx", 0 },
    { "CslReN2a.pcx", 0 },
    { "CslReN2b.pcx", 0 },
    { "CslReG1d.pcx", 0 },
    { "CslReG4a.pcx", 0 },
    { "CslReG4b.pcx", 0 },
    { "CslReG4c.pcx", 0 },
    { "CslReG4d.pcx", 0 },
    { "CslReE5a.pcx", 0 },
    { "CslReE5e.pcx", 0 },
    { "CslReE5b.pcx", 0 },
    { "CslReE5c.pcx", 0 },
    { "CslReE5d.pcx", 0 },
    { "CslReG6a.pcx", 0 },
    { "CslReG6b.pcx", 0 },
    { "CslReG6c.pcx", 0 },
    { "CslReS7a.pcx", 0 },
    { "CslReS7b.pcx", 0 },
    { "CslReS7c.pcx", 0 },
    { "CslAbAb1.pcx", 0 },
    { "CslAbAb2.pcx", 0 },
    { "CslAbAb3.pcx", 0 },
    { "CslAbAb4.pcx", 0 },
    { "CslAbAb5.pcx", 0 },
    { "CslAbAb6.pcx", 0 },
    { "CslAbAb7.pcx", 0 },
    { "CslAbAb8.pcx", 0 },
    { "CslAbAb9.pcx", 0 },
    { "CslAbDb1.pcx", 0 },
    { "CslAbdb2.pcx", 0 },
    { "CslAbDb3.pcx", 0 },
    { "CslAbDb4.pcx", 0 },
    { "CslAbDb5.pcx", 0 },
    { "CslAbds1.pcx", 0 },
    { "CslAbDs2.pcx", 0 },
    { "CslAbDs3.pcx", 0 },
    { "CslAbDs4.pcx", 0 },
    { "CslAbDs5.pcx", 0 },
    { "CslAbFl1.pcx", 0 },
    { "CslAbFl2.pcx", 0 },
    { "CslAbfl3.pcx", 0 },
    { "CslAbFl4.pcx", 0 },
    { "CslAbFl5.pcx", 0 },
    { "CslAbfw1.pcx", 0 },
    { "CslAbFw2.pcx", 0 },
    { "CslAbFw3.pcx", 0 },
    { "CslAbFw4.pcx", 0 },
    { "CslAbFw5.pcx", 0 },
    { "CslAbPf1.pcx", 0 },
    { "CslAbpf2.pcx", 0 },
    { "CslAbPf3.pcx", 0 },
    { "CslAbPf4.pcx", 0 },
    { "Csl2BBa.pcx", 0 },
    { "Csl2BBb.pcx", 0 },
    { "Csl2BBc.pcx", 0 },
    { "Csl2BBd.pcx", 0 },
    { "Csl2BBe.pcx", 0 },
    { "Csl2BBf.pcx", 0 },
    { "Csl2ELa.pcx", 0 },
    { "Csl2ELb.pcx", 0 },
    { "Csl2ELc.pcx", 0 },
    { "Csl2ELd.pcx", 0 },
    { "Csl2ELe.pcx", 0 },
    { "Csl2HSa.pcx", 0 },
    { "CslReE5c.pcx", 0 },
    { "Csl2HSc.pcx", 0 },
    { "Csl2HSd.pcx", 0 },
    { "Csl2HSe.pcx", 0 },
    { "Csl2NBa.pcx", 0 },
    { "Csl2NBb.pcx", 0 },
    { "Csl2NBc.pcx", 0 },
    { "Csl2NBd.pcx", 0 },
    { "Csl2NBe.pcx", 0 },
    { "Csl2RNa.pcx", 0 },
    { "Csl2RNb.pcx", 0 },
    { "Csl2RNc.pcx", 0 },
    { "Csl2RNd.pcx", 0 },
    { "Csl2RNe.pcx", 0 },
    { "Csl2SPa.pcx", 0 },
    { "Csl2SPb.pcx", 0 },
    { "Csl2SPc.pcx", 0 },
    { "Csl2SPd.pcx", 0 },
    { "Csl2SPe.pcx", 0 },
    { "Csl2UAa.pcx", 0 },
    { "Csl2UAb.pcx", 0 },
    { "Csl2UAc.pcx", 0 },
    { "Csl2UAd.pcx", 0 },
    { "Csl2UAe.pcx", 0 },
    { "Csl2UAf.pcx", 0 },
    { "Csl2UAg.pcx", 0 },
    { "Csl2UAh.pcx", 0 },
    { "Csl2UAi.pcx", 0 },
    { "Csl2UAj.pcx", 0 },
    { "Csl2UAk.pcx", 0 },
    { "Csl2UAl.pcx", 0 },
    { "Csl2UAm.pcx", 0 }
};
DATA(0x0066baf8) const SCampaignScenarioImage* g_campaignScenarioImageTraits =
    g_campaignScenarioImages;

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

VA(0x0045e250, 0x160)
MAC_ADDRESS(0x21dd80, 0x1c4)
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
        unsigned length = copyResourceString(destination, source);
        g_campaignMusicCues[cue].m_track = destination;
        destination += length;
    }
    return 1;
}
