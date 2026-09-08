// Complete custom-campaign selection window; the retail source name is provisional.
#ifndef HOMM3_CUSTOMCAMPAIGNWINDOW_H
#define HOMM3_CUSTOMCAMPAIGNWINDOW_H

#include "customcampaign.h"
#include "campaignbrief.h"
#include "window.h"

class slider;
class textWidget;
class type_text_scroller;

// The Complete-only "Select a Campaign" list (constructor 0x4827b0,
// "CamCust.pcx"). Every field is byte-proven by the constructor's stores
// and the members' reads: the two eighteen-row textWidget arrays at
// +0x50 / +0x98 (ids 100..117 name, 118..135 count - the deselect
// override maps that range back to a row), the selected-name text at
// +0xe0, the description scroller at +0xe4, the slider at +0xe8, the
// scroll origin at +0xec, the selection at +0xf0, the double-click
// timestamp at +0xf4 (GameTime::Get) and the header vector at +0xf8.
// Names INVENTED (no Dreamcast twin).
class TCustomCampaignWindow : public CHeroWindowEx {
public:
    enum {
        CAMPAIGN_LIST_ROWS = 18
    };

    textWidget* nameWidgets[CAMPAIGN_LIST_ROWS];   // +0x50
    textWidget* countWidgets[CAMPAIGN_LIST_ROWS];  // +0x98
    textWidget* selectedName;         // +0xe0
    type_text_scroller* description;  // +0xe4
    slider* campaignSlider;           // +0xe8
    int firstVisible;                 // +0xec
    int selected;                     // +0xf0
    unsigned long lastClickTime;      // +0xf4
    // LoadCampaignList binds insert's const T& straight to its
    // CampaignHeaderStruct* local (address-taken, memory-homed), which a
    // void* element would have copied through a temporary first.
    std::vector<TCampaignBrief::CampaignHeaderStruct*> campaignHeaders;  // +0xf8

    TCustomCampaignWindow();
    virtual ~TCustomCampaignWindow();
    // Slot 12 of vtable 0x63d6fc (retail 0x4835c0): the one override the
    // window adds over CHeroWindowEx; a row click selects, a second click
    // inside 400 ms accepts.
    virtual int OnWidgetDeselect(int id, unsigned char* bExitFlag);
    void LoadCampaignList();
    void UpdateList();
    // Retail 0x483670 (name provisional): hands the selected header's
    // file name to gpGame->campaign.select_campaign(20, ...).
    bool AcceptSelection();
};
SIZE(TCustomCampaignWindow, 0x108);

// Complete's custom-campaign list orders its header pointers through this
// predicate. The predicate body is a separate retail helper; this owner
// header carries its one authoritative type shape for the retained STL sort
// specialization in customcampaign.obj.
class CampaignHeaderPointerLess {
public:
    bool operator()(TCampaignBrief::CampaignHeaderStruct* left,
                    TCampaignBrief::CampaignHeaderStruct* right) const;
};

#endif // HOMM3_CUSTOMCAMPAIGNWINDOW_H
