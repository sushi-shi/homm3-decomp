// campaignmusic.cpp - the game's music-cue table: one pass over CmpMusic.txt
// that copies its forty-nine track names into a single pooled allocation and
// hangs them off the cue rows at 0x66c090.
//
// THIS COMPILAND IS ABSENT FROM THE DREAMCAST ROSTER; see campaignmusic.h for
// why the name is an inference the link order bounds rather than proves. Its
// whole .text contribution is the one body below plus the compiler-generated
// static-destructor wrapper at 0x45e3b0, bracketed by static-initializer runs
// of the shape netmsg.obj and campaign.obj carry (a lone 32-byte guard row at
// 0x45e230, and 32/89/96/97 plus seven ~95-byte bitset initializers at
// 0x45e3d0..0x45e7bf). Those are the excluded initializer class.
//
// The body is campaignmap.obj's InitializeCampaignMapTraitsTable (0x45dee0)
// down to the register: the same TResourcePtr guard, the same
// TAutoArrayPtr<char> function-local static with its atexit wrapper eight
// bytes past its own guard byte, the same measure-then-copy pair of passes.
#include <va.h>
#include <string.h>

#include "campaignmusic.h"

#include "ownership.h"
#include "resourcemanager.h"
#include "textresource.h"

// Retail 0x45e250. Two passes over the resource - one to total the track
// names' lengths, one to copy them into the pool - with the resource released
// through the guard on every exit.
VA(0x0045e250, 0x160)  // anchor-string CmpMusic.txt + EarlySetup call edge, retail-only
unsigned char InitializeCampaignMusicTable()
{
    TResourcePtr<TTextResource> pTextResource(
        ResourceManager::GetText(
            DATA_COMPGEN(0x0066c484, campaignMusicTextName, "CmpMusic.txt")));
    if (!pTextResource.get())
        return 0;

    unsigned strSize = 0;
    unsigned cue;
    for (cue = 0; cue < CAMPAIGN_MUSIC_CUE_COUNT; ++cue)
        strSize += strlen(pTextResource->GetText(cue)) + 1;

    // The pool is the static's INITIALIZER, not an assignment into a
    // default-constructed one: retail runs the guard test at this point and
    // builds the object straight out of `operator new`'s result
    // (`setne cl` into _m_bOwns), which is one guard site and no
    // operator= at all.
    DATA_COMPGEN_GUARD(0x00694e18, campaignMusicTracksGuard, campaignMusicTracks)
    VA_COMPGEN(0x0045e3b0, 0x16, STATIC_DTOR, campaignMusicTracks)
    DATA(0x00694e20)
    static TAutoArrayPtr<char> campaignMusicTracks(new char[strSize]);
    if (!campaignMusicTracks.get())
        return 0;

    char* destination = campaignMusicTracks.get();
    for (cue = 0; cue < CAMPAIGN_MUSIC_CUE_COUNT; ++cue) {
        const char* source = pTextResource->GetText(cue);
        unsigned length = strlen(source) + 1;
        memcpy(destination, source, length);
        gCampaignMusicCues[cue].track = destination;
        destination += length;
    }
    return 1;
}
