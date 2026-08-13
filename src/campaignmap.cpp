// campaignmap.cpp - E:\gamedcs\campaignmap.cpp (compiland campaignmap.obj)
// HAND-OWNED after admission. Nine Dreamcast AutoArrayPtr / ResourcePtr
// roster entries are header emissions and have no distinct retail bodies in
// this compiland.
#include <va.h>
#include <string.h>
#include "campaignmap.h"
#include "textresource.h"
#include "resourcemanager.h"

// Retail's underlying writable array is immediately followed by the public
// const-reference cell. The large static initializer remains unclaimed in
// this first code admission; these independently located declarations give
// the function's relocations source authority without fabricating its data.
DATA(0x00663538)
TCampaignMapTraits aCampaignMapTraitsImp[21];

DATA(0x00663688)
const TCampaignMapTraits (&akCampaignMapTraits)[21] = aCampaignMapTraitsImp;

// Exact 21-pointer walk used by the region-name pass (entry zero is null).
DATA(0x0063bc50)
TCampaignMapTraits::TRegionTraits* const aCampaignRegionTraits[21] = { 0 };

// E:\gamedcs\campaignmap.cpp:161
VA(0x0045dee0, 0x310)  // anchor-string/caller, dc 0x5af64
unsigned char InitializeCampaignMapTraitsTable()
{
    DATA_COMPGEN_GUARD(0x00694df8, campaignNamesGuard, campaignNames)
    VA_COMPGEN(0x0045e1f0, 0x16, STATIC_DTOR, campaignNames)
    DATA(0x00694e00)
    static TAutoArrayPtr<char> campaignNames;

    TResourcePtr<TTextResource> pTextResource(
        ResourceManager::GetText(
            DATA_COMPGEN(0x0066b7bc, campaignTextName, "camptext.txt")));
    if (!pTextResource.get())
        return 0;

    unsigned strSize = 0;
    int textLine = 1;
    unsigned campaign;
    for (campaign = 0; campaign < 21; ++campaign) {
        if (akCampaignMapTraits[campaign].m_numRegions > 0) {
            strSize += strlen(pTextResource->GetText(textLine)) + 1;
            ++textLine;
        }
    }

    for (campaign = 0; campaign < 21; ++campaign) {
        if (akCampaignMapTraits[campaign].m_numRegions > 0) {
            while (strlen(pTextResource->GetText(textLine)) == 0)
                ++textLine;
            ++textLine;
            for (unsigned region = 0;
                 region < akCampaignMapTraits[campaign].m_numRegions;
                 ++region) {
                strSize += strlen(pTextResource->GetText(textLine)) + 1;
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
        if (akCampaignMapTraits[campaign].m_numRegions > 0) {
            const char* source = pTextResource->GetText(textLine);
            unsigned length = strlen(source) + 1;
            memcpy(destination, source, length);
            aCampaignMapTraitsImp[campaign].m_name = destination;
            destination += length;
            ++textLine;
        }
    }

    for (campaign = 0; campaign < 21; ++campaign) {
        if (akCampaignMapTraits[campaign].m_numRegions > 0) {
            while (strlen(pTextResource->GetText(textLine)) == 0)
                ++textLine;
            ++textLine;
            for (unsigned region = 0;
                 region < akCampaignMapTraits[campaign].m_numRegions;
                 ++region) {
                const char* source = pTextResource->GetText(textLine);
                unsigned length = strlen(source) + 1;
                memcpy(destination, source, length);
                aCampaignRegionTraits[campaign][region].m_name = destination;
                destination += length;
                ++textLine;
            }
        }
    }

    return 1;
}
