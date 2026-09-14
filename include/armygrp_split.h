// armygrp_split.h - source-private split-army dialog reconstruction
#ifndef HOMM3_ARMYGRP_SPLIT_H
#define HOMM3_ARMYGRP_SPLIT_H

#include <va.h>
#include "advmgr.h"
#include "advmgr_popup.h"
#include "slider.h"
#include "textntry.h"
#include "widget.h"

class message;

// Widget and dialog-return domains proven by TSplitWindow's constructor and
// handler. The 0x7800 close result has no stronger semantic name yet.
// Before normalization (type): TSplitWidgetId.
#ifndef SplitWidgetId
#define SplitWidgetId TSplitWidgetId
#endif
enum SplitWidgetId {
    SPLIT_WIDGET_SOURCE_ENTRY = 4,
    SPLIT_WIDGET_DESTINATION_ENTRY = 5
};

// Before normalization (type): TSplitDialogReturn.
#ifndef SplitDialogReturn
#define SplitDialogReturn TSplitDialogReturn
#endif
enum SplitDialogReturn {
    DIALOG_RETURN_SPLIT_CLOSE = 0x7800,
    DIALOG_RETURN_SPLIT_CANCEL = 0x7801
};

// Retail allocates 0x80 bytes for this source-private dialog.
// Before normalization (type): TSplitWindow.
#ifndef SplitWindow
#define SplitWindow TSplitWindow
#endif
class SplitWindow : public CAdvPopup {
public:
    slider* m_splitSlider;                // +0x60, widget id 6
    textEntryWidget* m_sourceEntry;       // +0x64, widget id 4
    textEntryWidget* m_destinationEntry;  // +0x68, widget id 5
    int m_totalTroops;             // +0x6c
    int m_sourceTroops;            // +0x70
    int m_destinationTroops;       // +0x74
    signed char m_minimumTransfer; // +0x78
    signed char m_sourceMustKeep;  // +0x79

private:
    // The retail constructor places the two keep-army bytes at
    // +0x78/+0x79 and the creature dword at +0x7c; these bytes align that word.
    char m_paddingBeforeCreature[2];

public:
    CreatureType m_creature;      // +0x7c

    SplitWindow(int x2, int y2, CreatureType thisArmy);
    virtual ~SplitWindow();
    inline void updateSplitArmy(unsigned char update);
    inline void setRolloverText(int codeY);
    virtual int windowHandler(message& msg);
};
SIZE(SplitWindow, 0x80);

#endif  /* HOMM3_ARMYGRP_SPLIT_H */
