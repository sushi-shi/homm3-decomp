#ifndef HOMM3_VIEWWRLD_H
#define HOMM3_VIEWWRLD_H

#include "advmgr_popup.h"
#include "struct.h"

class type_func_button;

// Dreamcast supplies the shared four-member tail after CAdvPopup. Complete
// keeps that tail, adds two level-selector buttons between RolloverWidget and
// origin, and grows the object to 0x78. The retail ViewWorld caller constructs
// this object at [ebp-0x8c] and puts the following type_point at [ebp-0x14],
// independently fixing the extent. The constructor/callback family proves
// SurfaceButton@+0x64, UndergroundButton@+0x68, origin@+0x6c and the two
// dimensions at +0x70/+0x74; the inherited 0x60-byte base fixes the remaining
// Dreamcast RolloverWidget at +0x60.
class TViewWorldWindow : public CAdvPopup {
public:
    // The constructor's own append order fixes every id below: the three
    // magnification buttons carry VWMag1/VWMag2/VWMag4.def at 16, 17 and
    // 18, the puzzle button VWPuz.def at 19, and the 144x144 mini-map
    // border at 20. WindowHandler cases on all five plus the house
    // 0x7802 accept id. Spellings are role-based - neither corpus names
    // them - and the scale each magnification arm installs is what fixes
    // WHICH is which (16 -> 7.68f, 17 -> 11.84f, 18 -> 16.0f).
    enum EOtherWidgetIDs {
        MAP_ID = 0,
        MAGNIFY_FAR_ID = 16,
        MAGNIFY_MID_ID = 17,
        MAGNIFY_FULL_ID = 18,
        PUZZLE_ID = 19,
        RADAR_ID = 20,
        ACCEPT_ID = 0x7802
    };
    // Dreamcast's older revision reserves two slots. Complete's constructor
    // compares the Dinkumware vector capacity against 0x28 and allocates 160
    // bytes, directly proving the revised constant.
    enum { NWIDGETS = 40 };

private:
    // CodeView type 0x1A89 is pointer-to-const-widget, not a button pointer.
    // Like the dimension-door twin, this inherited source member is not
    // initialized by the constructor.
    const widget* m_rolloverWidget;
    type_func_button* m_surfaceButton;
    type_func_button* m_undergroundButton;
    type_point m_origin;
    int m_viewableWidth;
    int m_viewableHeight;
    // Complete's two level callbacks (0x5fbdf0 / 0x5fbec0) are free
    // functions the constructor hands to the level buttons; they read
    // and write origin and the extents directly.
    friend int viewWorldSurfaceHandler(message& msg);
    friend int viewWorldUndergroundHandler(message& msg);
    // advManager::ViewWorld reads origin and both extents straight out of
    // its stack-constructed window to feed VWCompleteDraw's five
    // arguments, so the owner of that entry sees the same private tail the
    // two callbacks above do.
    friend class advManager;

public:
    TViewWorldWindow();
    virtual ~TViewWorldWindow();
    void init(type_point newCenter, unsigned char updateFlag);
    using CAdvPopup::drawWindow;
    void drawWindow();
    virtual int windowHandler(message& msg);

private:
    int convertID2HelpID(int id) const;
    void updateRadar(int mrx, int mry, float radarDivisor);
    void updateViewWorld(message* msg);
};
SIZE(TViewWorldWindow, 0x78);

int viewWorldSurfaceHandler(message& msg);
int viewWorldUndergroundHandler(message& msg);

// The "adventure repaint suppressed" latch whose DATA claim advmgr.cpp
// holds (src/advmgr.cpp:6277), recorded there as having no located writer.
// advManager::ViewWorld IS that writer - it raises the latch on entry and
// drops it just before the closing UpdateRadar - and 0x6aac3c sits inside
// viewwrld.obj's own .bss run (0x6aab68 .. 0x6aac3c), so this compiland
// owns the definition. The claim is left where it stands rather than moved
// across lanes; this is the declaration its writer compiles against.
extern int g_unnamed6aac3c;
// cmbtmgr.h's modal-screen latch (retail .bss 0x698a18). ViewWorld parks
// it at 2 for the life of the view-world dialog, which is what kb.cpp's
// fast-cycle guard tests. Declared here rather than by pulling cmbtmgr.h
// into a TU that has no other use for it.
extern int g_combatActive698a18;

#endif  /* HOMM3_VIEWWRLD_H */
