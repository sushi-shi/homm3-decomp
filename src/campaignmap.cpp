// retains the table initializer and its static-dtor wrapper, both exact. Nine
// Dreamcast AutoArrayPtr / ResourcePtr roster entries are header emissions and
// have no distinct retail bodies in this compiland.
#include "va.h"

#include <string.h>

#include "campaignmap.h"

#include "resourcemanager.h"
#include "textresource.h"

// Region offsets and the eight player-color images for each state are
// the pinned retail initializers. The 0x6c-byte rows, aligned to eight bytes, run
// from 0x660eb8 to the campaign table at 0x663538. Names use the asset codes.
DATA(0x00660eb8)
// Dreamcast original: aGood1RegionTraits.
static TCampaignMapTraits::TRegionTraits g_good1RegionTraits[3] = {
    { 0, 58, 314,
      { "G1A_enR.pcx", "G1A_enB.pcx", "G1A_enN.pcx", "G1A_enG.pcx", "G1A_enO.pcx", "G1A_enV.pcx", "G1A_enT.pcx", "G1A_enP.pcx" },
      { "G1A_seR.pcx", "G1A_seB.pcx", "G1A_seN.pcx", "G1A_seG.pcx", "G1A_seO.pcx", "G1A_seV.pcx", "G1A_seT.pcx", "G1A_seP.pcx" },
      { "G1A_coR.pcx", "G1A_coB.pcx", "G1A_coN.pcx", "G1A_coG.pcx", "G1A_coO.pcx", "G1A_coV.pcx", "G1A_coT.pcx", "G1A_coP.pcx" }
    },
    { 0, 138, 310,
      { "G1B_enR.pcx", "G1B_enB.pcx", "G1B_enN.pcx", "G1B_enG.pcx", "G1B_enO.pcx", "G1B_enV.pcx", "G1B_enT.pcx", "G1B_enP.pcx" },
      { "G1B_seR.pcx", "G1B_seB.pcx", "G1B_seN.pcx", "G1B_seG.pcx", "G1B_seO.pcx", "G1B_seV.pcx", "G1B_seT.pcx", "G1B_seP.pcx" },
      { "G1B_coR.pcx", "G1B_coB.pcx", "G1B_coN.pcx", "G1B_coG.pcx", "G1B_coO.pcx", "G1B_coV.pcx", "G1B_coT.pcx", "G1B_coP.pcx" }
    },
    { 0, 45, 163,
      { "G1C_enR.pcx", "G1C_enB.pcx", "G1C_enN.pcx", "G1C_enG.pcx", "G1C_enO.pcx", "G1C_enV.pcx", "G1C_enT.pcx", "G1C_enP.pcx" },
      { "G1C_seR.pcx", "G1C_seB.pcx", "G1C_seN.pcx", "G1C_seG.pcx", "G1C_seO.pcx", "G1C_seV.pcx", "G1C_seT.pcx", "G1C_seP.pcx" },
      { "G1C_coR.pcx", "G1C_coB.pcx", "G1C_coN.pcx", "G1C_coG.pcx", "G1C_coO.pcx", "G1C_coV.pcx", "G1C_coT.pcx", "G1C_coP.pcx" }
    }
};

DATA(0x00661000)
// Dreamcast original: aGood2RegionTraits.
static TCampaignMapTraits::TRegionTraits g_good2RegionTraits[4] = {
    { 0, 57, 90,
      { "G2A_enR.pcx", "G2A_enB.pcx", "G2A_enN.pcx", "G2A_enG.pcx", "G2A_enO.pcx", "G2A_enV.pcx", "G2A_enT.pcx", "G2A_enP.pcx" },
      { "G2A_seR.pcx", "G2A_seB.pcx", "G2A_seN.pcx", "G2A_seG.pcx", "G2A_seO.pcx", "G2A_seV.pcx", "G2A_seT.pcx", "G2A_seP.pcx" },
      { "G2A_coR.pcx", "G2A_coB.pcx", "G2A_coN.pcx", "G2A_coG.pcx", "G2A_coO.pcx", "G2A_coV.pcx", "G2A_coT.pcx", "G2A_coP.pcx" }
    },
    { 0, 317, 49,
      { "G2B_enR.pcx", "G2B_enB.pcx", "G2B_enN.pcx", "G2B_enG.pcx", "G2B_enO.pcx", "G2B_enV.pcx", "G2B_enT.pcx", "G2B_enP.pcx" },
      { "G2B_seR.pcx", "G2B_seB.pcx", "G2B_seN.pcx", "G2B_seG.pcx", "G2B_seO.pcx", "G2B_seV.pcx", "G2B_seT.pcx", "G2B_seP.pcx" },
      { "G2B_coR.pcx", "G2B_coB.pcx", "G2B_coN.pcx", "G2B_coG.pcx", "G2B_coO.pcx", "G2B_coV.pcx", "G2B_coT.pcx", "G2B_coP.pcx" }
    },
    { 0, 55, 378,
      { "G2C_enR.pcx", "G2C_enB.pcx", "G2C_enN.pcx", "G2C_enG.pcx", "G2C_enO.pcx", "G2C_enV.pcx", "G2C_enT.pcx", "G2C_enP.pcx" },
      { "G2C_seR.pcx", "G2C_seB.pcx", "G2C_seN.pcx", "G2C_seG.pcx", "G2C_seO.pcx", "G2C_seV.pcx", "G2C_seT.pcx", "G2C_seP.pcx" },
      { "G2C_coR.pcx", "G2C_coB.pcx", "G2C_coN.pcx", "G2C_coG.pcx", "G2C_coO.pcx", "G2C_coV.pcx", "G2C_coT.pcx", "G2C_coP.pcx" }
    },
    { 0, 152, 126,
      { "G2D_enR.pcx", "G2D_enB.pcx", "G2D_enN.pcx", "G2D_enG.pcx", "G2D_enO.pcx", "G2D_enV.pcx", "G2D_enT.pcx", "G2D_enP.pcx" },
      { "G2D_seR.pcx", "G2D_seB.pcx", "G2D_seN.pcx", "G2D_seG.pcx", "G2D_seO.pcx", "G2D_seV.pcx", "G2D_seT.pcx", "G2D_seP.pcx" },
      { "G2D_coR.pcx", "G2D_coB.pcx", "G2D_coN.pcx", "G2D_coG.pcx", "G2D_coO.pcx", "G2D_coV.pcx", "G2D_coT.pcx", "G2D_coP.pcx" }
    }
};

DATA(0x006611b0)
// Dreamcast original: aGood3RegionTraits.
static TCampaignMapTraits::TRegionTraits g_good3RegionTraits[3] = {
    { 0, 290, 376,
      { "G3A_enR.pcx", "G3A_enB.pcx", "G3A_enN.pcx", "G3A_enG.pcx", "G3A_enO.pcx", "G3A_enV.pcx", "G3A_enT.pcx", "G3A_enP.pcx" },
      { "G3A_seR.pcx", "G3A_seB.pcx", "G3A_seN.pcx", "G3A_seG.pcx", "G3A_seO.pcx", "G3A_seV.pcx", "G3A_seT.pcx", "G3A_seP.pcx" },
      { "G3A_coR.pcx", "G3A_coB.pcx", "G3A_coN.pcx", "G3A_coG.pcx", "G3A_coO.pcx", "G3A_coV.pcx", "G3A_coT.pcx", "G3A_coP.pcx" }
    },
    { 0, 61, 147,
      { "G3B_enR.pcx", "G3B_enB.pcx", "G3B_enN.pcx", "G3B_enG.pcx", "G3B_enO.pcx", "G3B_enV.pcx", "G3B_enT.pcx", "G3B_enP.pcx" },
      { "G3B_seR.pcx", "G3B_seB.pcx", "G3B_seN.pcx", "G3B_seG.pcx", "G3B_seO.pcx", "G3B_seV.pcx", "G3B_seT.pcx", "G3B_seP.pcx" },
      { "G3B_coR.pcx", "G3B_coB.pcx", "G3B_coN.pcx", "G3B_coG.pcx", "G3B_coO.pcx", "G3B_coV.pcx", "G3B_coT.pcx", "G3B_coP.pcx" }
    },
    { 0, 132, 202,
      { "G3C_enR.pcx", "G3C_enB.pcx", "G3C_enN.pcx", "G3C_enG.pcx", "G3C_enO.pcx", "G3C_enV.pcx", "G3C_enT.pcx", "G3C_enP.pcx" },
      { "G3C_seR.pcx", "G3C_seB.pcx", "G3C_seN.pcx", "G3C_seG.pcx", "G3C_seO.pcx", "G3C_seV.pcx", "G3C_seT.pcx", "G3C_seP.pcx" },
      { "G3C_coR.pcx", "G3C_coB.pcx", "G3C_coN.pcx", "G3C_coG.pcx", "G3C_coO.pcx", "G3C_coV.pcx", "G3C_coT.pcx", "G3C_coP.pcx" }
    }
};

