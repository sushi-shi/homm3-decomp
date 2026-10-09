// The campaign movie table (campaignmovie.cpp). Names INVENTED: no native
// symbol covers the table, its row type or its loader. The game declares the
// table beside the music cues and never reads it; only the campaign editor
// calls the loader.
#ifndef HOMM3_CAMPAIGNMOVIE_H
#define HOMM3_CAMPAIGNMOVIE_H

#include "va.h"

// One movie: its preview image (a static PCX name) and the display name the
// loader copies from CmpMovie.txt. The campaign editor's prologue page lists
// m_name and shows m_imageName for the selection.
struct TCampaignMovieTraits {
    const char* m_imageName;
    char* m_name;
};
SIZE(TCampaignMovieTraits, 8);

// The loader's bound: 0x194 bytes of text-line pointers, 101 rows.
enum ECampaignMovieConstants {
    CAMPAIGN_MOVIE_COUNT = 101
};

extern const TCampaignMovieTraits (&g_campaignMovieTraits)[CAMPAIGN_MOVIE_COUNT];
extern TCampaignMovieTraits g_campaignMovieTraitsImp[CAMPAIGN_MOVIE_COUNT];

#endif  /* HOMM3_CAMPAIGNMOVIE_H */
