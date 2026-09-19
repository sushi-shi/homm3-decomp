#ifndef HOMM3_CAMPAIGNMAP_H
#define HOMM3_CAMPAIGNMAP_H

#include <va.h>
#include "ownership.h"

// Retail Complete layout, proved by InitializeCampaignMapTraitsTable's
// 0x10 campaign stride and 0x6c region stride. Dreamcast attests the six
// semantic member names, but its RoE-era region stores only one image
// pointer per state. Retail stores one per player color (the static PCX
// pointer runs at 0x660eb8 make all 24 slots explicit).
struct TCampaignMapTraits {
    struct TRegionTraits {
        const char* m_name;
        int m_offsetX;
        int m_offsetY;
        const char* m_enabledImageName[8];
        const char* m_selectedImageName[8];
        const char* m_conqueredImageName[8];
    };

    const char* m_name;
    const char* m_imageName;
    int m_numRegions;
    const TRegionTraits* m_regionTraits;
};
SIZE(TCampaignMapTraits::TRegionTraits, 108);
SIZE(TCampaignMapTraits, 16);

extern const TCampaignMapTraits (&g_campaignMapTraits)[21];
extern TCampaignMapTraits g_campaignMapTraitsImp[21];
extern TCampaignMapTraits::TRegionTraits* const g_campaignRegionTraits[21];

unsigned char initializeCampaignMapTraitsTable();

#endif  /* HOMM3_CAMPAIGNMAP_H */