DATA(0x006612f8)
// Dreamcast original: aEvil1RegionTraits.
static TCampaignMapTraits::TRegionTraits g_evil1RegionTraits[7] = {
    { 0, 271, 332,
      { "E1A_enR.pcx", "E1A_enB.pcx", "E1A_enN.pcx", "E1A_enG.pcx", "E1A_enO.pcx", "E1A_enV.pcx", "E1A_enT.pcx", "E1A_enP.pcx" },
      { "E1A_seR.pcx", "E1A_seB.pcx", "E1A_seN.pcx", "E1A_seG.pcx", "E1A_seO.pcx", "E1A_seV.pcx", "E1A_seT.pcx", "E1A_seP.pcx" },
      { "E1A_coR.pcx", "E1A_coB.pcx", "E1A_coN.pcx", "E1A_coG.pcx", "E1A_coO.pcx", "E1A_coV.pcx", "E1A_coT.pcx", "E1A_coP.pcx" }
    },
    { 0, 139, 113,
      { "E1B_enR.pcx", "E1B_enB.pcx", "E1B_enN.pcx", "E1B_enG.pcx", "E1B_enO.pcx", "E1B_enV.pcx", "E1B_enT.pcx", "E1B_enP.pcx" },
      { "E1B_seR.pcx", "E1B_seB.pcx", "E1B_seN.pcx", "E1B_seG.pcx", "E1B_seO.pcx", "E1B_seV.pcx", "E1B_seT.pcx", "E1B_seP.pcx" },
      { "E1B_coR.pcx", "E1B_coB.pcx", "E1B_coN.pcx", "E1B_coG.pcx", "E1B_coO.pcx", "E1B_coV.pcx", "E1B_coT.pcx", "E1B_coP.pcx" }
    },
    { 0, 27, 70,
      { "E1C_enR.pcx", "E1C_enB.pcx", "E1C_enN.pcx", "E1C_enG.pcx", "E1C_enO.pcx", "E1C_enV.pcx", "E1C_enT.pcx", "E1C_enP.pcx" },
      { "E1C_seR.pcx", "E1C_seB.pcx", "E1C_seN.pcx", "E1C_seG.pcx", "E1C_seO.pcx", "E1C_seV.pcx", "E1C_seT.pcx", "E1C_seP.pcx" },
      { "E1C_coR.pcx", "E1C_coB.pcx", "E1C_coN.pcx", "E1C_coG.pcx", "E1C_coO.pcx", "E1C_coV.pcx", "E1C_coT.pcx", "E1C_coP.pcx" }
    },
    { 0, 257, 127,
      { "E1P1_enR.pcx", "E1P1_enB.pcx", "E1P1_enN.pcx", "E1P1_enG.pcx", "E1P1_enO.pcx", "E1P1_enV.pcx", "E1P1_enT.pcx", "E1P1_enP.pcx" },
      { "E1P1_seR.pcx", "E1P1_seB.pcx", "E1P1_seN.pcx", "E1P1_seG.pcx", "E1P1_seO.pcx", "E1P1_seV.pcx", "E1P1_seT.pcx", "E1P1_seP.pcx" },
      { "E1P1_coR.pcx", "E1P1_coB.pcx", "E1P1_coN.pcx", "E1P1_coG.pcx", "E1P1_coO.pcx", "E1P1_coV.pcx", "E1P1_coT.pcx", "E1P1_coP.pcx" }
    },
    { 0, 58, 314,
      { "E1P2_enR.pcx", "E1P2_enB.pcx", "E1P2_enN.pcx", "E1P2_enG.pcx", "E1P2_enO.pcx", "E1P2_enV.pcx", "E1P2_enT.pcx", "E1P2_enP.pcx" },
      { "E1P2_seR.pcx", "E1P2_seB.pcx", "E1P2_seN.pcx", "E1P2_seG.pcx", "E1P2_seO.pcx", "E1P2_seV.pcx", "E1P2_seT.pcx", "E1P2_seP.pcx" },
      { "E1P2_coR.pcx", "E1P2_coB.pcx", "E1P2_coN.pcx", "E1P2_coG.pcx", "E1P2_coO.pcx", "E1P2_coV.pcx", "E1P2_coT.pcx", "E1P2_coP.pcx" }
    },
    { 0, 138, 310,
      { "E1P3_enR.pcx", "E1P3_enB.pcx", "E1P3_enN.pcx", "E1P3_enG.pcx", "E1P3_enO.pcx", "E1P3_enV.pcx", "E1P3_enT.pcx", "E1P3_enP.pcx" },
      { "E1P3_seR.pcx", "E1P3_seB.pcx", "E1P3_seN.pcx", "E1P3_seG.pcx", "E1P3_seO.pcx", "E1P3_seV.pcx", "E1P3_seT.pcx", "E1P3_seP.pcx" },
      { "E1P3_coR.pcx", "E1P3_coB.pcx", "E1P3_coN.pcx", "E1P3_coG.pcx", "E1P3_coO.pcx", "E1P3_coV.pcx", "E1P3_coT.pcx", "E1P3_coP.pcx" }
    },
    { 0, 45, 163,
      { "E1P4_enR.pcx", "E1P4_enB.pcx", "E1P4_enN.pcx", "E1P4_enG.pcx", "E1P4_enO.pcx", "E1P4_enV.pcx", "E1P4_enT.pcx", "E1P4_enP.pcx" },
      { "E1P4_seR.pcx", "E1P4_seB.pcx", "E1P4_seN.pcx", "E1P4_seG.pcx", "E1P4_seO.pcx", "E1P4_seV.pcx", "E1P4_seT.pcx", "E1P4_seP.pcx" },
      { "E1P4_coR.pcx", "E1P4_coB.pcx", "E1P4_coN.pcx", "E1P4_coG.pcx", "E1P4_coO.pcx", "E1P4_coV.pcx", "E1P4_coT.pcx", "E1P4_coP.pcx" }
    }
};

DATA(0x006615f0)
// Dreamcast original: aEvil2RegionTraits.
static TCampaignMapTraits::TRegionTraits g_evil2RegionTraits[4] = {
    { 0, 132, 202,
      { "E2A_enR.pcx", "E2A_enB.pcx", "E2A_enN.pcx", "E2A_enG.pcx", "E2A_enO.pcx", "E2A_enV.pcx", "E2A_enT.pcx", "E2A_enP.pcx" },
      { "E2A_seR.pcx", "E2A_seB.pcx", "E2A_seN.pcx", "E2A_seG.pcx", "E2A_seO.pcx", "E2A_seV.pcx", "E2A_seT.pcx", "E2A_seP.pcx" },
      { "E2A_coR.pcx", "E2A_coB.pcx", "E2A_coN.pcx", "E2A_coG.pcx", "E2A_coO.pcx", "E2A_coV.pcx", "E2A_coT.pcx", "E2A_coP.pcx" }
    },
    { 0, 61, 145,
      { "E2B_enR.pcx", "E2B_enB.pcx", "E2B_enN.pcx", "E2B_enG.pcx", "E2B_enO.pcx", "E2B_enV.pcx", "E2B_enT.pcx", "E2B_enP.pcx" },
      { "E2B_seR.pcx", "E2B_seB.pcx", "E2B_seN.pcx", "E2B_seG.pcx", "E2B_seO.pcx", "E2B_seV.pcx", "E2B_seT.pcx", "E2B_seP.pcx" },
      { "E2B_coR.pcx", "E2B_coB.pcx", "E2B_coN.pcx", "E2B_coG.pcx", "E2B_coO.pcx", "E2B_coV.pcx", "E2B_coT.pcx", "E2B_coP.pcx" }
    },
    { 0, 219, 307,
      { "E2D_enR.pcx", "E2D_enB.pcx", "E2D_enN.pcx", "E2D_enG.pcx", "E2D_enO.pcx", "E2D_enV.pcx", "E2D_enT.pcx", "E2D_enP.pcx" },
      { "E2D_seR.pcx", "E2D_seB.pcx", "E2D_seN.pcx", "E2D_seG.pcx", "E2D_seO.pcx", "E2D_seV.pcx", "E2D_seT.pcx", "E2D_seP.pcx" },
      { "E2D_coR.pcx", "E2D_coB.pcx", "E2D_coN.pcx", "E2D_coG.pcx", "E2D_coO.pcx", "E2D_coV.pcx", "E2D_coT.pcx", "E2D_coP.pcx" }
    },
    { 0, 93, 261,
      { "E2C_enR.pcx", "E2C_enB.pcx", "E2C_enN.pcx", "E2C_enG.pcx", "E2C_enO.pcx", "E2C_enV.pcx", "E2C_enT.pcx", "E2C_enP.pcx" },
      { "E2C_seR.pcx", "E2C_seB.pcx", "E2C_seN.pcx", "E2C_seG.pcx", "E2C_seO.pcx", "E2C_seV.pcx", "E2C_seT.pcx", "E2C_seP.pcx" },
      { "E2C_coR.pcx", "E2C_coB.pcx", "E2C_coN.pcx", "E2C_coG.pcx", "E2C_coO.pcx", "E2C_coV.pcx", "E2C_coT.pcx", "E2C_coP.pcx" }
    }
};

DATA(0x006617a0)
// Dreamcast original: aNeutral1RegionTraits.
static TCampaignMapTraits::TRegionTraits g_neutral1RegionTraits[3] = {
    { 0, 43, 94,
      { "N1A_enR.pcx", "N1A_enB.pcx", "N1A_enN.pcx", "N1A_enG.pcx", "N1A_enO.pcx", "N1A_enV.pcx", "N1A_enT.pcx", "N1A_enP.pcx" },
      { "N1A_seR.pcx", "N1A_seB.pcx", "N1A_seN.pcx", "N1A_seG.pcx", "N1A_seO.pcx", "N1A_seV.pcx", "N1A_seT.pcx", "N1A_seP.pcx" },
      { "N1A_coR.pcx", "N1A_coB.pcx", "N1A_coN.pcx", "N1A_coG.pcx", "N1A_coO.pcx", "N1A_coV.pcx", "N1A_coT.pcx", "N1A_coP.pcx" }
    },
    { 0, 310, 290,
      { "N1B_enR.pcx", "N1B_enB.pcx", "N1B_enN.pcx", "N1B_enG.pcx", "N1B_enO.pcx", "N1B_enV.pcx", "N1B_enT.pcx", "N1B_enP.pcx" },
      { "N1B_seR.pcx", "N1B_seB.pcx", "N1B_seN.pcx", "N1B_seG.pcx", "N1B_seO.pcx", "N1B_seV.pcx", "N1B_seT.pcx", "N1B_seP.pcx" },
      { "N1B_coR.pcx", "N1B_coB.pcx", "N1B_coN.pcx", "N1B_coG.pcx", "N1B_coO.pcx", "N1B_coV.pcx", "N1B_coT.pcx", "N1B_coP.pcx" }
    },
    { 0, 189, 202,
      { "N1CD_enR.pcx", "N1CD_enB.pcx", "N1CD_enN.pcx", "N1CD_enG.pcx", "N1CD_enO.pcx", "N1CD_enV.pcx", "N1CD_enT.pcx", "N1CD_enP.pcx" },
      { "N1CD_seR.pcx", "N1CD_seB.pcx", "N1CD_seN.pcx", "N1CD_seG.pcx", "N1CD_seO.pcx", "N1CD_seV.pcx", "N1CD_seT.pcx", "N1CD_seP.pcx" },
      { "N1CD_coR.pcx", "N1CD_coB.pcx", "N1CD_coN.pcx", "N1CD_coG.pcx", "N1CD_coO.pcx", "N1CD_coV.pcx", "N1CD_coT.pcx", "N1CD_coP.pcx" }
    }
};

