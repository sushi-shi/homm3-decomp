// QuestLocation.h - a Seer's Hut or a Quest Guard
// (C:\Dev\Heroes 3 Exp 2\Editor\QuestLocation.cpp, by its RTTI).
//
// RTTI TQuestLocation derives virtually from TGameObject: its vtable, the
// vbptr, the owned quest (+8), the deadline (+0x10, -1 for none) and the
// three messages (+0x14), then the vtordisp before TGameObject (+0x48).
// A location without a quest keeps no deadline or messages.
#ifndef HOMM3_EDITOR_QUESTLOCATION_H
#define HOMM3_EDITOR_QUESTLOCATION_H

#include <memory>
#include <string>

#include "editor/Array.h"
#include "editor/GameObject.h"
#include "editor/Quest.h"

class TQuestLocation : public virtual TGameObject {
public:
    enum { s_kNumMessages = 3 };

    TQuestLocation(const TObjectType& objType);
    TQuestLocation(const TObjectType& objType, TRawIStream* pIStream, int version);
    TQuestLocation(const TQuestLocation& other);

    virtual void importText(std::istream* pIStream, EGameVersion version);
    virtual void write(TRawOStream* pOStream, int version) const;
    virtual bool isCustomized() const;
    virtual bool hasText() const;
    virtual void exportText(std::ostream* pOStream, EGameVersion version) const;

    // Drops the deadline and the messages back to none.
    virtual void resetQuestTerms();

    const TQuest* getPQuest() const { return _m_pQuest.get(); }
    void setQuest(std::auto_ptr<TQuest> pQuest);
    // No quest, and no deadline or messages.
    void clearQuest();
    int getDeadline() const { return _m_deadline; }
    void setDeadline(int newDeadline) { _m_deadline = newDeadline; }
    const std::string& getMessage(unsigned int i) const;
    void setMessage(unsigned int i, const std::string& newMessage);

protected:
    void read(TRawIStream* pIStream, int version);

private:
    std::auto_ptr<TQuest> _m_pQuest;
    int _m_deadline;
    TArray<std::string, s_kNumMessages> _m_aMessage;
};

#endif  /* HOMM3_EDITOR_QUESTLOCATION_H */
