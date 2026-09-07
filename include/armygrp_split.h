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
enum TSplitWidgetId {
    SPLIT_WIDGET_SOURCE_ENTRY = 4,
    SPLIT_WIDGET_DESTINATION_ENTRY = 5
};

enum TSplitDialogReturn {
    DIALOG_RETURN_SPLIT_CLOSE = 0x7800,
    DIALOG_RETURN_SPLIT_CANCEL = 0x7801
};

// Retail allocates 0x80 bytes for this source-private dialog.
class TSplitWindow : public CAdvPopup {
public:
    // Before normalization: splitSlider.
    slider* m_splitSlider;                // +0x60, widget id 6
    // Before normalization: sourceEntry.
    textEntryWidget* m_sourceEntry;       // +0x64, widget id 4
    // Before normalization: destinationEntry.
    textEntryWidget* m_destinationEntry;  // +0x68, widget id 5
    // Before normalization: totalTroops.
    int m_totalTroops;             // +0x6c
    // Before normalization: sourceTroops.
    int m_sourceTroops;            // +0x70
    // Before normalization: destinationTroops.
    int m_destinationTroops;       // +0x74
    // Before normalization: minimumTransfer.
    signed char m_minimumTransfer; // +0x78
    // Before normalization: sourceMustKeep.
    signed char m_sourceMustKeep;  // +0x79
private:
    // Before normalization: pad_7a.
    // The retail constructor places the two keep-army bytes at
    // +0x78/+0x79 and the creature dword at +0x7c; these bytes align that word.
    char m_paddingBeforeCreature[2];
public:
    // Before normalization: creature.
    TCreatureType m_creature;      // +0x7c

    TSplitWindow(int x2, int y2, TCreatureType thisArmy);
    virtual ~TSplitWindow();
    // Before normalization (function): TSplitWindow::UpdateSplitArmy.
    inline void updateSplitArmy(unsigned char update);
    // Before normalization (function): TSplitWindow::SetRolloverText.
    inline void setRolloverText(int codeY);
    // Before normalization (function): TSplitWindow::WindowHandler.
    virtual int windowHandler(message* msg);
};
SIZE(TSplitWindow, 0x80);

#endif  /* HOMM3_ARMYGRP_SPLIT_H */