DATA(0x006618e8)
// Dreamcast original: aSecret1RegionTraits.
static TCampaignMapTraits::TRegionTraits g_secret1RegionTraits[3] = {
    { 0, 264, 199,
      { "S1A_enR.pcx", "S1A_enB.pcx", "S1A_enN.pcx", "S1A_enG.pcx", "S1A_enO.pcx", "S1A_enV.pcx", "S1A_enT.pcx", "S1A_enP.pcx" },
      { "S1A_seR.pcx", "S1A_seB.pcx", "S1A_seN.pcx", "S1A_seG.pcx", "S1A_seO.pcx", "S1A_seV.pcx", "S1A_seT.pcx", "S1A_seP.pcx" },
      { "S1A_coR.pcx", "S1A_coB.pcx", "S1A_coN.pcx", "S1A_coG.pcx", "S1A_coO.pcx", "S1A_coV.pcx", "S1A_coT.pcx", "S1A_coP.pcx" }
    },
    { 0, 83, 152,
      { "S1C_enR.pcx", "S1C_enB.pcx", "S1C_enN.pcx", "S1C_enG.pcx", "S1C_enO.pcx", "S1C_enV.pcx", "S1C_enT.pcx", "S1C_enP.pcx" },
      { "S1C_seR.pcx", "S1C_seB.pcx", "S1C_seN.pcx", "S1C_seG.pcx", "S1C_seO.pcx", "S1C_seV.pcx", "S1C_seT.pcx", "S1C_seP.pcx" },
      { "S1C_coR.pcx", "S1C_coB.pcx", "S1C_coN.pcx", "S1C_coG.pcx", "S1C_coO.pcx", "S1C_coV.pcx", "S1C_coT.pcx", "S1C_coP.pcx" }
    },
    { 0, 183, 210,
      { "S1B_enR.pcx", "S1B_enB.pcx", "S1B_enN.pcx", "S1B_enG.pcx", "S1B_enO.pcx", "S1B_enV.pcx", "S1B_enT.pcx", "S1B_enP.pcx" },
      { "S1B_seR.pcx", "S1B_seB.pcx", "S1B_seN.pcx", "S1B_seG.pcx", "S1B_seO.pcx", "S1B_seV.pcx", "S1B_seT.pcx", "S1B_seP.pcx" },
      { "S1B_coR.pcx", "S1B_coB.pcx", "S1B_coN.pcx", "S1B_coG.pcx", "S1B_coO.pcx", "S1B_coV.pcx", "S1B_coT.pcx", "S1B_coP.pcx" }
    }
};

DATA(0x00661a30)
static TCampaignMapTraits::TRegionTraits g_brRegionTraits[4] = {
    { 0, 19, 233,
      { "BrA_enRe.pcx", "BrA_enBl.pcx", "BrA_enBr.pcx", "BrA_enGr.pcx", "BrA_enOr.pcx", "BrA_enVi.pcx", "BrA_enTe.pcx", "BrA_enPi.pcx" },
      { "BrA_seRe.pcx", "BrA_seBl.pcx", "BrA_seBr.pcx", "BrA_seGr.pcx", "BrA_seOr.pcx", "BrA_seVi.pcx", "BrA_seTe.pcx", "BrA_sePi.pcx" },
      { "BrA_coRe.pcx", "BrA_coBl.pcx", "BrA_coBr.pcx", "BrA_coGr.pcx", "BrA_coOr.pcx", "BrA_coVi.pcx", "BrA_coTe.pcx", "BrA_coPi.pcx" }
    },
    { 0, 126, 381,
      { "BrB_enRe.pcx", "BrB_enBl.pcx", "BrB_enBr.pcx", "BrB_enGr.pcx", "BrB_enOr.pcx", "BrB_enVi.pcx", "BrB_enTe.pcx", "BrB_enPi.pcx" },
      { "BrB_seRe.pcx", "BrB_seBl.pcx", "BrB_seBr.pcx", "BrB_seGr.pcx", "BrB_seOr.pcx", "BrB_seVi.pcx", "BrB_seTe.pcx", "BrB_sePi.pcx" },
      { "BrB_coRe.pcx", "BrB_coBl.pcx", "BrB_coBr.pcx", "BrB_coGr.pcx", "BrB_coOr.pcx", "BrB_coVi.pcx", "BrB_coTe.pcx", "BrB_coPi.pcx" }
    },
    { 0, 225, 357,
      { "BrC_enRe.pcx", "BrC_enBl.pcx", "BrC_enBr.pcx", "BrC_enGr.pcx", "BrC_enOr.pcx", "BrC_enVi.pcx", "BrC_enTe.pcx", "BrC_enPi.pcx" },
      { "BrC_seRe.pcx", "BrC_seBl.pcx", "BrC_seBr.pcx", "BrC_seGr.pcx", "BrC_seOr.pcx", "BrC_seVi.pcx", "BrC_seTe.pcx", "BrC_sePi.pcx" },
      { "BrC_coRe.pcx", "BrC_coBl.pcx", "BrC_coBr.pcx", "BrC_coGr.pcx", "BrC_coOr.pcx", "BrC_coVi.pcx", "BrC_coTe.pcx", "BrC_coPi.pcx" }
    },
    { 0, 193, 320,
      { "BrD_enRe.pcx", "BrD_enBl.pcx", "BrD_enBr.pcx", "BrD_enGr.pcx", "BrD_enOr.pcx", "BrD_enVi.pcx", "BrD_enTe.pcx", "BrD_enPi.pcx" },
      { "BrD_seRe.pcx", "BrD_seBl.pcx", "BrD_seBr.pcx", "BrD_seGr.pcx", "BrD_seOr.pcx", "BrD_seVi.pcx", "BrD_seTe.pcx", "BrD_sePi.pcx" },
      { "BrD_coRe.pcx", "BrD_coBl.pcx", "BrD_coBr.pcx", "BrD_coGr.pcx", "BrD_coOr.pcx", "BrD_coVi.pcx", "BrD_coTe.pcx", "BrD_coPi.pcx" }
    }
};

DATA(0x00661be0)
static TCampaignMapTraits::TRegionTraits g_isRegionTraits[4] = {
    { 0, 295, 399,
      { "IsA_enRe.pcx", "IsA_enBl.pcx", "IsA_enBr.pcx", "IsA_enGr.pcx", "IsA_enOr.pcx", "IsA_enVi.pcx", "IsA_enTe.pcx", "IsA_enPi.pcx" },
      { "IsA_seRe.pcx", "IsA_seBl.pcx", "IsA_seBr.pcx", "IsA_seGr.pcx", "IsA_seOr.pcx", "IsA_seVi.pcx", "IsA_seTe.pcx", "IsA_sePi.pcx" },
      { "IsA_coRe.pcx", "IsA_coBl.pcx", "IsA_coBr.pcx", "IsA_coGr.pcx", "IsA_coOr.pcx", "IsA_coVi.pcx", "IsA_coTe.pcx", "IsA_coPi.pcx" }
    },
    { 0, 184, 293,
      { "IsB_enRe.pcx", "IsB_enBl.pcx", "IsB_enBr.pcx", "IsB_enGr.pcx", "IsB_enOr.pcx", "IsB_enVi.pcx", "IsB_enTe.pcx", "IsB_enPi.pcx" },
      { "IsB_seRe.pcx", "IsB_seBl.pcx", "IsB_seBr.pcx", "IsB_seGr.pcx", "IsB_seOr.pcx", "IsB_seVi.pcx", "IsB_seTe.pcx", "IsB_sePi.pcx" },
      { "IsB_coRe.pcx", "IsB_coBl.pcx", "IsB_coBr.pcx", "IsB_coGr.pcx", "IsB_coOr.pcx", "IsB_coVi.pcx", "IsB_coTe.pcx", "IsB_coPi.pcx" }
    },
    { 0, 41, 92,
      { "IsC_enRe.pcx", "IsC_enBl.pcx", "IsC_enBr.pcx", "IsC_enGr.pcx", "IsC_enOr.pcx", "IsC_enVi.pcx", "IsC_enTe.pcx", "IsC_enPi.pcx" },
      { "IsC_seRe.pcx", "IsC_seBl.pcx", "IsC_seBr.pcx", "IsC_seGr.pcx", "IsC_seOr.pcx", "IsC_seVi.pcx", "IsC_seTe.pcx", "IsC_sePi.pcx" },
      { "IsC_coRe.pcx", "IsC_coBl.pcx", "IsC_coBr.pcx", "IsC_coGr.pcx", "IsC_coOr.pcx", "IsC_coVi.pcx", "IsC_coTe.pcx", "IsC_coPi.pcx" }
    },
    { 0, 295, 399,
      { "IsD_enRe.pcx", "IsD_enBl.pcx", "IsD_enBr.pcx", "IsD_enGr.pcx", "IsD_enOr.pcx", "IsD_enVi.pcx", "IsD_enTe.pcx", "IsD_enPi.pcx" },
      { "IsD_seRe.pcx", "IsD_seBl.pcx", "IsD_seBr.pcx", "IsD_seGr.pcx", "IsD_seOr.pcx", "IsD_seVi.pcx", "IsD_seTe.pcx", "IsD_sePi.pcx" },
      { "IsD_coRe.pcx", "IsD_coBl.pcx", "IsD_coBr.pcx", "IsD_coGr.pcx", "IsD_coOr.pcx", "IsD_coVi.pcx", "IsD_coTe.pcx", "IsD_coPi.pcx" }
    }
};

