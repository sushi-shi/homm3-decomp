#ifndef HOMM3_CAMPAIGNWINDOW_H
#define HOMM3_CAMPAIGNWINDOW_H

#include "window.h"

class message;
class Bitmap816;

// Retail's constructor initializes heroWindow directly, installs vtable
// 0x63bca4, and accesses derived storage through +0x78.  That tail differs
// substantially from Dreamcast's smaller campaign roster, so it remains
// deliberately unmodelled until a compiled consumer needs it.
class TCampaignWindow : public heroWindow {
public:
    // Widget ids, byte-proven by the handler: the seven preview rows it
    // sweeps live at 101..107, and the campaign selector answers 108..127 -
    // twenty campaigns, exactly the twenty 0x50-byte rows of
    // gCampaignPreviews (the handler's `lea edi,[edi+4*edi-0x21c]; shl
    // edi,4` is &gCampaignPreviews[id - 108]).
    // The three plates and the backdrop all answer 100, and the
    // per-campaign completion check marks answer 128..134 - the
    // constructor's `i - firstCampaign + 0x80`, one per visible row.
    enum EOtherWidgetIDs {
        BACKGROUND_ID = 100,
        PREVIEW_FIRST_ID = 101,
        PREVIEW_LAST_ID = 107,
        CAMPAIGN_FIRST_ID = 108,
        CAMPAIGN_LAST_ID = 127,
        CHECK_FIRST_ID = 128
    };

    // The constructor's inlined reserve allocates 0x134 bytes for the
    // widget vector, i.e. 77 pointers.
    enum { NWIDGETS = 77 };

    // Every preview still is 200x116, in OpenPreview's VideoOpen call and
    // again in the bitmapBorder16 it builds for the same row.
    enum EPreviewSize {
        PREVIEW_WIDTH = 200,
        PREVIEW_HEIGHT = 116
    };

    // The pages of campaigns the dialog selects between, and the
    // constructor's second argument. Its switch is the proof of the
    // split: page 0 seeds firstCampaign 0 and gates rows 0..6, page 1
    // seeds 7 and gates rows 7..12, page 2 seeds 13, gates rows 13..19
    // and swaps the backdrop to campbkx2.pcx. Seven, six and seven rows
    // are exactly Restoration of Erathia's, Armageddon's Blade's and
    // Shadow of Death's campaign counts, in shipping order.
    enum ECampaignSets {
        CAMPAIGN_SET_ROE = 0,
        CAMPAIGN_SET_AB = 1,
        CAMPAIGN_SET_SOD = 2
    };

    // The one campaign row that unlocks only when every other row on its
    // page is finished: the constructor seeds it available with the rest
    // of the Armageddon's Blade page and then clears it on the first
    // incomplete sibling, skipping itself in that test. Its state is
    // also what gates the page's forward plate.
    enum ECampaignRows {
        CAMPAIGN_ROW_AB_SEALED = 11
    };

    // The escape key, as puzzlewindow's DIALOG_CLOSE_KEY. The handler
    // dispatches on it with a switch: retail's `mov eax,[esi+4]; dec eax`
    // is a switch selector load, not a `cmp mem,1`, and keeping it a switch
    // is what stops VC6 caching the literal 1 in a callee-saved register.
    enum EKeys {
        DIALOG_CLOSE_KEY = 1
    };

