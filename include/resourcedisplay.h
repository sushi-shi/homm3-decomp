#ifndef HOMM3_RESOURCEDISPLAY_H
#define HOMM3_RESOURCEDISPLAY_H

#include "subwindow.h"

class bitmapBorder;
class border;
class heroWindow;
class textWidget;

// PROVEN retail layout, size 0x78. The constructor stores the seven
// text pointers at +0x38..+0x50, the seven border pointers at
// +0x54..+0x6c, the bitmap background at +0x70 and the final status
// text at +0x74. TSubWindow is exactly 0x34 bytes; isSmall occupies
// the derived head byte at +0x34.
class TResourceDisplay : public TSubWindow {
public:
    // The widget-id bands the constructor stamps in its resource loop
    // (0x3e9 + i on the seven textWidgets, 0x3f1 + i on the seven
    // borders). townManager::SetCommandAndText dispatches rollover
    // text on both bands one id wider - the shared building-record
    // table it indexes carries eight rows. Names INVENTED from the
    // constructor's own loop; no DC symbol covers the ids.
    enum EWidgetIDs {
        RESOURCE_TEXT_0_ID = 0x3e9,
        RESOURCE_TEXT_1_ID = 0x3ea,
        RESOURCE_TEXT_2_ID = 0x3eb,
        RESOURCE_TEXT_3_ID = 0x3ec,
        RESOURCE_TEXT_4_ID = 0x3ed,
        RESOURCE_TEXT_5_ID = 0x3ee,
        RESOURCE_TEXT_6_ID = 0x3ef,
        RESOURCE_TEXT_7_ID = 0x3f0,
        RESOURCE_BORDER_0_ID = 0x3f1,
        RESOURCE_BORDER_1_ID = 0x3f2,
        RESOURCE_BORDER_2_ID = 0x3f3,
        RESOURCE_BORDER_3_ID = 0x3f4,
        RESOURCE_BORDER_4_ID = 0x3f5,
        RESOURCE_BORDER_5_ID = 0x3f6,
        RESOURCE_BORDER_6_ID = 0x3f7
    };

private:
    unsigned char m_isSmall;

public:
    // Dreamcast places IsSmall immediately before three alignment
    // bytes and the widget array; NH3API confirms the PC +0x34/+0x38 offsets.
    char m_paddingBeforeResourceWidgets[3];

private:
    textWidget* m_resourceWidgets[7];

public:
    border* m_resourceIconWidgets[7];
    bitmapBorder* m_backgroundWidget;
    textWidget* m_dayWidget;

    TResourceDisplay(heroWindow* parent, bool isSmall);
    virtual ~TResourceDisplay();
    void update(bool draw, bool update);
    void clear();
};
SIZE(TResourceDisplay, 0x78);

#endif  /* HOMM3_RESOURCEDISPLAY_H */
