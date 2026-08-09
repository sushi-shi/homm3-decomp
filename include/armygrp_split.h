// armygrp_split.h - source-private split-army dialog reconstruction
#ifndef HOMM3_ARMYGRP_SPLIT_H
#define HOMM3_ARMYGRP_SPLIT_H

#include <va.h>
#include "advmgr.h"
#include "advmgr_popup.h"
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

// Correct retail interface for the split dialog's slider. slider.h retains
// recruit.obj's older provisional roster because changing that globally
// perturbs initialize_game_data's otherwise exact optimizer state.
class TSplitSliderInterface : public widget {
public:
    virtual void SetResolution(int num);
    virtual void SetState(int state);
};

// Retail allocates 0x80 bytes for this source-private dialog.
class TSplitWindow : public CAdvPopup {
public:
    TSplitSliderInterface* splitSlider; // +0x60, widget id 6
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
    int creature;                // +0x7c (TCreatureType domain)

    TSplitWindow(int x2, int y2, int thisArmy);
    virtual ~TSplitWindow();
    inline void UpdateSplitArmy(unsigned char update);
    virtual int WindowHandler(message* msg);
};
SIZE(TSplitWindow, 0x80);

// Constructor-only concrete tail. The target allocates 0x68 bytes and the
// constructor body supplies these inherited widget overrides.
class TSplitSliderView : public TSplitSliderInterface {
    char pad_30[0x38];
public:
    TSplitSliderView(int x, int y, int w, int h, int id, int num,
                     void (*callback)(int, heroWindow*), int graphics,
                     int page, unsigned char hotKey);
    virtual int Main(message* msg);
    virtual void zBufferDraw();
    virtual void Draw();
    virtual int GetRealHeight();
    virtual int GetRealWidth();
};
SIZE(TSplitSliderView, 0x68);

#endif  /* HOMM3_ARMYGRP_SPLIT_H */