DATA(0x00661d90)
static TCampaignMapTraits::TRegionTraits g_krRegionTraits[4] = {
    { 0, 149, 323,
      { "KrA_enRe.pcx", "KrA_enBl.pcx", "KrA_enBr.pcx", "KrA_enGr.pcx", "KrA_enOr.pcx", "KrA_enVi.pcx", "KrA_enTe.pcx", "KrA_enPi.pcx" },
      { "KrA_seRe.pcx", "KrA_seBl.pcx", "KrA_seBr.pcx", "KrA_seGr.pcx", "KrA_seOr.pcx", "KrA_seVi.pcx", "KrA_seTe.pcx", "KrA_sePi.pcx" },
      { "KrA_coRe.pcx", "KrA_coBl.pcx", "KrA_coBr.pcx", "KrA_coGr.pcx", "KrA_coOr.pcx", "KrA_coVi.pcx", "KrA_coTe.pcx", "KrA_coPi.pcx" }
    },
    { 0, 193, 235,
      { "KrB_enRe.pcx", "KrB_enBl.pcx", "KrB_enBr.pcx", "KrB_enGr.pcx", "KrB_enOr.pcx", "KrB_enVi.pcx", "KrB_enTe.pcx", "KrB_enPi.pcx" },
      { "KrB_seRe.pcx", "KrB_seBl.pcx", "KrB_seBr.pcx", "KrB_seGr.pcx", "KrB_seOr.pcx", "KrB_seVi.pcx", "KrB_seTe.pcx", "KrB_sePi.pcx" },
      { "KrB_coRe.pcx", "KrB_coBl.pcx", "KrB_coBr.pcx", "KrB_coGr.pcx", "KrB_coOr.pcx", "KrB_coVi.pcx", "KrB_coTe.pcx", "KrB_coPi.pcx" }
    },
    { 0, 137, 158,
      { "KrC_enRe.pcx", "KrC_enBl.pcx", "KrC_enBr.pcx", "KrC_enGr.pcx", "KrC_enOr.pcx", "KrC_enVi.pcx", "KrC_enTe.pcx", "KrC_enPi.pcx" },
      { "KrC_seRe.pcx", "KrC_seBl.pcx", "KrC_seBr.pcx", "KrC_seGr.pcx", "KrC_seOr.pcx", "KrC_seVi.pcx", "KrC_seTe.pcx", "KrC_sePi.pcx" },
      { "KrC_coRe.pcx", "KrC_coBl.pcx", "KrC_coBr.pcx", "KrC_coGr.pcx", "KrC_coOr.pcx", "KrC_coVi.pcx", "KrC_coTe.pcx", "KrC_coPi.pcx" }
    },
    { 0, 88, 107,
      { "KrD_enRe.pcx", "KrD_enBl.pcx", "KrD_enBr.pcx", "KrD_enGr.pcx", "KrD_enOr.pcx", "KrD_enVi.pcx", "KrD_enTe.pcx", "KrD_enPi.pcx" },
      { "KrD_seRe.pcx", "KrD_seBl.pcx", "KrD_seBr.pcx", "KrD_seGr.pcx", "KrD_seOr.pcx", "KrD_seVi.pcx", "KrD_seTe.pcx", "KrD_sePi.pcx" },
      { "KrD_coRe.pcx", "KrD_coBl.pcx", "KrD_coBr.pcx", "KrD_coGr.pcx", "KrD_coOr.pcx", "KrD_coVi.pcx", "KrD_coTe.pcx", "KrD_coPi.pcx" }
    }
};

DATA(0x00661f40)
static TCampaignMapTraits::TRegionTraits g_niRegionTraits[4] = {
    { 0, 119, 111,
      { "NiA_enRe.pcx", "NiA_enBl.pcx", "NiA_enBr.pcx", "NiA_enGr.pcx", "NiA_enOr.pcx", "NiA_enVi.pcx", "NiA_enTe.pcx", "NiA_enPi.pcx" },
      { "NiA_seRe.pcx", "NiA_seBl.pcx", "NiA_seBr.pcx", "NiA_seGr.pcx", "NiA_seOr.pcx", "NiA_seVi.pcx", "NiA_seTe.pcx", "NiA_sePi.pcx" },
      { "NiA_coRe.pcx", "NiA_coBl.pcx", "NiA_coBr.pcx", "NiA_coGr.pcx", "NiA_coOr.pcx", "NiA_coVi.pcx", "NiA_coTe.pcx", "NiA_coPi.pcx" }
    },
    { 0, 224, 145,
      { "NiB_enRe.pcx", "NiB_enBl.pcx", "NiB_enBr.pcx", "NiB_enGr.pcx", "NiB_enOr.pcx", "NiB_enVi.pcx", "NiB_enTe.pcx", "NiB_enPi.pcx" },
      { "NiB_seRe.pcx", "NiB_seBl.pcx", "NiB_seBr.pcx", "NiB_seGr.pcx", "NiB_seOr.pcx", "NiB_seVi.pcx", "NiB_seTe.pcx", "NiB_sePi.pcx" },
      { "NiB_coRe.pcx", "NiB_coBl.pcx", "NiB_coBr.pcx", "NiB_coGr.pcx", "NiB_coOr.pcx", "NiB_coVi.pcx", "NiB_coTe.pcx", "NiB_coPi.pcx" }
    },
    { 0, 321, 213,
      { "NiC_enRe.pcx", "NiC_enBl.pcx", "NiC_enBr.pcx", "NiC_enGr.pcx", "NiC_enOr.pcx", "NiC_enVi.pcx", "NiC_enTe.pcx", "NiC_enPi.pcx" },
      { "NiC_seRe.pcx", "NiC_seBl.pcx", "NiC_seBr.pcx", "NiC_seGr.pcx", "NiC_seOr.pcx", "NiC_seVi.pcx", "NiC_seTe.pcx", "NiC_sePi.pcx" },
      { "NiC_coRe.pcx", "NiC_coBl.pcx", "NiC_coBr.pcx", "NiC_coGr.pcx", "NiC_coOr.pcx", "NiC_coVi.pcx", "NiC_coTe.pcx", "NiC_coPi.pcx" }
    },
    { 0, 234, 250,
      { "NiD_enRe.pcx", "NiD_enBl.pcx", "NiD_enBr.pcx", "NiD_enGr.pcx", "NiD_enOr.pcx", "NiD_enVi.pcx", "NiD_enTe.pcx", "NiD_enPi.pcx" },
      { "NiD_seRe.pcx", "NiD_seBl.pcx", "NiD_seBr.pcx", "NiD_seGr.pcx", "NiD_seOr.pcx", "NiD_seVi.pcx", "NiD_seTe.pcx", "NiD_sePi.pcx" },
      { "NiD_coRe.pcx", "NiD_coBl.pcx", "NiD_coBr.pcx", "NiD_coGr.pcx", "NiD_coOr.pcx", "NiD_coVi.pcx", "NiD_coTe.pcx", "NiD_coPi.pcx" }
    }
};

DATA(0x006620f0)
static TCampaignMapTraits::TRegionTraits g_taRegionTraits[3] = {
    { 0, 229, 233,
      { "TaA_enRe.pcx", "TaA_enBl.pcx", "TaA_enBr.pcx", "TaA_enGr.pcx", "TaA_enOr.pcx", "TaA_enVi.pcx", "TaA_enTe.pcx", "TaA_enPi.pcx" },
      { "TaA_seRe.pcx", "TaA_seBl.pcx", "TaA_seBr.pcx", "TaA_seGr.pcx", "TaA_seOr.pcx", "TaA_seVi.pcx", "TaA_seTe.pcx", "TaA_sePi.pcx" },
      { "TaA_coRe.pcx", "TaA_coBl.pcx", "TaA_coBr.pcx", "TaA_coGr.pcx", "TaA_coOr.pcx", "TaA_coVi.pcx", "TaA_coTe.pcx", "TaA_coPi.pcx" }
    },
    { 0, 148, 194,
      { "TaB_enRe.pcx", "TaB_enBl.pcx", "TaB_enBr.pcx", "TaB_enGr.pcx", "TaB_enOr.pcx", "TaB_enVi.pcx", "TaB_enTe.pcx", "TaB_enPi.pcx" },
      { "TaB_seRe.pcx", "TaB_seBl.pcx", "TaB_seBr.pcx", "TaB_seGr.pcx", "TaB_seOr.pcx", "TaB_seVi.pcx", "TaB_seTe.pcx", "TaB_sePi.pcx" },
      { "TaB_coRe.pcx", "TaB_coBl.pcx", "TaB_coBr.pcx", "TaB_coGr.pcx", "TaB_coOr.pcx", "TaB_coVi.pcx", "TaB_coTe.pcx", "TaB_coPi.pcx" }
    },
    { 0, 113, 97,
      { "TaC_enRe.pcx", "TaC_enBl.pcx", "TaC_enBr.pcx", "TaC_enGr.pcx", "TaC_enOr.pcx", "TaC_enVi.pcx", "TaC_enTe.pcx", "TaC_enPi.pcx" },
      { "TaC_seRe.pcx", "TaC_seBl.pcx", "TaC_seBr.pcx", "TaC_seGr.pcx", "TaC_seOr.pcx", "TaC_seVi.pcx", "TaC_seTe.pcx", "TaC_sePi.pcx" },
      { "TaC_coRe.pcx", "TaC_coBl.pcx", "TaC_coBr.pcx", "TaC_coGr.pcx", "TaC_coOr.pcx", "TaC_coVi.pcx", "TaC_coTe.pcx", "TaC_coPi.pcx" }
    }
};

