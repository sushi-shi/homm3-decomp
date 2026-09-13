// basemgr.h - baseManager, the root of NWC's manager framework
// (basewin lineage; the homm2 sibling's baseManager is the template).
#ifndef HOMM3_BASEMGR_H
#define HOMM3_BASEMGR_H

#include <va.h>

class message;

// PROVEN layout (2026-08-04): Dreamcast size 56 (vptr@0, nextManager@4,
// prevManager@8, id@12, priority@16, cMgrName@20 char[32], status@52),
// corroborated store-for-store by the retail ctor (0x44d530).

class baseManager {
public:
    // STATUS_SUSPENDED is executive::CallManager's own suspend mode: the
    // arm that keeps the adventure manager on the stack writes 2 and
    // sleeps its widgets, and both restore arms write 1 back (exec.cpp,
    // 0x4b0c70). advManager::Main reads it on entry and answers a
    // suspended manager with the CONTINUE verdict - the only COMPARE
    // against the value anywhere in the tree.
    enum { STATUS_ACTIVE = 1, STATUS_SUSPENDED = 2 };

    baseManager* m_nextManager;
    baseManager* m_prevManager;
    int m_id;
    int m_priority;
    char m_mgrName[32];
    int m_status;

    baseManager();
    virtual int open(int) = 0;         // slot 0
    virtual void close() = 0;          // slot 1
    virtual int main(message& msg) = 0;  // slot 2, DC ?Main@...@@UAAHAAUmessage@@@Z
};
SIZE(baseManager, 56);

#endif  /* HOMM3_BASEMGR_H */
