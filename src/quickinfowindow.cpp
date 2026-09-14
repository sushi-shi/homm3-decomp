// quickinfowindow.cpp - E:\gamedcs\quickinfowindow.cpp (compiland quickinfowindow.obj)
// 4 functions in link order.
#include <va.h>
#include "creaturetype.h"
#include <stdio.h>
#include <string.h>
#include "quickinfowindow.h"
#include "exec.h"
#include "iconwdgt.h"
#include "kb.h"
#include "textwdgt.h"

VA(0x0052f8c0, 0x430)  // dc 0x11787c
QuickCreatureWindow::QuickCreatureWindow(ViewLevel viewLevel,
    CreatureType id, int count, Disposition disposition, int cost)
    : DialogBoxWindow(0, 0, 256, 256, 0x12)
{
    m_widgets.reserve(m_widgets.size() + 3);

    m_widgets.push_back(new iconWidget(
        99, 26, 58, 64, 1000, "TwCrPort.def", id + 2, 0, 0, 0, 0x10));
    addWidget(m_widgets.back(), -1);

    if (viewLevel == ViewAll) {
        if (count < 1000) {
            sprintf(g_text, "%d %s", count, getArmyName(id, count));
        } else {
            sprintf(g_text, "%dk %s", count / 1000,
                    getArmyName(id, count));
        }
    } else {
        sprintf(g_text, "%s %s", ArmyGroup::getArmySizeName(count, 1),
                getArmyName(id, 0));
    }

    m_widgets.push_back(new textWidget(
        16, 110, 224, 36, g_text, "smalfont.fnt", font::PRIMARY,
        1001, 1, 0, 8));
    addWidget(m_widgets.back(), -1);

    if (viewLevel == ViewAll) {
        switch (disposition) {
        case Flee:
            strcpy(g_text,
                   g_generalText->getText(GENERAL_TEXT_QUICK_CREATURE_FLEE));
            break;
        case Attack:
            strcpy(g_text,
                   g_generalText->getText(GENERAL_TEXT_QUICK_CREATURE_ATTACK));
            break;
        case Join:
            strcpy(g_text,
                   g_generalText->getText(GENERAL_TEXT_QUICK_CREATURE_JOIN));
            break;
        case JoinPrice:
            sprintf(g_text,
                    g_generalText->getText(
                        GENERAL_TEXT_QUICK_CREATURE_JOIN_COST),
                    cost);
            break;
        }

        m_widgets.push_back(new textWidget(
            16, 156, 224, 74, g_text, "smalfont.fnt", font::PRIMARY,
            1001, 1, 0, 8));
        addWidget(m_widgets.back(), -1);
    }
}

VA_COMPGEN(0x0052fcf0, 0x21, SCALAR_DELETING_DTOR, TQuickCreatureWindow)

VA(0x0052fd20, 0xB)  // dc 0x117b5c
QuickCreatureWindow::~QuickCreatureWindow()
{
}

// E:\gamedcs\quickinfowindow.cpp:88
// Retail folds this identical body with the quick-hero/quick-town wrappers at
// 0x530d30 (`gpWindowManager->DoQuickView(this)`). It therefore has no distinct
// quickinfowindow contribution to claim; the shared representative belongs to
// the later quicktownwindow span.
#if 0  // @carcass
DC_ONLY(0x117b8c, 0x28)
void QuickCreatureWindow::quickWindowWait()
{
    // @stub
}
#endif