DATA(0x00662238)
static TCampaignMapTraits::TRegionTraits g_arRegionTraits[8] = {
    { 0, 136, 238,
      { "ArA_enRe.pcx", "ArA_enBl.pcx", "ArA_enBr.pcx", "ArA_enGr.pcx", "ArA_enOr.pcx", "ArA_enVi.pcx", "ArA_enTe.pcx", "ArA_enPi.pcx" },
      { "ArA_seRe.pcx", "ArA_seBl.pcx", "ArA_seBr.pcx", "ArA_seGr.pcx", "ArA_seOr.pcx", "ArA_seVi.pcx", "ArA_seTe.pcx", "ArA_sePi.pcx" },
      { "ArA_coRe.pcx", "ArA_coBl.pcx", "ArA_coBr.pcx", "ArA_coGr.pcx", "ArA_coOr.pcx", "ArA_coVi.pcx", "ArA_coTe.pcx", "ArA_coPi.pcx" }
    },
    { 0, 136, 121,
      { "ArB_enRe.pcx", "ArB_enBl.pcx", "ArB_enBr.pcx", "ArB_enGr.pcx", "ArB_enOr.pcx", "ArB_enVi.pcx", "ArB_enTe.pcx", "ArB_enPi.pcx" },
      { "ArB_seRe.pcx", "ArB_seBl.pcx", "ArB_seBr.pcx", "ArB_seGr.pcx", "ArB_seOr.pcx", "ArB_seVi.pcx", "ArB_seTe.pcx", "ArB_sePi.pcx" },
      { "ArB_coRe.pcx", "ArB_coBl.pcx", "ArB_coBr.pcx", "ArB_coGr.pcx", "ArB_coOr.pcx", "ArB_coVi.pcx", "ArB_coTe.pcx", "ArB_coPi.pcx" }
    },
    { 0, 207, 155,
      { "ArC_enRe.pcx", "ArC_enBl.pcx", "ArC_enBr.pcx", "ArC_enGr.pcx", "ArC_enOr.pcx", "ArC_enVi.pcx", "ArC_enTe.pcx", "ArC_enPi.pcx" },
      { "ArC_seRe.pcx", "ArC_seBl.pcx", "ArC_seBr.pcx", "ArC_seGr.pcx", "ArC_seOr.pcx", "ArC_seVi.pcx", "ArC_seTe.pcx", "ArC_sePi.pcx" },
      { "ArC_coRe.pcx", "ArC_coBl.pcx", "ArC_coBr.pcx", "ArC_coGr.pcx", "ArC_coOr.pcx", "ArC_coVi.pcx", "ArC_coTe.pcx", "ArC_coPi.pcx" }
    },
    { 0, 106, 397,
      { "ArD_enRe.pcx", "ArD_enBl.pcx", "ArD_enBr.pcx", "ArD_enGr.pcx", "ArD_enOr.pcx", "ArD_enVi.pcx", "ArD_enTe.pcx", "ArD_enPi.pcx" },
      { "ArD_seRe.pcx", "ArD_seBl.pcx", "ArD_seBr.pcx", "ArD_seGr.pcx", "ArD_seOr.pcx", "ArD_seVi.pcx", "ArD_seTe.pcx", "ArD_sePi.pcx" },
      { "ArD_coRe.pcx", "ArD_coBl.pcx", "ArD_coBr.pcx", "ArD_coGr.pcx", "ArD_coOr.pcx", "ArD_coVi.pcx", "ArD_coTe.pcx", "ArD_coPi.pcx" }
    },
    { 0, 110, 275,
      { "ArE_enRe.pcx", "ArE_enBl.pcx", "ArE_enBr.pcx", "ArE_enGr.pcx", "ArE_enOr.pcx", "ArE_enVi.pcx", "ArE_enTe.pcx", "ArE_enPi.pcx" },
      { "ArE_seRe.pcx", "ArE_seBl.pcx", "ArE_seBr.pcx", "ArE_seGr.pcx", "ArE_seOr.pcx", "ArE_seVi.pcx", "ArE_seTe.pcx", "ArE_sePi.pcx" },
      { "ArE_coRe.pcx", "ArE_coBl.pcx", "ArE_coBr.pcx", "ArE_coGr.pcx", "ArE_coOr.pcx", "ArE_coVi.pcx", "ArE_coTe.pcx", "ArE_coPi.pcx" }
    },
    { 0, 159, 188,
      { "ArF_enRe.pcx", "ArF_enBl.pcx", "ArF_enBr.pcx", "ArF_enGr.pcx", "ArF_enOr.pcx", "ArF_enVi.pcx", "ArF_enTe.pcx", "ArF_enPi.pcx" },
      { "ArF_seRe.pcx", "ArF_seBl.pcx", "ArF_seBr.pcx", "ArF_seGr.pcx", "ArF_seOr.pcx", "ArF_seVi.pcx", "ArF_seTe.pcx", "ArF_sePi.pcx" },
      { "ArF_coRe.pcx", "ArF_coBl.pcx", "ArF_coBr.pcx", "ArF_coGr.pcx", "ArF_coOr.pcx", "ArF_coVi.pcx", "ArF_coTe.pcx", "ArF_coPi.pcx" }
    },
    { 0, 201, 261,
      { "ArG_enRe.pcx", "ArG_enBl.pcx", "ArG_enBr.pcx", "ArG_enGr.pcx", "ArG_enOr.pcx", "ArG_enVi.pcx", "ArG_enTe.pcx", "ArG_enPi.pcx" },
      { "ArG_seRe.pcx", "ArG_seBl.pcx", "ArG_seBr.pcx", "ArG_seGr.pcx", "ArG_seOr.pcx", "ArG_seVi.pcx", "ArG_seTe.pcx", "ArG_sePi.pcx" },
      { "ArG_coRe.pcx", "ArG_coBl.pcx", "ArG_coBr.pcx", "ArG_coGr.pcx", "ArG_coOr.pcx", "ArG_coVi.pcx", "ArG_coTe.pcx", "ArG_coPi.pcx" }
    },
    { 0, 233, 197,
      { "ArH_enRe.pcx", "ArH_enBl.pcx", "ArH_enBr.pcx", "ArH_enGr.pcx", "ArH_enOr.pcx", "ArH_enVi.pcx", "ArH_enTe.pcx", "ArH_enPi.pcx" },
      { "ArH_seRe.pcx", "ArH_seBl.pcx", "ArH_seBr.pcx", "ArH_seGr.pcx", "ArH_seOr.pcx", "ArH_seVi.pcx", "ArH_seTe.pcx", "ArH_sePi.pcx" },
      { "ArH_coRe.pcx", "ArH_coBl.pcx", "ArH_coBr.pcx", "ArH_coGr.pcx", "ArH_coOr.pcx", "ArH_coVi.pcx", "ArH_coTe.pcx", "ArH_coPi.pcx" }
    }
};

DATA(0x00662598)
static TCampaignMapTraits::TRegionTraits g_hsRegionTraits[4] = {
    { 0, 141, 326,
      { "HSA_enRe.pcx", "HSA_enBl.pcx", "HSA_enBr.pcx", "HSA_enGr.pcx", "HSA_enOr.pcx", "HSA_enVi.pcx", "HSA_enTe.pcx", "HSA_enPi.pcx" },
      { "HSA_seRe.pcx", "HSA_seBl.pcx", "HSA_seBr.pcx", "HSA_seGr.pcx", "HSA_seOr.pcx", "HSA_seVi.pcx", "HSA_seTe.pcx", "HSA_sePi.pcx" },
      { "HSA_coRe.pcx", "HSA_coBl.pcx", "HSA_coBr.pcx", "HSA_coGr.pcx", "HSA_coOr.pcx", "HSA_coVi.pcx", "HSA_coTe.pcx", "HSA_coPi.pcx" }
    },
    { 0, 239, 275,
      { "HSB_enRe.pcx", "HSB_enBl.pcx", "HSB_enBr.pcx", "HSB_enGr.pcx", "HSB_enOr.pcx", "HSB_enVi.pcx", "HSB_enTe.pcx", "HSB_enPi.pcx" },
      { "HSB_seRe.pcx", "HSB_seBl.pcx", "HSB_seBr.pcx", "HSB_seGr.pcx", "HSB_seOr.pcx", "HSB_seVi.pcx", "HSB_seTe.pcx", "HSB_sePi.pcx" },
      { "HSB_coRe.pcx", "HSB_coBl.pcx", "HSB_coBr.pcx", "HSB_coGr.pcx", "HSB_coOr.pcx", "HSB_coVi.pcx", "HSB_coTe.pcx", "HSB_coPi.pcx" }
    },
    { 0, 23, 161,
      { "HSC_enRe.pcx", "HSC_enBl.pcx", "HSC_enBr.pcx", "HSC_enGr.pcx", "HSC_enOr.pcx", "HSC_enVi.pcx", "HSC_enTe.pcx", "HSC_enPi.pcx" },
      { "HSC_seRe.pcx", "HSC_seBl.pcx", "HSC_seBr.pcx", "HSC_seGr.pcx", "HSC_seOr.pcx", "HSC_seVi.pcx", "HSC_seTe.pcx", "HSC_sePi.pcx" },
      { "HSC_coRe.pcx", "HSC_coBl.pcx", "HSC_coBr.pcx", "HSC_coGr.pcx", "HSC_coOr.pcx", "HSC_coVi.pcx", "HSC_coTe.pcx", "HSC_coPi.pcx" }
    },
    { 0, 6, 9,
      { "HSD_enRe.pcx", "HSD_enBl.pcx", "HSD_enBr.pcx", "HSD_enGr.pcx", "HSD_enOr.pcx", "HSD_enVi.pcx", "HSD_enTe.pcx", "HSD_enPi.pcx" },
      { "HSD_seRe.pcx", "HSD_seBl.pcx", "HSD_seBr.pcx", "HSD_seGr.pcx", "HSD_seOr.pcx", "HSD_seVi.pcx", "HSD_seTe.pcx", "HSD_sePi.pcx" },
      { "HSD_coRe.pcx", "HSD_coBl.pcx", "HSD_coBr.pcx", "HSD_coGr.pcx", "HSD_coOr.pcx", "HSD_coVi.pcx", "HSD_coTe.pcx", "HSD_coPi.pcx" }
    }
};

DATA(0x00662748)
static TCampaignMapTraits::TRegionTraits g_bbRegionTraits[5] = {
    { 0, 168, 342,
      { "BBA_enRe.pcx", "BBA_enBl.pcx", "BBA_enBr.pcx", "BBA_enGr.pcx", "BBA_enOr.pcx", "BBA_enVi.pcx", "BBA_enTe.pcx", "BBA_enPi.pcx" },
      { "BBA_seRe.pcx", "BBA_seBl.pcx", "BBA_seBr.pcx", "BBA_seGr.pcx", "BBA_seOr.pcx", "BBA_seVi.pcx", "BBA_seTe.pcx", "BBA_sePi.pcx" },
      { "BBA_coRe.pcx", "BBA_coBl.pcx", "BBA_coBr.pcx", "BBA_coGr.pcx", "BBA_coOr.pcx", "BBA_coVi.pcx", "BBA_coTe.pcx", "BBA_coPi.pcx" }
    },
    { 0, 218, 263,
      { "BBB_enRe.pcx", "BBB_enBl.pcx", "BBB_enBr.pcx", "BBB_enGr.pcx", "BBB_enOr.pcx", "BBB_enVi.pcx", "BBB_enTe.pcx", "BBB_enPi.pcx" },
      { "BBB_seRe.pcx", "BBB_seBl.pcx", "BBB_seBr.pcx", "BBB_seGr.pcx", "BBB_seOr.pcx", "BBB_seVi.pcx", "BBB_seTe.pcx", "BBB_sePi.pcx" },
      { "BBB_coRe.pcx", "BBB_coBl.pcx", "BBB_coBr.pcx", "BBB_coGr.pcx", "BBB_coOr.pcx", "BBB_coVi.pcx", "BBB_coTe.pcx", "BBB_coPi.pcx" }
    },
    { 0, 0, 71,
      { "BBC_enRe.pcx", "BBC_enBl.pcx", "BBC_enBr.pcx", "BBC_enGr.pcx", "BBC_enOr.pcx", "BBC_enVi.pcx", "BBC_enTe.pcx", "BBC_enPi.pcx" },
      { "BBC_seRe.pcx", "BBC_seBl.pcx", "BBC_seBr.pcx", "BBC_seGr.pcx", "BBC_seOr.pcx", "BBC_seVi.pcx", "BBC_seTe.pcx", "BBC_sePi.pcx" },
      { "BBC_coRe.pcx", "BBC_coBl.pcx", "BBC_coBr.pcx", "BBC_coGr.pcx", "BBC_coOr.pcx", "BBC_coVi.pcx", "BBC_coTe.pcx", "BBC_coPi.pcx" }
    },
    { 0, 291, 79,
      { "BBD_enRe.pcx", "BBD_enBl.pcx", "BBD_enBr.pcx", "BBD_enGr.pcx", "BBD_enOr.pcx", "BBD_enVi.pcx", "BBD_enTe.pcx", "BBD_enPi.pcx" },
      { "BBD_seRe.pcx", "BBD_seBl.pcx", "BBD_seBr.pcx", "BBD_seGr.pcx", "BBD_seOr.pcx", "BBD_seVi.pcx", "BBD_seTe.pcx", "BBD_sePi.pcx" },
      { "BBD_coRe.pcx", "BBD_coBl.pcx", "BBD_coBr.pcx", "BBD_coGr.pcx", "BBD_coOr.pcx", "BBD_coVi.pcx", "BBD_coTe.pcx", "BBD_coPi.pcx" }
    },
    { 0, 316, 199,
      { "BBE_enRe.pcx", "BBE_enBl.pcx", "BBE_enBr.pcx", "BBE_enGr.pcx", "BBE_enOr.pcx", "BBE_enVi.pcx", "BBE_enTe.pcx", "BBE_enPi.pcx" },
      { "BBE_seRe.pcx", "BBE_seBl.pcx", "BBE_seBr.pcx", "BBE_seGr.pcx", "BBE_seOr.pcx", "BBE_seVi.pcx", "BBE_seTe.pcx", "BBE_sePi.pcx" },
      { "BBE_coRe.pcx", "BBE_coBl.pcx", "BBE_coBr.pcx", "BBE_coGr.pcx", "BBE_coOr.pcx", "BBE_coVi.pcx", "BBE_coTe.pcx", "BBE_coPi.pcx" }
    }
};

