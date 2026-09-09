// campaign.h - the Complete campaign-set chooser owned by campaign.cpp.
#ifndef HOMM3_CAMPAIGN_H
#define HOMM3_CAMPAIGN_H

#include "window.h"

class message;

// Complete-only campaign-set chooser used by kb.cpp's DoCampaignWindow.
// Retail constructor 0x456ec0 derives heroWindow directly and the caller's
// adjacent stack objects bound the complete object to the 0x4c-byte base;
// 0x457230/0x4574a0 are its destructor and modal wrapper.  No Dreamcast class
// name survives, so the role name remains deliberately conservative.
//
// The owning module is src/campaign.cpp, in the retail link-order gap
// between button and campaignbrief. The module and class names remain
// provisional because this chooser has no Dreamcast counterpart.
class TCampaignSetWindow : public heroWindow {
public:
    // Its modal result selects the TCampaignWindow page passed by each
    // retail arm; the fourth result opens the Complete-only custom chooser.
    enum ECampaignSetResults {
        CAMPAIGN_SET_SOD_ID = 0,
        CAMPAIGN_SET_AB_ID = 1,
        CAMPAIGN_SET_ROE_ID = 2,
        CUSTOM_CAMPAIGN_ID = 3
    };

    // The five plate widgets, in construction order.  The Armageddon's
    // Blade plate is built only in video-game-state 3, so every id after
    // it shifts down by one when that state is not held - which is what
    // the handler's `*gpVideoGameState == 3 ? 104 : 103` high bound
    // spells.
    enum EWidgetIDs {
        SOD_PLATE_ID = 100,
        LAST_PLATE_ID = 103,
        LAST_PLATE_WITH_AB_ID = 104
    };

    // The gpGeneralText rows each plate answers a right-click with. Retail
    // folds them into the load offset (`[ecx + 0xb7c]` = GetText(735)); the
    // values are those offsets divided by four. Names describe the plate.
    enum ECampaignSetHelpText {
        CAMPAIGN_SET_ARM_HELP = 725,
        CAMPAIGN_SET_ROE_HELP = 726,
        CAMPAIGN_SET_CUS_HELP = 727,
        CAMPAIGN_SET_EXIT_HELP = 728,
        CAMPAIGN_SET_SOD_HELP = 735
    };

    TCampaignSetWindow();
    virtual ~TCampaignSetWindow();
    // Slot 3 of vtable 0x63bc08 (0x4574d0): the hover sweep that lights
    // the plate under the mouse and repaints the plate band.
    // Before normalization (function): TCampaignSetWindow::handle_message.
    virtual int handleMessage(message& msg);
    // Before normalization (function): TCampaignSetWindow::DoModal.
    void doModal();
};
SIZE(TCampaignSetWindow, 0x4c);

#endif  // HOMM3_CAMPAIGN_H
