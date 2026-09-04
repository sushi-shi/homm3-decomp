// customcampaignwindow.cpp - Complete-only compiland; no Dreamcast twin.
// HAND-OWNED. Retail's two static-initializer runs bracket it: cinit0275..
// 0285 at 0x4823c0 open the object right after cursor.obj's SendMapChange
// (0x482390), and cinit0287..0296 at 0x483b40 open customcampaign.obj.
// Everything between - the "Select a Campaign" window constructor
// (0x4827b0, "CamCust.pcx"), its Maps\*.h3c scanner, the campaign-name
// sort predicate, the list refresh and the std::sort COMDATs the scanner
// instantiates - is this unit. Its calls into CampaignHeaderStruct's
// members are cross-TU, which is why retail keeps every one of them out of
// line where a single-TU spelling let /Ob2 expand them. The compiland's
// real name is unknown (alphabetically it sits between cursor and
// customcampaign); the unit name is provisional.
#include <va.h>
#include <algorithm>
#include <direct.h>
#include <io.h>
#include "game.h"
#include "campaignbrief.h"
#include "customcampaign.h"
#include "misc.h"
#include "slider.h"

#if 0  // @carcass - Complete-only "Select a Campaign" window constructor (1831 B of widget setup).
VA(0x004827b0, 0x727)  // DoCampaignWindow's CUSTOM_CAMPAIGN_ID arm + CamCust.pcx, retail-only
TCustomCampaignWindow::TCustomCampaignWindow()
{
    // @stub
}
#endif  // @carcass

// Complete-only. Scans Maps\\*.h3c through the CRT _find family (each step
// bracketed by a _getcwd retail kept), keeps every header that loads with
// at least one map, sorts by campaign name and sizes the slider for the
// eighteen-row list.
//
// Residual (65.03%): the std::sort expansion. Retail CALLS _Sort (its
// COMDAT is 0x483940, with _Median/_Unguarded_partition folded in) and
// _Insertion_sort_1, and EXPANDS _Unguarded_insert (the predicate is
// called straight from the unguarded loop); our /Ob2 spends the budget
// the other way round - _Sort expanded with its children called, and
// _Unguarded_insert called. Every other site agrees (the ctor, Load,
// GetNumMaps and FreeData are cross-TU calls, push_back's insert(it,n,x)
// COMDAT is called on both sides). The remaining call rows are name-only
// (_getcwd, operator new, the Load/UpdateList stubs). Pins are not used
// in this lane; the lever is the /Ob2 site budget.
VA(0x00482fd0, 0x264)  // TCustomCampaignWindow ctor callee + _findfirst("*.h3c"), retail-only
void TCustomCampaignWindow::LoadCampaignList()
{
    char currentDirectory[100];
    _finddata_t fileInfo;

    _getcwd(currentDirectory, sizeof(currentDirectory));
    _chdir(DATA_COMPGEN(0x006755ac, mapsDirectory, "Maps"));
    _getcwd(currentDirectory, sizeof(currentDirectory));
    long findHandle = _findfirst(
        DATA_COMPGEN(0x006755a4, campaignFilePattern, "*.h3c"), &fileInfo);
    _chdir(DATA_COMPGEN(0x006755a0, parentDirectory, ".."));
    _getcwd(currentDirectory, sizeof(currentDirectory));
    if (findHandle == -1)
        return;

    do {
        TCampaignBrief::CampaignHeaderStruct* header =
            new TCampaignBrief::CampaignHeaderStruct(fileInfo.name);
        _getcwd(currentDirectory, sizeof(currentDirectory));
        if (!header->Load()) {
            delete header;
        } else if (header->GetNumMaps() == 0) {
            header->FreeData();
            delete header;
        } else {
            header->FreeData();
            campaignHeaders.push_back(header);
        }
        _getcwd(currentDirectory, sizeof(currentDirectory));
    } while (_findnext(findHandle, &fileInfo) == 0);
    _findclose(findHandle);
    _getcwd(currentDirectory, sizeof(currentDirectory));

    std::sort(campaignHeaders.begin(), campaignHeaders.end(),
              CampaignHeaderPointerLess());
    if (campaignHeaders.size() > CAMPAIGN_LIST_ROWS) {
        campaignSlider->send_message(widget::WIDGET_SET_STATUS, 6);
        campaignSlider->SetResolution(
            campaignHeaders.size() - (CAMPAIGN_LIST_ROWS - 1));
    }
    UpdateList();
}

// Complete-only: the sort predicate, by campaign name. Retail evaluates
// the right operand's name first (VC6's right-to-left argument order for
// the inlined string operator<) and destroys the two temporaries in
// reverse.
VA(0x00483240, 0xEA)  // _Insertion_sort_1/_Sort predicate call sites, retail-only
bool CampaignHeaderPointerLess::operator()(void* left, void* right) const
{
    return static_cast<TCampaignBrief::CampaignHeaderStruct*>(left)
               ->GetCampaignName()
           < static_cast<TCampaignBrief::CampaignHeaderStruct*>(right)
                 ->GetCampaignName();
}

#if 0  // @carcass - Complete-only list refresh: needs the type_text_scroller model for the description widget.
VA(0x00483330, 0x281)  // LoadCampaignList's tail callee, retail-only
void TCustomCampaignWindow::UpdateList()
{
    // @stub
}
#endif  // @carcass


// Exact Dinkumware _Insertion_sort_1 body retained by the Complete-only
// custom-campaign list sort. Retail proves four-byte pointer elements, the
// by-value empty predicate, the extra _Ty** discriminator, and all three
// out-of-line predicate calls. The stock VC6 template is byte-identical.
VA_COMPGEN(0x00483AA0, 0xA0, INSERTION_SORT_1,
           CampaignHeaderPointerLess)