DATA(0x00662968)
static TCampaignMapTraits::TRegionTraits g_nbRegionTraits[4] = {
    { 0, 7, 292,
      { "NBA_enRe.pcx", "NBA_enBl.pcx", "NBA_enBr.pcx", "NBA_enGr.pcx", "NBA_enOr.pcx", "NBA_enVi.pcx", "NBA_enTe.pcx", "NBA_enPi.pcx" },
      { "NBA_seRe.pcx", "NBA_seBl.pcx", "NBA_seBr.pcx", "NBA_seGr.pcx", "NBA_seOr.pcx", "NBA_seVi.pcx", "NBA_seTe.pcx", "NBA_sePi.pcx" },
      { "NBA_coRe.pcx", "NBA_coBl.pcx", "NBA_coBr.pcx", "NBA_coGr.pcx", "NBA_coOr.pcx", "NBA_coVi.pcx", "NBA_coTe.pcx", "NBA_coPi.pcx" }
    },
    { 0, 162, 334,
      { "NBB_enRe.pcx", "NBB_enBl.pcx", "NBB_enBr.pcx", "NBB_enGr.pcx", "NBB_enOr.pcx", "NBB_enVi.pcx", "NBB_enTe.pcx", "NBB_enPi.pcx" },
      { "NBB_seRe.pcx", "NBB_seBl.pcx", "NBB_seBr.pcx", "NBB_seGr.pcx", "NBB_seOr.pcx", "NBB_seVi.pcx", "NBB_seTe.pcx", "NBB_sePi.pcx" },
      { "NBB_coRe.pcx", "NBB_coBl.pcx", "NBB_coBr.pcx", "NBB_coGr.pcx", "NBB_coOr.pcx", "NBB_coVi.pcx", "NBB_coTe.pcx", "NBB_coPi.pcx" }
    },
    { 0, 63, 195,
      { "NBC_enRe.pcx", "NBC_enBl.pcx", "NBC_enBr.pcx", "NBC_enGr.pcx", "NBC_enOr.pcx", "NBC_enVi.pcx", "NBC_enTe.pcx", "NBC_enPi.pcx" },
      { "NBC_seRe.pcx", "NBC_seBl.pcx", "NBC_seBr.pcx", "NBC_seGr.pcx", "NBC_seOr.pcx", "NBC_seVi.pcx", "NBC_seTe.pcx", "NBC_sePi.pcx" },
      { "NBC_coRe.pcx", "NBC_coBl.pcx", "NBC_coBr.pcx", "NBC_coGr.pcx", "NBC_coOr.pcx", "NBC_coVi.pcx", "NBC_coTe.pcx", "NBC_coPi.pcx" }
    },
    { 0, 57, 46,
      { "NBD_enRe.pcx", "NBD_enBl.pcx", "NBD_enBr.pcx", "NBD_enGr.pcx", "NBD_enOr.pcx", "NBD_enVi.pcx", "NBD_enTe.pcx", "NBD_enPi.pcx" },
      { "NBD_seRe.pcx", "NBD_seBl.pcx", "NBD_seBr.pcx", "NBD_seGr.pcx", "NBD_seOr.pcx", "NBD_seVi.pcx", "NBD_seTe.pcx", "NBD_sePi.pcx" },
      { "NBD_coRe.pcx", "NBD_coBl.pcx", "NBD_coBr.pcx", "NBD_coGr.pcx", "NBD_coOr.pcx", "NBD_coVi.pcx", "NBD_coTe.pcx", "NBD_coPi.pcx" }
    }
};

DATA(0x00662b18)
static TCampaignMapTraits::TRegionTraits g_elRegionTraits[4] = {
    { 0, 12, 73,
      { "ELA_enRe.pcx", "ELA_enBl.pcx", "ELA_enBr.pcx", "ELA_enGr.pcx", "ELA_enOr.pcx", "ELA_enVi.pcx", "ELA_enTe.pcx", "ELA_enPi.pcx" },
      { "ELA_seRe.pcx", "ELA_seBl.pcx", "ELA_seBr.pcx", "ELA_seGr.pcx", "ELA_seOr.pcx", "ELA_seVi.pcx", "ELA_seTe.pcx", "ELA_sePi.pcx" },
      { "ELA_coRe.pcx", "ELA_coBl.pcx", "ELA_coBr.pcx", "ELA_coGr.pcx", "ELA_coOr.pcx", "ELA_coVi.pcx", "ELA_coTe.pcx", "ELA_coPi.pcx" }
    },
    { 0, 0, 242,
      { "ELB_enRe.pcx", "ELB_enBl.pcx", "ELB_enBr.pcx", "ELB_enGr.pcx", "ELB_enOr.pcx", "ELB_enVi.pcx", "ELB_enTe.pcx", "ELB_enPi.pcx" },
      { "ELB_seRe.pcx", "ELB_seBl.pcx", "ELB_seBr.pcx", "ELB_seGr.pcx", "ELB_seOr.pcx", "ELB_seVi.pcx", "ELB_seTe.pcx", "ELB_sePi.pcx" },
      { "ELB_coRe.pcx", "ELB_coBl.pcx", "ELB_coBr.pcx", "ELB_coGr.pcx", "ELB_coOr.pcx", "ELB_coVi.pcx", "ELB_coTe.pcx", "ELB_coPi.pcx" }
    },
    { 0, 255, 34,
      { "ELC_enRe.pcx", "ELC_enBl.pcx", "ELC_enBr.pcx", "ELC_enGr.pcx", "ELC_enOr.pcx", "ELC_enVi.pcx", "ELC_enTe.pcx", "ELC_enPi.pcx" },
      { "ELC_seRe.pcx", "ELC_seBl.pcx", "ELC_seBr.pcx", "ELC_seGr.pcx", "ELC_seOr.pcx", "ELC_seVi.pcx", "ELC_seTe.pcx", "ELC_sePi.pcx" },
      { "ELC_coRe.pcx", "ELC_coBl.pcx", "ELC_coBr.pcx", "ELC_coGr.pcx", "ELC_coOr.pcx", "ELC_coVi.pcx", "ELC_coTe.pcx", "ELC_coPi.pcx" }
    },
    { 0, 92, 144,
      { "ELD_enRe.pcx", "ELD_enBl.pcx", "ELD_enBr.pcx", "ELD_enGr.pcx", "ELD_enOr.pcx", "ELD_enVi.pcx", "ELD_enTe.pcx", "ELD_enPi.pcx" },
      { "ELD_seRe.pcx", "ELD_seBl.pcx", "ELD_seBr.pcx", "ELD_seGr.pcx", "ELD_seOr.pcx", "ELD_seVi.pcx", "ELD_seTe.pcx", "ELD_sePi.pcx" },
      { "ELD_coRe.pcx", "ELD_coBl.pcx", "ELD_coBr.pcx", "ELD_coGr.pcx", "ELD_coOr.pcx", "ELD_coVi.pcx", "ELD_coTe.pcx", "ELD_coPi.pcx" }
    }
};

