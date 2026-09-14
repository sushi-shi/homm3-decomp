// exec.h - prototypes of exec.cpp (compiland exec.obj)
#ifndef HOMM3_EXEC_H
#define HOMM3_EXEC_H

#include "basemgr.h"
#include "textresource.h"

// Dreamcast roster verbatim: headManager@0, tailManager@4,
// currentManager@8, dialogReturn@12 - byte-corroborated by the retail
// Add/RemoveManager list walks (tail-first, like heroWindow's widget
// list).
// Before normalization (type): executive.
class Executive {
public:
    BaseManager* m_headManager;
    BaseManager* m_tailManager;
    BaseManager* m_currentManager;
    long m_dialogReturn;

    Executive();
    int initSystem();
    void shutDownSystem();
    int addManager(BaseManager* newManager, int newPriority);
    int doDialog(BaseManager* newDialog);
    void removeManager(BaseManager* killManager);
    void callManager(BaseManager* newManager);
    void mainLoop();
};

// events.obj joins the gate for the refugee camp (0x4a4600), whose
// recruit dialog is run through gpExecutive->DoDialog. The pointer stays
// invisible to every TU with no consumer.
extern Executive* g_executive;

void aiShutDown();

// philai.obj's cooperative main-loop pump. ai_player.obj calls it between
// each enemy mobility calculation and path seed; keeping the declaration in
// this narrow executive surface avoids importing philai.h's skill roster.
void checkDoMain(int forceMouseCheck, int mouseOnly);         // 0x5242d0

#endif  /* HOMM3_EXEC_H */
