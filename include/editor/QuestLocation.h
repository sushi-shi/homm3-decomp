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
    // Out of line: the props dialog calls the body /OPT:ICF folds onto
    // TScholar::setSpell.
    void setDeadline(int newDeadline);
    const std::string& getMessage(unsigned int i) const;
    void setMessage(unsigned int i, const std::string& newMessage);

protected:
    void read(TRawIStream* pIStream, int version);

private:
    std::auto_ptr<TQuest> _m_pQuest;
    int _m_deadline;
    // The messages' default constructor (h3maped 0x49713e, 48.77) has no EH
    // frame in retail although it calls basic_string::_Tidy out of line, and
    // retail emits it after the copy constructor (0x4970a8). VC6 drops the
    // unwind for a call to a body it has already compiled and found nothrow
    // (an empty callee defined before a caller removes the caller's frame);
    // here _Tidy is compiled after both constructors. throw() on TArray()
    // reproduces the bytes but would strip the frames retail keeps in the
    // cell and bookkeeping arrays' constructors (0x43900a, 0x433b2f).
    TArray<std::string, s_kNumMessages> _m_aMessage;
};

#endif  /* HOMM3_EDITOR_QUESTLOCATION_H */