DATA(0x00662cc8)
static TCampaignMapTraits::TRegionTraits g_rnRegionTraits[4] = {
    { 0, 85, 319,
      { "RNA_enRe.pcx", "RNA_enBl.pcx", "RNA_enBr.pcx", "RNA_enGr.pcx", "RNA_enOr.pcx", "RNA_enVi.pcx", "RNA_enTe.pcx", "RNA_enPi.pcx" },
      { "RNA_seRe.pcx", "RNA_seBl.pcx", "RNA_seBr.pcx", "RNA_seGr.pcx", "RNA_seOr.pcx", "RNA_seVi.pcx", "RNA_seTe.pcx", "RNA_sePi.pcx" },
      { "RNA_coRe.pcx", "RNA_coBl.pcx", "RNA_coBr.pcx", "RNA_coGr.pcx", "RNA_coOr.pcx", "RNA_coVi.pcx", "RNA_coTe.pcx", "RNA_coPi.pcx" }
    },
    { 0, 195, 275,
      { "RNB_enRe.pcx", "RNB_enBl.pcx", "RNB_enBr.pcx", "RNB_enGr.pcx", "RNB_enOr.pcx", "RNB_enVi.pcx", "RNB_enTe.pcx", "RNB_enPi.pcx" },
      { "RNB_seRe.pcx", "RNB_seBl.pcx", "RNB_seBr.pcx", "RNB_seGr.pcx", "RNB_seOr.pcx", "RNB_seVi.pcx", "RNB_seTe.pcx", "RNB_sePi.pcx" },
      { "RNB_coRe.pcx", "RNB_coBl.pcx", "RNB_coBr.pcx", "RNB_coGr.pcx", "RNB_coOr.pcx", "RNB_coVi.pcx", "RNB_coTe.pcx", "RNB_coPi.pcx" }
    },
    { 0, 68, 185,
      { "RNC_enRe.pcx", "RNC_enBl.pcx", "RNC_enBr.pcx", "RNC_enGr.pcx", "RNC_enOr.pcx", "RNC_enVi.pcx", "RNC_enTe.pcx", "RNC_enPi.pcx" },
      { "RNC_seRe.pcx", "RNC_seBl.pcx", "RNC_seBr.pcx", "RNC_seGr.pcx", "RNC_seOr.pcx", "RNC_seVi.pcx", "RNC_seTe.pcx", "RNC_sePi.pcx" },
      { "RNC_coRe.pcx", "RNC_coBl.pcx", "RNC_coBr.pcx", "RNC_coGr.pcx", "RNC_coOr.pcx", "RNC_coVi.pcx", "RNC_coTe.pcx", "RNC_coPi.pcx" }
    },
    { 0, 78, 30,
      { "RND_enRe.pcx", "RND_enBl.pcx", "RND_enBr.pcx", "RND_enGr.pcx", "RND_enOr.pcx", "RND_enVi.pcx", "RND_enTe.pcx", "RND_enPi.pcx" },
      { "RND_seRe.pcx", "RND_seBl.pcx", "RND_seBr.pcx", "RND_seGr.pcx", "RND_seOr.pcx", "RND_seVi.pcx", "RND_seTe.pcx", "RND_sePi.pcx" },
      { "RND_coRe.pcx", "RND_coBl.pcx", "RND_coBr.pcx", "RND_coGr.pcx", "RND_coOr.pcx", "RND_coVi.pcx", "RND_coTe.pcx", "RND_coPi.pcx" }
    }
};

DATA(0x00662e78)
static TCampaignMapTraits::TRegionTraits g_uaRegionTraits[12] = {
    { 0, 158, 409,
      { "UAA_enRe.pcx", "UAA_enBl.pcx", "UAA_enBr.pcx", "UAA_enGr.pcx", "UAA_enOr.pcx", "UAA_enVi.pcx", "UAA_enTe.pcx", "UAA_enPi.pcx" },
      { "UAA_seRe.pcx", "UAA_seBl.pcx", "UAA_seBr.pcx", "UAA_seGr.pcx", "UAA_seOr.pcx", "UAA_seVi.pcx", "UAA_seTe.pcx", "UAA_sePi.pcx" },
      { "UAA_coRe.pcx", "UAA_coBl.pcx", "UAA_coBr.pcx", "UAA_coGr.pcx", "UAA_coOr.pcx", "UAA_coVi.pcx", "UAA_coTe.pcx", "UAA_coPi.pcx" }
    },
    { 0, 63, 346,
      { "UAB_enRe.pcx", "UAB_enBl.pcx", "UAB_enBr.pcx", "UAB_enGr.pcx", "UAB_enOr.pcx", "UAB_enVi.pcx", "UAB_enTe.pcx", "UAB_enPi.pcx" },
      { "UAB_seRe.pcx", "UAB_seBl.pcx", "UAB_seBr.pcx", "UAB_seGr.pcx", "UAB_seOr.pcx", "UAB_seVi.pcx", "UAB_seTe.pcx", "UAB_sePi.pcx" },
      { "UAB_coRe.pcx", "UAB_coBl.pcx", "UAB_coBr.pcx", "UAB_coGr.pcx", "UAB_coOr.pcx", "UAB_coVi.pcx", "UAB_coTe.pcx", "UAB_coPi.pcx" }
    },
    { 0, 9, 8,
      { "UAC_enRe.pcx", "UAC_enBl.pcx", "UAC_enBr.pcx", "UAC_enGr.pcx", "UAC_enOr.pcx", "UAC_enVi.pcx", "UAC_enTe.pcx", "UAC_enPi.pcx" },
      { "UAC_seRe.pcx", "UAC_seBl.pcx", "UAC_seBr.pcx", "UAC_seGr.pcx", "UAC_seOr.pcx", "UAC_seVi.pcx", "UAC_seTe.pcx", "UAC_sePi.pcx" },
      { "UAC_coRe.pcx", "UAC_coBl.pcx", "UAC_coBr.pcx", "UAC_coGr.pcx", "UAC_coOr.pcx", "UAC_coVi.pcx", "UAC_coTe.pcx", "UAC_coPi.pcx" }
    },
    { 0, 207, 1,
      { "UAD_enRe.pcx", "UAD_enBl.pcx", "UAD_enBr.pcx", "UAD_enGr.pcx", "UAD_enOr.pcx", "UAD_enVi.pcx", "UAD_enTe.pcx", "UAD_enPi.pcx" },
      { "UAD_seRe.pcx", "UAD_seBl.pcx", "UAD_seBr.pcx", "UAD_seGr.pcx", "UAD_seOr.pcx", "UAD_seVi.pcx", "UAD_seTe.pcx", "UAD_sePi.pcx" },
      { "UAD_coRe.pcx", "UAD_coBl.pcx", "UAD_coBr.pcx", "UAD_coGr.pcx", "UAD_coOr.pcx", "UAD_coVi.pcx", "UAD_coTe.pcx", "UAD_coPi.pcx" }
    },
    { 0, 133, 357,
      { "UAE_enRe.pcx", "UAE_enBl.pcx", "UAE_enBr.pcx", "UAE_enGr.pcx", "UAE_enOr.pcx", "UAE_enVi.pcx", "UAE_enTe.pcx", "UAE_enPi.pcx" },
      { "UAE_seRe.pcx", "UAE_seBl.pcx", "UAE_seBr.pcx", "UAE_seGr.pcx", "UAE_seOr.pcx", "UAE_seVi.pcx", "UAE_seTe.pcx", "UAE_sePi.pcx" },
      { "UAE_coRe.pcx", "UAE_coBl.pcx", "UAE_coBr.pcx", "UAE_coGr.pcx", "UAE_coOr.pcx", "UAE_coVi.pcx", "UAE_coTe.pcx", "UAE_coPi.pcx" }
    },
    { 0, 185, 83,
      { "UAF_enRe.pcx", "UAF_enBl.pcx", "UAF_enBr.pcx", "UAF_enGr.pcx", "UAF_enOr.pcx", "UAF_enVi.pcx", "UAF_enTe.pcx", "UAF_enPi.pcx" },
      { "UAF_seRe.pcx", "UAF_seBl.pcx", "UAF_seBr.pcx", "UAF_seGr.pcx", "UAF_seOr.pcx", "UAF_seVi.pcx", "UAF_seTe.pcx", "UAF_sePi.pcx" },
      { "UAF_coRe.pcx", "UAF_coBl.pcx", "UAF_coBr.pcx", "UAF_coGr.pcx", "UAF_coOr.pcx", "UAF_coVi.pcx", "UAF_coTe.pcx", "UAF_coPi.pcx" }
    },
    { 0, 160, 263,
      { "UAG_enRe.pcx", "UAG_enBl.pcx", "UAG_enBr.pcx", "UAG_enGr.pcx", "UAG_enOr.pcx", "UAG_enVi.pcx", "UAG_enTe.pcx", "UAG_enPi.pcx" },
      { "UAG_seRe.pcx", "UAG_seBl.pcx", "UAG_seBr.pcx", "UAG_seGr.pcx", "UAG_seOr.pcx", "UAG_seVi.pcx", "UAG_seTe.pcx", "UAG_sePi.pcx" },
      { "UAG_coRe.pcx", "UAG_coBl.pcx", "UAG_coBr.pcx", "UAG_coGr.pcx", "UAG_coOr.pcx", "UAG_coVi.pcx", "UAG_coTe.pcx", "UAG_coPi.pcx" }
    },
    { 0, 109, 173,
      { "UAH_enRe.pcx", "UAH_enBl.pcx", "UAH_enBr.pcx", "UAH_enGr.pcx", "UAH_enOr.pcx", "UAH_enVi.pcx", "UAH_enTe.pcx", "UAH_enPi.pcx" },
      { "UAH_seRe.pcx", "UAH_seBl.pcx", "UAH_seBr.pcx", "UAH_seGr.pcx", "UAH_seOr.pcx", "UAH_seVi.pcx", "UAH_seTe.pcx", "UAH_sePi.pcx" },
      { "UAH_coRe.pcx", "UAH_coBl.pcx", "UAH_coBr.pcx", "UAH_coGr.pcx", "UAH_coOr.pcx", "UAH_coVi.pcx", "UAH_coTe.pcx", "UAH_coPi.pcx" }
    },
    { 0, 56, 127,
      { "UAI_enRe.pcx", "UAI_enBl.pcx", "UAI_enBr.pcx", "UAI_enGr.pcx", "UAI_enOr.pcx", "UAI_enVi.pcx", "UAI_enTe.pcx", "UAI_enPi.pcx" },
      { "UAI_seRe.pcx", "UAI_seBl.pcx", "UAI_seBr.pcx", "UAI_seGr.pcx", "UAI_seOr.pcx", "UAI_seVi.pcx", "UAI_seTe.pcx", "UAI_sePi.pcx" },
      { "UAI_coRe.pcx", "UAI_coBl.pcx", "UAI_coBr.pcx", "UAI_coGr.pcx", "UAI_coOr.pcx", "UAI_coVi.pcx", "UAI_coTe.pcx", "UAI_coPi.pcx" }
    },
    { 0, 10, 252,
      { "UAJ_enRe.pcx", "UAJ_enBl.pcx", "UAJ_enBr.pcx", "UAJ_enGr.pcx", "UAJ_enOr.pcx", "UAJ_enVi.pcx", "UAJ_enTe.pcx", "UAJ_enPi.pcx" },
      { "UAJ_seRe.pcx", "UAJ_seBl.pcx", "UAJ_seBr.pcx", "UAJ_seGr.pcx", "UAJ_seOr.pcx", "UAJ_seVi.pcx", "UAJ_seTe.pcx", "UAJ_sePi.pcx" },
      { "UAJ_coRe.pcx", "UAJ_coBl.pcx", "UAJ_coBr.pcx", "UAJ_coGr.pcx", "UAJ_coOr.pcx", "UAJ_coVi.pcx", "UAJ_coTe.pcx", "UAJ_coPi.pcx" }
    },
    { 0, 211, 176,
      { "UAK_enRe.pcx", "UAK_enBl.pcx", "UAK_enBr.pcx", "UAK_enGr.pcx", "UAK_enOr.pcx", "UAK_enVi.pcx", "UAK_enTe.pcx", "UAK_enPi.pcx" },
      { "UAK_seRe.pcx", "UAK_seBl.pcx", "UAK_seBr.pcx", "UAK_seGr.pcx", "UAK_seOr.pcx", "UAK_seVi.pcx", "UAK_seTe.pcx", "UAK_sePi.pcx" },
      { "UAK_coRe.pcx", "UAK_coBl.pcx", "UAK_coBr.pcx", "UAK_coGr.pcx", "UAK_coOr.pcx", "UAK_coVi.pcx", "UAK_coTe.pcx", "UAK_coPi.pcx" }
    },
    { 0, 261, 210,
      { "UAL_enRe.pcx", "UAL_enBl.pcx", "UAL_enBr.pcx", "UAL_enGr.pcx", "UAL_enOr.pcx", "UAL_enVi.pcx", "UAL_enTe.pcx", "UAL_enPi.pcx" },
      { "UAL_seRe.pcx", "UAL_seBl.pcx", "UAL_seBr.pcx", "UAL_seGr.pcx", "UAL_seOr.pcx", "UAL_seVi.pcx", "UAL_seTe.pcx", "UAL_sePi.pcx" },
      { "UAL_coRe.pcx", "UAL_coBl.pcx", "UAL_coBr.pcx", "UAL_coGr.pcx", "UAL_coOr.pcx", "UAL_coVi.pcx", "UAL_coTe.pcx", "UAL_coPi.pcx" }
    }
};

