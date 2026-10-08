// TimedEvent.h - a timed map event (Loki h3maped TimedEvent.cpp, RoE).
// Layout (0x44): _m_name @0, _m_message @0xc (SGI strings), the resource
// grant @0x18, the player mask @0x34, the computer flag @0x38, the first day
// @0x3c and the repeat interval @0x40. _m_name, s_kMaxMessageLen and the
// parameter names come from the asserts; the other member names are not
// recorded.
#ifndef HOMM3_EDITOR_TIMEDEVENT_H
#define HOMM3_EDITOR_TIMEDEVENT_H

#include <bitset>
#include <exception>
#include <string>

#include "editor/Player.h"
#include "editor/ResourceQuantities.h"

class istream;
class ostream;
class TRawIStream;
class TRawOStream;

// The calendar: "newDay < kNumDaysPerYear * 2", "newInterval <=
// kNumDaysPerYear". Every Loki object that includes this header emits 7, 4,
// 28, 12 and 336 in this order; the other four names are inferred.
const unsigned int kNumDaysPerWeek = 7;
const unsigned int kNumWeeksPerMonth = 4;
const unsigned int kNumDaysPerMonth = kNumDaysPerWeek * kNumWeeksPerMonth;
const unsigned int kNumMonthsPerYear = 12;
const unsigned int kNumDaysPerYear = kNumDaysPerMonth * kNumMonthsPerYear;

class TTimedEvent {
public:
    class TImportTextFailure : public exception {
    };

    static const unsigned int s_kMaxMessageLen = 300;

    TTimedEvent();

    const string& getName() const { return _m_name; }
    void setName(const string& newName);
    const string& getMessage() const { return _m_message; }
    void setMessage(const string& newMessage);
    const TResourceQuantities& getResourceQuantities() const { return _m_resourceQuantities; }
    void setResourceQuantities(const TResourceQuantities& newQuantities)
    {
        _m_resourceQuantities = newQuantities;
    }
    bool getBApplyToPlayer(TPlayer player) const;
    void setBApplyToPlayer(TPlayer player, bool bApply);
    bool getBApplyToComputer() const { return _m_bApplyToComputer; }
    void setBApplyToComputer(bool bApply) { _m_bApplyToComputer = bApply; }
    unsigned int getFirstOccurence() const { return _m_firstOccurence; }
    void setFirstOccurence(unsigned int newDay);
    unsigned int getSubsequentInterval() const { return _m_subsequentInterval; }
    void setSubsequentInterval(unsigned int newInterval);

    void importText(istream* pIStream);
    void exportText(ostream* pOStream) const;

    friend bool operator==(const TTimedEvent& lhs, const TTimedEvent& rhs);

private:
    string _m_name;
    string _m_message;
    TResourceQuantities _m_resourceQuantities;
    bitset<kNumPlayers> _m_bApplyToPlayer;
    bool _m_bApplyToComputer;
    unsigned int _m_firstOccurence;
    unsigned int _m_subsequentInterval;
};

// Inline: the map specifications' timed events page keeps the linkonce copy.
inline bool operator!=(const TTimedEvent& lhs, const TTimedEvent& rhs)
{
    return !(lhs == rhs);
}

TRawOStream& operator<<(TRawOStream& stream, const TTimedEvent& event);
TRawIStream& operator>>(TRawIStream& stream, TTimedEvent& event);

#endif  /* HOMM3_EDITOR_TIMEDEVENT_H */
