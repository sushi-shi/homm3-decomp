#include "va.h"

#include "exec.h"

#include "advmgr.h"
#include "basemgr.h"
#include "inputmgr.h"
#include "kb.h"
#include "kbwin.h"
#include "mousemgr.h"
#include "soundmgr.h"
#include "terrain.h"
#include "window.h"
#include "winmgr.h"

// Project-inferred shared operation. Keep the text lookup after a failed add;
// shutDown may return when shutdown is already in progress.
void executive::addManagerOrShutDown(baseManager* newManager)
{
    if (addManager(newManager, -1))
        shutDown(g_generalText->getText(GENERAL_TEXT_ADD_MANAGER_ERROR));
}

// Project-inferred resume shared by normal return and exception unwinding.
// Redraw/menu restoration belongs only to the successful caller path.
static void resumeAdventureManager()
{
    g_advManager->setStatus(baseManager::STATUS_ACTIVE);
    g_advManager->m_advWindow->sleepAllWidgets(0);
}

VA(0x004b0900, 0x10)
DC_ADDRESS(0x09e510, 0xe)
MAC_ADDRESS(0x0c3910, 0x18)
executive::executive()
{
    m_headManager = 0;
    m_tailManager = 0;
    m_currentManager = 0;
    m_dialogReturn = 0;
}

// gpGeneralText is the canonical TTextResource loaded from genrltxt.txt.
VA(0x004b0910, 0x79)
DC_ADDRESS(0x09e520, 0x74)
MAC_ADDRESS(0x0c3928, 0xdc)
int executive::initSystem()
{
    if (g_inputManager->open(-1))
        shutDown(g_generalText->getText(GENERAL_TEXT_INPUT_DEVICE_INITIALIZATION_ERROR));
    if (addManager(g_mouseManager, -1))
        shutDown(g_generalText->getText(GENERAL_TEXT_MOUSE_INITIALIZATION_ERROR));
    if (addManager(g_windowManager, -1))
        shutDown(g_generalText->getText(GENERAL_TEXT_WINDOWS_INITIALIZATION_ERROR));
    return 0;
}

VA(0x004b0990, 0x78)
DC_ADDRESS(0x09e594, 0xd8)
MAC_ADDRESS(0x0c3a04, 0xf0)
void executive::shutDownSystem()
{
    g_shutDownDone = 1;
    g_soundManager->close();
    earlyShutDownSystem();

    baseManager* thisManager = m_headManager;
    while (thisManager) {
        baseManager* nextManager = thisManager->m_nextManager;
        if (thisManager != g_windowManager && thisManager != g_mouseManager)
            removeManager(thisManager);
        thisManager = nextManager;
    }
    if (g_windowManager->getStatus() == baseManager::STATUS_ACTIVE)
        removeManager(g_windowManager);
    if (g_mouseManager->getStatus() == baseManager::STATUS_ACTIVE)
        removeManager(g_mouseManager);
    g_inputManager->close();
}

VA(0x004b0a10, 0x10B)
DC_ADDRESS(0x09e66c, 0x10a)
MAC_ADDRESS(0x0c3af4, 0x2c0)
int executive::doDialog(baseManager* newDialog)
{
    executive dialogExec;
    baseManager* savedMgr[20];
    baseManager* savedPrev[20];
    baseManager* savedNext[20];
    baseManager* m;
    int count = 0;
    int i;

    for (m = m_headManager; m; count++) {
        savedMgr[count] = m;
        savedPrev[count] = m->m_prevManager;
        savedNext[count] = m->m_nextManager;
        m = m->m_nextManager;
    }
    addManagerOrShutDown(newDialog);
    dialogExec.addManagerOrShutDown(g_mouseManager);
    dialogExec.addManagerOrShutDown(g_windowManager);
    dialogExec.addManagerOrShutDown(newDialog);
    dialogExec.mainLoop();
    removeManager(newDialog);
    for (i = 0; i < count; i++) {
        savedMgr[i]->m_prevManager = savedPrev[i];
        savedMgr[i]->m_nextManager = savedNext[i];
    }
    return dialogExec.m_dialogReturn;
}

