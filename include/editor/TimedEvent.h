// TimedEvent.h - a timed map event (TimedEvent.cpp; Loki h3maped). The
// Windows record is 0x4c bytes: Loki's name, message, resource grant,
// player mask, computer flag, first day and repeat interval, with
// Complete's apply-to-human flag before the computer one. The implicit
// copy (h3maped 0x417060) copies them in that order: the strings at +0
// and +0x10, the resources at +0x20, the mask at +0x3c, the two flags at
// +0x40/+0x41 and the days at +0x44/+0x48. Member names other than
// _m_name are Loki's inferences. The string and flag setters are inline
// (the event sheets assign the members); the player, day and interval
// setters and the player test are TimedEvent.cpp's (0x4c0235..0x4c06c7).
#ifndef HOMM3_EDITOR_TIMEDEVENT_H
#define HOMM3_EDITOR_TIMEDEVENT_H

#include <string>

#include "editor/Player.h"
#include "editor/ResourceQuantities.h"

class TRawIStream;
class TRawOStream;

// The calendar (Loki's TimedEvent.h: "newDay < kNumDaysPerYear * 2").
const unsigned int kNumDaysPerWeek = 7;
const unsigned int kNumWeeksPerMonth = 4;
const unsigned int kNumDaysPerMonth = kNumDaysPerWeek * kNumWeeksPerMonth;
const unsigned int kNumMonthsPerYear = 12;
const unsigned int kNumDaysPerYear = kNumDaysPerMonth * kNumMonthsPerYear;

class TTimedEvent {
public:
    TTimedEvent();

    // The map file's record (TimedEvent.cpp, h3maped 0x4c025c).
    void read(TRawIStream* pIStream, int version);
    void write(TRawOStream* pOStream, int version) const;

    enum { s_kMaxMessageLen = 300 };

    void setName(const std::string& newName) { _m_name = newName; }
    void setMessage(const std::string& newMessage) { _m_message = newMessage; }
    void setResourceQuantities(const TResourceQuantities& newQuantities)
    {
        _m_resourceQuantities = newQuantities;
    }
    void setBApplyToPlayer(TPlayer player, bool bApply);
    void setBApplyToHuman(bool bApply) { _m_bApplyToHuman = bApply; }
    void setBApplyToComputer(bool bApply) { _m_bApplyToComputer = bApply; }
    void setFirstOccurence(unsigned int newDay);
    void setSubsequentInterval(unsigned int newInterval);

    const std::string& getName() const { return _m_name; }
    const std::string& getMessage() const { return _m_message; }
    const TResourceQuantities& getResourceQuantities() const { return _m_resourceQuantities; }
    bool getBApplyToPlayer(TPlayer player) const;
    bool getBApplyToHuman() const { return _m_bApplyToHuman; }
    bool getBApplyToComputer() const { return _m_bApplyToComputer; }
    unsigned int getFirstOccurence() const { return _m_firstOccurence; }
    unsigned int getSubsequentInterval() const { return _m_subsequentInterval; }

    friend bool operator==(const TTimedEvent& lhs, const TTimedEvent& rhs);

private:
    std::string _m_name;
    std::string _m_message;
    TResourceQuantities _m_resourceQuantities;
    TPlayerMask _m_bApplyToPlayer;
    bool _m_bApplyToHuman;
    bool _m_bApplyToComputer;
    unsigned int _m_firstOccurence;
    unsigned int _m_subsequentInterval;
};

#endif  /* HOMM3_EDITOR_TIMEDEVENT_H */
