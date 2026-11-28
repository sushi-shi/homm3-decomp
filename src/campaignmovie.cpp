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
    { "CslReG1a.pcx", NULL },
    { "CslReG1b.pcx", NULL },
    { "CslReG1c.pcx", NULL },
    { "CslReE3a.pcx", NULL },
    { "CslReE3b.pcx", NULL },
    { "CslReE3c.pcx", NULL },
    { "CslReN2a.pcx", NULL },
    { "CslReN2b.pcx", NULL },
    { "CslReG1d.pcx", NULL },
    { "CslReG4a.pcx", NULL },
    { "CslReG4b.pcx", NULL },
    { "CslReG4c.pcx", NULL },
    { "CslReG4d.pcx", NULL },
    { "CslReE5a.pcx", NULL },
    { "CslReE5e.pcx", NULL },
    { "CslReE5b.pcx", NULL },
    { "CslReE5c.pcx", NULL },
    { "CslReE5d.pcx", NULL },
    { "CslReG6a.pcx", NULL },
    { "CslReG6b.pcx", NULL },
    { "CslReG6c.pcx", NULL },
    { "CslReS7a.pcx", NULL },
    { "CslReS7b.pcx", NULL },
    { "CslReS7c.pcx", NULL },
    { "CslAbAb1.pcx", NULL },
    { "CslAbAb2.pcx", NULL },
    { "CslAbAb3.pcx", NULL },
    { "CslAbAb4.pcx", NULL },
    { "CslAbAb5.pcx", NULL },
    { "CslAbAb6.pcx", NULL },
    { "CslAbAb7.pcx", NULL },
    { "CslAbAb8.pcx", NULL },
    { "CslAbAb9.pcx", NULL },
    { "CslAbDb1.pcx", NULL },
    { "CslAbdb2.pcx", NULL },
    { "CslAbDb3.pcx", NULL },
    { "CslAbDb4.pcx", NULL },
    { "CslAbDb5.pcx", NULL },
    { "CslAbds1.pcx", NULL },
    { "CslAbDs2.pcx", NULL },
    { "CslAbDs3.pcx", NULL },
    { "CslAbDs4.pcx", NULL },
    { "CslAbDs5.pcx", NULL },
    { "CslAbFl1.pcx", NULL },
    { "CslAbFl2.pcx", NULL },
    { "CslAbfl3.pcx", NULL },
    { "CslAbFl4.pcx", NULL },
    { "CslAbFl5.pcx", NULL },
    { "CslAbfw1.pcx", NULL },
    { "CslAbFw2.pcx", NULL },
    { "CslAbFw3.pcx", NULL },
    { "CslAbFw4.pcx", NULL },
    { "CslAbFw5.pcx", NULL },
    { "CslAbPf1.pcx", NULL },
    { "CslAbpf2.pcx", NULL },
    { "CslAbPf3.pcx", NULL },
    { "CslAbPf4.pcx", NULL },
    { "Csl2BBa.pcx", NULL },
    { "Csl2BBb.pcx", NULL },
    { "Csl2BBc.pcx", NULL },
    { "Csl2BBd.pcx", NULL },
    { "Csl2BBe.pcx", NULL },
    { "Csl2BBf.pcx", NULL },
    { "Csl2ELa.pcx", NULL },
    { "Csl2ELb.pcx", NULL },
    { "Csl2ELc.pcx", NULL },
    { "Csl2ELd.pcx", NULL },
    { "Csl2ELe.pcx", NULL },
    { "Csl2HSa.pcx", NULL },
    { "CslReE5c.pcx", NULL },
    { "Csl2HSc.pcx", NULL },
    { "Csl2HSd.pcx", NULL },
    { "Csl2HSe.pcx", NULL },
    { "Csl2NBa.pcx", NULL },
    { "Csl2NBb.pcx", NULL },
    { "Csl2NBc.pcx", NULL },
    { "Csl2NBd.pcx", NULL },
    { "Csl2NBe.pcx", NULL },
    { "Csl2RNa.pcx", NULL },
    { "Csl2RNb.pcx", NULL },
    { "Csl2RNc.pcx", NULL },
    { "Csl2RNd.pcx", NULL },
    { "Csl2RNe.pcx", NULL },
    { "Csl2SPa.pcx", NULL },
    { "Csl2SPb.pcx", NULL },
    { "Csl2SPc.pcx", NULL },
    { "Csl2SPd.pcx", NULL },
    { "Csl2SPe.pcx", NULL },
    { "Csl2UAa.pcx", NULL },
    { "Csl2UAb.pcx", NULL },
    { "Csl2UAc.pcx", NULL },
    { "Csl2UAd.pcx", NULL },
    { "Csl2UAe.pcx", NULL },
    { "Csl2UAf.pcx", NULL },
    { "Csl2UAg.pcx", NULL },
    { "Csl2UAh.pcx", NULL },
    { "Csl2UAi.pcx", NULL },
    { "Csl2UAj.pcx", NULL },
    { "Csl2UAk.pcx", NULL },
    { "Csl2UAl.pcx", NULL },
    { "Csl2UAm.pcx", NULL }
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

    VA_COMPGEN_AT(h3ccmped, 0x0041b830, 0x16, STATIC_DTOR, campaignMovieNames)
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
