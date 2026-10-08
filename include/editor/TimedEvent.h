// TimedEvent.h - a timed map event (TimedEvent.cpp; Loki h3maped). The
// Windows record is 0x4c bytes: Loki's name, message, resource grant,
// player mask, computer flag, first day and repeat interval, with
// Complete's apply-to-human flag before the computer one. The implicit
// copy (h3maped 0x417060) copies them in that order: the strings at +0
// and +0x10, the resources at +0x20, the mask at +0x3c, the two flags at
// +0x40/+0x41 and the days at +0x44/+0x48. Member names other than
// _m_name are Loki's inferences.
#ifndef HOMM3_EDITOR_TIMEDEVENT_H
#define HOMM3_EDITOR_TIMEDEVENT_H

#include <string>

#include "editor/Player.h"
#include "editor/ResourceQuantities.h"

class TTimedEvent {
public:
    const std::string& getName() const { return _m_name; }
    const std::string& getMessage() const { return _m_message; }
    const TResourceQuantities& getResourceQuantities() const { return _m_resourceQuantities; }
    bool getBApplyToHuman() const { return _m_bApplyToHuman; }
    bool getBApplyToComputer() const { return _m_bApplyToComputer; }
    unsigned int getFirstOccurence() const { return _m_firstOccurence; }
    unsigned int getSubsequentInterval() const { return _m_subsequentInterval; }

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
