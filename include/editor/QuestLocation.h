// QuestLocation.h - a Seer's Hut or a Quest Guard
// (C:\Dev\Heroes 3 Exp 2\Editor\QuestLocation.cpp, by its RTTI).
//
// RTTI TQuestLocation derives virtually from TGameObject: its vtable, the
// vbptr, the owned quest (+8), the deadline (+0x10, -1 for none) and the
// three messages (+0x14), then the vtordisp before TGameObject (+0x48).
//
// Ported so far: the declarations the map needs.
#ifndef HOMM3_EDITOR_QUESTLOCATION_H
#define HOMM3_EDITOR_QUESTLOCATION_H

#include <memory>
#include <string>

#include "editor/GameObject.h"
#include "editor/Quest.h"

class TQuestLocation : public virtual TGameObject {
public:
    enum { s_kNumMessages = 3 };

    // Drops the deadline and the messages back to none (0x497046).
    virtual void resetQuestTerms();

    const TQuest* getPQuest() const { return _m_pQuest.get(); }
    // No quest, and no deadline or messages (0x496bbd).
    void clearQuest();

private:
    std::auto_ptr<TQuest> _m_pQuest;
    int _m_deadline;
    std::string _m_aMessage[s_kNumMessages];
};

#endif  /* HOMM3_EDITOR_QUESTLOCATION_H */