    // +0x4c/+0x50: the constructor seeds 0 and -1 into them before any
    // other derived store. Dreamcast names these two retained words;
    // +0x54..+0x5f is untouched by every body carved here.
    // Dreamcast lastActive at +0x44 maps to PC +0x4c. Both constructors
    // clear it before the campaign state; no further retail use is reconstructed.
    int m_lastActive;
    // Dreamcast currentCampVideo at +0x48 maps to PC +0x50. Both
    // constructors initialize it to -1 in the same statement order.
    int m_currentCampVideo;
    // Dreamcast's three-member tail follows currentCampVideo at +0x48:
    // saveVideoFile (+0x4c, void*), CheckMark (+0x50, Bitmap816*), and
    // RolloverWidget (+0x54, const widget*). The retained PC members
    // above shift by +8, leaving exactly this tail before the new
    // campaignAvailable array at +0x60. No retail uses of this tail are
    // located; declarations follow source/layout evidence.
    void* m_saveVideoFile;               // +0x54
    Bitmap816* m_checkMark;              // +0x58
    const widget* m_rolloverWidget;      // +0x5c
    // +0x60. One byte per campaign row, cleared as a single inlined
    // 21-byte memset (five dwords plus a byte) and then filled by the
    // constructor's three-way switch; the widget loop walks 0..19 and the
    // page switch writes as high as index 19, so the block is the same
    // 21 wide as SCampaign::campaignCompleted, which every gate reads.
    unsigned char m_campaignAvailable[21];
    // The PC constructor clears 21 campaign bytes at +0x60, then
    // stores firstCampaign at +0x78. These three bytes align the dword.
    char m_paddingBeforeFirstCampaign[3];
    // +0x78. The handler subtracts it (plus 7) from a campaign id to reach
    // that campaign's preview widget, so it is the ordinal of the first
    // campaign the current page shows; the constructor seeds 0, 7 or 13.
    int m_firstCampaign;

    // DC's sole int newCampaign is the reset flag (test at dc 0x5b5c0).
    // Complete narrows that flag to a byte and adds the campaign-set slot.
    // Native bool versus unsigned char remains unresolved; the second
    // parameter's current spelling is not a recovered DC name.
    TCampaignWindow(unsigned char newGame, int newCampaign);
    virtual ~TCampaignWindow();
    void doModal();
    void openPreview(int campaignIndex);
    // Retail emits no out-of-line body: every caller expands it under /Ob2,
    // and the expansion is register-visible - the hoisted `this` is the EBX
    // the handler's two preview sweeps share (see campaignwindow.cpp).
    void hideText();
};

// Retail /Gr passes the message in ECX, as DoDialog's TDialogHandler does.
int campaignWindowHandler(message& msg);

// Complete-only initialized campaign-preview table at retail 0x66c498:
// twenty 0x50-byte rows, one per campaign, indexed by `id -
// CAMPAIGN_FIRST_ID`. Every one of the eight descriptor dwords is
// byte-proven, six of them twice over - OpenPreview (0x45e7c0) hands
// +0x00/+0x04/+0x08 to VideoOpen and then builds the row's still as
// `bitmapBorder16(+0x04, +0x08, 200, 116, +0x1c, +0x18, 0x800)`, which
// fixes the image name and the widget id; the constructor reads
// +0x04/+0x08 again for the check-mark plate and +0x0c/+0x10/+0x14 for
// the caption box. The 12-dword tail is the row's private snapshot of
// the consecutive Bink state beginning at gBinkVideo, written by
// OpenPreview and restored by the destructor and the hover handler.
struct SCampaignPreview {
    int m_video;
    int m_x;
    int m_y;
    int m_textX;
    int m_textY;
    int m_textWidth;
    const char* m_image;
    int m_widgetId;
    int m_binkState[12];
};
SIZE(SCampaignPreview, 0x50);
extern SCampaignPreview g_campaignPreviews[20];

extern THelpText g_campaignWindowHelp[24];

// The twenty campaign data-file names the handler hands to
// SCampaign::select_campaign. Retail addresses them as
// `[4*id + 0x66c92c]` with id in 108..127, i.e. base 0x66cadc indexed by
// id - CAMPAIGN_FIRST_ID - the constant-folded form of a twenty-entry
// table that starts immediately after the 0x66cad8 hover latch. No DATA
// claim yet: the fold means the table's own extent is not carved.
extern const char* g_campaignFileNames[20];

#endif  /* HOMM3_CAMPAIGNWINDOW_H */