VA(0x004b0b20, 0xCB)
DC_ADDRESS(0x09e778, 0xc0)
MAC_ADDRESS(0x0c3db4, 0x14c)
int executive::addManager(baseManager* newManager, int newPriority)
{
    if (!newManager)
        return 3;
    if (newPriority == -1) {
        if (!m_tailManager)
            newPriority = 0;
        else
            newPriority = m_tailManager->m_priority + 1;
    }
    if (!newManager->getStatus() && newManager->open(newPriority))
        return 3;
    baseManager* current = m_tailManager;
    while (current && current->m_priority > newPriority)
        current = current->m_prevManager;
    if (!current) {
        newManager->m_nextManager = m_headManager;
        newManager->m_prevManager = 0;
        if (m_headManager)
            m_headManager->m_prevManager = newManager;
        m_headManager = newManager;
        if (!m_tailManager)
            m_tailManager = newManager;
    } else if (!current->m_nextManager) {
        newManager->m_prevManager = m_tailManager;
        newManager->m_nextManager = 0;
        m_tailManager->m_nextManager = newManager;
        m_tailManager = newManager;
    } else {
        newManager->m_prevManager = current;
        newManager->m_nextManager = current->m_nextManager;
        current->m_nextManager->m_prevManager = newManager;
        current->m_nextManager = newManager;
    }
    return 0;
}

VA(0x004b0bf0, 0x79)
DC_ADDRESS(0x09e838, 0x60)
MAC_ADDRESS(0x0c3f00, 0xc8)
void executive::removeManager(baseManager* killManager)
{
    if (!killManager)
        return;
    killManager->close();
    baseManager* prev = killManager->m_prevManager;
    if (!prev) {
        if (m_headManager == m_tailManager) {
            m_tailManager = 0;
            m_headManager = 0;
        } else {
            m_headManager = killManager->m_nextManager;
            m_headManager->m_prevManager = 0;
        }
        killManager->clearLinks();
        return;
    }
    prev->m_nextManager = killManager->m_nextManager;
    if (!prev->m_nextManager)
        m_tailManager = prev;
    else
        prev->m_nextManager->m_prevManager = prev;
    killManager->clearLinks();
}

VA(0x004b0c70, 0x1D0)
DC_ADDRESS(0x09e898, 0x118)
MAC_ADDRESS(0x0c3fc8, 0x258)
void executive::callManager(baseManager* newManager)
{
    baseManager* saved = m_currentManager;

    try {
        if (saved == g_advManager) {
            g_advManager->setStatus(2);
            g_advManager->m_advWindow->sleepAllWidgets(1);
            g_advManager->m_heroLogoShowing = 0;
        } else {
            removeManager(m_currentManager);
        }
        try {
            addManagerOrShutDown(newManager);
            try {
                mainLoop();
            } catch (...) {
                removeManager(newManager);
                throw;
            }
            removeManager(newManager);
        } catch (...) {
            if (saved == g_advManager) {
                resumeAdventureManager();
            } else {
                addManagerOrShutDown(saved);
            }
            throw;
        }
        if (saved == g_advManager) {
            resumeAdventureManager();
            g_advManager->redrawAdvScreen(1, 0);
            kbChangeMenu(g_dfltMenu);
            g_advManager->forceNewHover();
            g_advManager->overrideBottomView(
                advManager::BOTTOM_VIEW_DEFAULT, -1);
            if (g_windowManager->m_isWaitingForFadeIn)
                g_windowManager->fadeScreen(0, 4, 0);
        } else {
            addManagerOrShutDown(saved);
        }
    } catch (...) {
        m_currentManager = saved;
        throw;
    }
    m_currentManager = saved;
}

VA(0x004b0e40, 0xF5)
DC_ADDRESS(0x09e9b0, 0x130)
MAC_ADDRESS(0x0c4220, 0x198)
void executive::mainLoop()
{
    message msg;
    int done = 0;
    int dispatch;

    if (!m_headManager)
        return;
    g_inputManager->flush();
    while (!done) {
        process1WindowsMessage();
        msg = g_inputManager->getEvent();
        dispatch = 1;
        m_currentManager = m_headManager;
        if (!m_currentManager)
            return;
        while (m_currentManager && dispatch && !done) {
            if (m_currentManager->getStatus() == 1
                    && (msg.m_id != MESSAGE_MOUSE_MOVE
                        || m_currentManager != g_windowManager)) {
                switch (m_currentManager->main(msg)) {
                    case MESSAGE_DISPATCH_CONSUME:
                        dispatch = 0;
                        break;
                    case MESSAGE_DISPATCH_FORWARD:
                        if (msg.m_id & MESSAGE_EXECUTIVE) {
                            switch (msg.m_codeX) {
                                case EXECUTIVE_COMMAND_TERMINATE_LOOP:
                                    done = 1;
                                    break;
                                case EXECUTIVE_COMMAND_RETURN_RESULT:
                                    m_dialogReturn = msg.m_extra;
                                    done = 1;
                                    break;
                                case EXECUTIVE_COMMAND_REMOVE_MANAGER:
                                    removeManager(m_currentManager);
                                    m_currentManager = 0;
                                    break;
                            }
                        }
                        break;
                }
            }
            if (m_currentManager)
                m_currentManager = m_currentManager->m_nextManager;
        }
    }
}
