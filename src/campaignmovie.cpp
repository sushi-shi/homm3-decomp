// campaignmovie.cpp - the campaign movie table: one row per prologue and
// epilogue movie, its preview image and the display name CmpMovie.txt
// supplies.

// The game links this object between campaignmap.obj and campaignmusic.obj:
// its header initializer is the lone guard row at 0x45e210 (.CRT$XCU 149) and
// its table and pooled image names fill .data 0x66b7d0..0x66c090. Nothing in
// the game reads the table, and /OPT:REF strips the loader and its literal.
// The campaign editor links the same object (h3ccmped 0x41b690..0x41b850,
// table 0x4b16b8) and fills the scenario pages' movie lists from it. The
// compiland name is inferred from the loaded file and the alphabetical
// bracket in both images.
#include "va.h"

#include <string.h>

#include "campaignmovie.h"

#include "ownership.h"
#include "resourcemanager.h"
#include "textresource.h"

DATA(0x0066b7d0) TCampaignMovieTraits g_campaignMovieTraitsImp[CAMPAIGN_MOVIE_COUNT] = {
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

DATA(0x0066baf8)
const TCampaignMovieTraits (&g_campaignMovieTraits)[CAMPAIGN_MOVIE_COUNT] =
    g_campaignMovieTraitsImp;

unsigned char initializeCampaignMovieTraitsTable()
{
    TResourcePtr<TTextResource> textResource(
        ResourceManager::GetText("CmpMovie.txt"));
    if (!textResource.get())
        return 0;

    unsigned strSize = 0;
    unsigned movie;
    for (movie = 0; movie < CAMPAIGN_MOVIE_COUNT; ++movie)
        strSize += strlen(textResource->GetText(movie)) + 1;

    static TAutoArrayPtr<char> campaignMovieNames(new char[strSize]);
    if (!campaignMovieNames.get())
        return 0;

    char* destination = campaignMovieNames.get();
    for (movie = 0; movie < CAMPAIGN_MOVIE_COUNT; ++movie) {
        const char* source = textResource->GetText(movie);
        unsigned length = strlen(source) + 1;
        memcpy(destination, source, length);
        g_campaignMovieTraitsImp[movie].m_name = destination;
        destination += length;
    }
    return 1;
}