DATA(0x00663388)
static TCampaignMapTraits::TRegionTraits g_spRegionTraits[4] = {
    { 0, 8, 295,
      { "SPA_enRe.pcx", "SPA_enBl.pcx", "SPA_enBr.pcx", "SPA_enGr.pcx", "SPA_enOr.pcx", "SPA_enVi.pcx", "SPA_enTe.pcx", "SPA_enPi.pcx" },
      { "SPA_seRe.pcx", "SPA_seBl.pcx", "SPA_seBr.pcx", "SPA_seGr.pcx", "SPA_seOr.pcx", "SPA_seVi.pcx", "SPA_seTe.pcx", "SPA_sePi.pcx" },
      { "SPA_coRe.pcx", "SPA_coBl.pcx", "SPA_coBr.pcx", "SPA_coGr.pcx", "SPA_coOr.pcx", "SPA_coVi.pcx", "SPA_coTe.pcx", "SPA_coPi.pcx" }
    },
    { 0, 45, 141,
      { "SPB_enRe.pcx", "SPB_enBl.pcx", "SPB_enBr.pcx", "SPB_enGr.pcx", "SPB_enOr.pcx", "SPB_enVi.pcx", "SPB_enTe.pcx", "SPB_enPi.pcx" },
      { "SPB_seRe.pcx", "SPB_seBl.pcx", "SPB_seBr.pcx", "SPB_seGr.pcx", "SPB_seOr.pcx", "SPB_seVi.pcx", "SPB_seTe.pcx", "SPB_sePi.pcx" },
      { "SPB_coRe.pcx", "SPB_coBl.pcx", "SPB_coBr.pcx", "SPB_coGr.pcx", "SPB_coOr.pcx", "SPB_coVi.pcx", "SPB_coTe.pcx", "SPB_coPi.pcx" }
    },
    { 0, 142, 21,
      { "SPC_enRe.pcx", "SPC_enBl.pcx", "SPC_enBr.pcx", "SPC_enGr.pcx", "SPC_enOr.pcx", "SPC_enVi.pcx", "SPC_enTe.pcx", "SPC_enPi.pcx" },
      { "SPC_seRe.pcx", "SPC_seBl.pcx", "SPC_seBr.pcx", "SPC_seGr.pcx", "SPC_seOr.pcx", "SPC_seVi.pcx", "SPC_seTe.pcx", "SPC_sePi.pcx" },
      { "SPC_coRe.pcx", "SPC_coBl.pcx", "SPC_coBr.pcx", "SPC_coGr.pcx", "SPC_coOr.pcx", "SPC_coVi.pcx", "SPC_coTe.pcx", "SPC_coPi.pcx" }
    },
    { 0, 244, 156,
      { "SPD_enRe.pcx", "SPD_enBl.pcx", "SPD_enBr.pcx", "SPD_enGr.pcx", "SPD_enOr.pcx", "SPD_enVi.pcx", "SPD_enTe.pcx", "SPD_enPi.pcx" },
      { "SPD_seRe.pcx", "SPD_seBl.pcx", "SPD_seBr.pcx", "SPD_seGr.pcx", "SPD_seOr.pcx", "SPD_seVi.pcx", "SPD_seTe.pcx", "SPD_sePi.pcx" },
      { "SPD_coRe.pcx", "SPD_coBl.pcx", "SPD_coBr.pcx", "SPD_coGr.pcx", "SPD_coOr.pcx", "SPD_coVi.pcx", "SPD_coTe.pcx", "SPD_coPi.pcx" }
    }
};

DATA(0x00663538)
TCampaignMapTraits g_campaignMapTraitsImp[21] = {
    { 0, 0, 0, 0 },
    { 0, "G1_bg.pcx", 3, g_good1RegionTraits },
    { 0, "G2_bg.pcx", 4, g_good2RegionTraits },
    { 0, "G3_bg.pcx", 3, g_good3RegionTraits },
    { 0, "E1_bg.pcx", 7, g_evil1RegionTraits },
    { 0, "E2_bg.pcx", 4, g_evil2RegionTraits },
    { 0, "N1_bg.pcx", 3, g_neutral1RegionTraits },
    { 0, "S1_bg.pcx", 3, g_secret1RegionTraits },
    { 0, "BR_bg.pcx", 4, g_brRegionTraits },
    { 0, "IS_bg.pcx", 4, g_isRegionTraits },
    { 0, "KR_bg.pcx", 4, g_krRegionTraits },
    { 0, "NI_bg.pcx", 4, g_niRegionTraits },
    { 0, "TA_bg.pcx", 3, g_taRegionTraits },
    { 0, "AR_bg.pcx", 8, g_arRegionTraits },
    { 0, "HS_bg.pcx", 4, g_hsRegionTraits },
    { 0, "BB_bg.pcx", 5, g_bbRegionTraits },
    { 0, "NB_bg.pcx", 4, g_nbRegionTraits },
    { 0, "EL_bg.pcx", 4, g_elRegionTraits },
    { 0, "RN_bg.pcx", 4, g_rnRegionTraits },
    { 0, "UA_bg.pcx", 12, g_uaRegionTraits },
    { 0, "SP_bg.pcx", 4, g_spRegionTraits }
};

DATA(0x00663688)
const TCampaignMapTraits (&g_campaignMapTraits)[21] = g_campaignMapTraitsImp;

// The region-name loader writes through this parallel pointer table.
DATA(0x0063bc50)
TCampaignMapTraits::TRegionTraits* const g_campaignRegionTraits[21] = {
    0,
    g_good1RegionTraits,
    g_good2RegionTraits,
    g_good3RegionTraits,
    g_evil1RegionTraits,
    g_evil2RegionTraits,
    g_neutral1RegionTraits,
    g_secret1RegionTraits,
    g_brRegionTraits,
    g_isRegionTraits,
    g_krRegionTraits,
    g_niRegionTraits,
    g_taRegionTraits,
    g_arRegionTraits,
    g_hsRegionTraits,
    g_bbRegionTraits,
    g_nbRegionTraits,
    g_elRegionTraits,
    g_rnRegionTraits,
    g_uaRegionTraits,
    g_spRegionTraits
};

DC_ADDRESS(0x05af64, 0x22c)
VA(0x0045dee0, 0x310) MAC_ADDRESS(0x069734, 0x424)
unsigned char initializeCampaignMapTraitsTable()
{
    DATA_COMPGEN_GUARD(0x00694df8, campaignNamesGuard, campaignNames)
    VA_COMPGEN(0x0045e1f0, 0x16, STATIC_DTOR, campaignNames)
    DATA(0x00694e00)
    static TAutoArrayPtr<char> campaignNames;

    TResourcePtr<TTextResource> textResource(
        ResourceManager::getText(
            DATA_COMPGEN(0x0066b7bc, campaignTextName, "camptext.txt")));
    if (!textResource.get())
        return 0;

    unsigned strSize = 0;
    int textLine = 1;
    unsigned campaign;
    for (campaign = 0; campaign < 21; ++campaign) {
        if (g_campaignMapTraits[campaign].m_numRegions > 0) {
            strSize += strlen(textResource->getText(textLine)) + 1;
            ++textLine;
        }
    }

    for (campaign = 0; campaign < 21; ++campaign) {
        if (g_campaignMapTraits[campaign].m_numRegions > 0) {
            while (strlen(textResource->getText(textLine)) == 0)
                ++textLine;
            ++textLine;
            for (unsigned region = 0;
                 region < g_campaignMapTraits[campaign].m_numRegions;
                 ++region) {
                strSize += strlen(textResource->getText(textLine)) + 1;
                ++textLine;
            }
        }
    }

    campaignNames = TAutoArrayPtr<char>(new char[strSize]);
    if (!campaignNames.get())
        return 0;

    char* destination = campaignNames.get();
    textLine = 1;
    for (campaign = 0; campaign < 21; ++campaign) {
        if (g_campaignMapTraits[campaign].m_numRegions > 0) {
            const char* source = textResource->getText(textLine);
            unsigned length = strlen(source) + 1;
            memcpy(destination, source, length);
            g_campaignMapTraitsImp[campaign].m_name = destination;
            destination += length;
            ++textLine;
        }
    }

    for (campaign = 0; campaign < 21; ++campaign) {
        if (g_campaignMapTraits[campaign].m_numRegions > 0) {
            while (strlen(textResource->getText(textLine)) == 0)
                ++textLine;
            ++textLine;
            for (unsigned region = 0;
                 region < g_campaignMapTraits[campaign].m_numRegions;
                 ++region) {
                const char* source = textResource->getText(textLine);
                unsigned length = strlen(source) + 1;
                memcpy(destination, source, length);
                g_campaignRegionTraits[campaign][region].m_name = destination;
                destination += length;
                ++textLine;
            }
        }
    }

    return 1;
}
