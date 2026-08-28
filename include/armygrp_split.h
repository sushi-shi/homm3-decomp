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
    slider* splitSlider;                // +0x60, widget id 6
    textEntryWidget* sourceEntry;       // +0x64, widget id 4
    textEntryWidget* destinationEntry;  // +0x68, widget id 5
    int totalTroops;             // +0x6c
    int sourceTroops;            // +0x70
    int destinationTroops;       // +0x74
    signed char minimumTransfer; // +0x78
    signed char sourceMustKeep;  // +0x79
private:
    char pad_7a[2];
public:
    TCreatureType creature;      // +0x7c

    TSplitWindow(int x2, int y2, TCreatureType thisArmy);
    virtual ~TSplitWindow();
    inline void UpdateSplitArmy(unsigned char update);
    inline void SetRolloverText(int codeY);
    virtual int WindowHandler(message* msg);
};
SIZE(TSplitWindow, 0x80);

#endif  /* HOMM3_ARMYGRP_SPLIT_H */
