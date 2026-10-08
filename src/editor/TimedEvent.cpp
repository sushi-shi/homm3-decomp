// TimedEvent.cpp - Loki h3maped object 34: the timed event's validated
// setters, its text import/export (map editor clipboard) and its binary
// form. Assert lines come from the retail immediates.
#include <assert.h>
#include <ctype.h>
#include <algorithm>
#include <functional>
#include <iostream.h>
#include <string>

#include "terrain.h"
#include "editor/TimedEvent.h"
#include "editor/MapEditorText.h"
#include "editor/RawStream.h"

// The reserved byte counts of the map format records this file reads and
// writes. The object emits them at the end of its .rodata in this order
// (values proven there); the names are inferred.
const unsigned int kNumTimedEventReserved = 16;

TTimedEvent::TTimedEvent()
    : _m_bApplyToPlayer(~bitset<kNumPlayers>(0)),
      _m_bApplyToComputer(false),
      _m_firstOccurence(0),
      _m_subsequentInterval(0)
{
}

void TTimedEvent::setName(const string& newName)
{
#line 54
    assert(newName.empty() || std::find_if( newName.begin(), newName.end(), std::not1( std::ptr_fun( ::isspace ) ) ) != newName.end());
    assert(newName.find( '\n' ) == std::string::npos);
    assert(newName.find( '\t' ) == std::string::npos);
    _m_name = newName;
}

void TTimedEvent::setMessage(const string& newMessage)
{
#line 64
    assert(newMessage.size() <= s_kMaxMessageLen);
    assert(newMessage.find( '\t' ) == std::string::npos);
    _m_message = newMessage;
}

void TTimedEvent::setBApplyToPlayer(TPlayer player, bool bApply)
{
#line 75
    assert(player >= 0 && player < kNumPlayers);
    _m_bApplyToPlayer[player] = bApply;
}

void TTimedEvent::setFirstOccurence(unsigned int newDay)
{
#line 82
    assert(newDay < kNumDaysPerYear * 2);
    _m_firstOccurence = newDay;
}

void TTimedEvent::setSubsequentInterval(unsigned int newInterval)
{
#line 89
    assert(newInterval <= kNumDaysPerYear);
    _m_subsequentInterval = newInterval;
}

void TTimedEvent::importText(istream* pIStream)
{
#line 96
    assert(pIStream != NULL);

    string line;
    getline(*pIStream, line);
    if (line != string(kNameStr) + ':')
        throw TImportTextFailure();
    getline(*pIStream, line);
    replace(line.begin(), line.end(), '\t', ' ');
    if (find_if(line.begin(), line.end(), not1(ptr_fun(isspace))) == line.end())
        throw TImportTextFailure();
    setName(line);

    getline(*pIStream, line);
    if (line != string(kMessageStr) + ':')
        throw TImportTextFailure();
    getline(*pIStream, line);
    if (line.size() > s_kMaxMessageLen)
        line.erase(s_kMaxMessageLen);
    replace(line.begin(), line.end(), '\t', '\n');
    setMessage(line);
}

bool TTimedEvent::getBApplyToPlayer(TPlayer player) const
{
#line 124
    assert(player >= 0 && player < kNumPlayers);
    return _m_bApplyToPlayer[player];
}

void TTimedEvent::exportText(ostream* pOStream) const
{
#line 131
    assert(pOStream != NULL);
    assert(std::find_if( _m_name.begin(), _m_name.end(), std::not1( std::ptr_fun( ::isspace ) ) ) != _m_name.end());

    *pOStream << kNameStr << ':' << '\n' << _m_name << '\n';
    string message = _m_message;
    replace(message.begin(), message.end(), '\n', '\t');
    *pOStream << kMessageStr << ':' << '\n' << message << '\n';
}

TRawOStream& operator<<(TRawOStream& stream, const TTimedEvent& event)
{
    stream << event.getName() << event.getMessage() << event.getResourceQuantities();

    unsigned char players = 0;
    for (unsigned int player = 0; player < kNumPlayers; ++player) {
        if (event.getBApplyToPlayer(TPlayer(player)))
            players |= 1 << player;
    }
    stream << players;

    stream << static_cast< signed char >( event.getBApplyToComputer() )
           << static_cast< short >( event.getFirstOccurence() )
           << static_cast< short >( event.getSubsequentInterval() );

    signed char reserved[kNumTimedEventReserved];
    fill_n(reserved, sizeof(reserved), 0);
    stream << reserved;
    return stream;
}

TRawIStream& operator>>(TRawIStream& stream, TTimedEvent& event)
{
    string text;
    stream >> text;
    event.setName(text);
    stream >> text;
    if (text.size() > TTimedEvent::s_kMaxMessageLen)
        text.erase(TTimedEvent::s_kMaxMessageLen);
    event.setMessage(text);

    TResourceQuantities quantities;
    stream >> quantities;
    event.setResourceQuantities(quantities);

    unsigned char players;
    stream >> players;
    for (unsigned int player = 0; player < kNumPlayers; ++player)
        event.setBApplyToPlayer(TPlayer(player), (players >> player) & 1);

    signed char bApplyToComputer;
    short firstOccurence;
    short subsequentInterval;
    stream >> bApplyToComputer >> firstOccurence >> subsequentInterval;
    event.setBApplyToComputer(bApplyToComputer != 0);
    event.setFirstOccurence(firstOccurence);
    event.setSubsequentInterval(subsequentInterval);

    signed char reserved[kNumTimedEventReserved];
    stream >> reserved;
    return stream;
}

bool operator==(const TTimedEvent& lhs, const TTimedEvent& rhs)
{
    return lhs._m_name == rhs._m_name
        && lhs._m_message == rhs._m_message
        && lhs._m_resourceQuantities == rhs._m_resourceQuantities
        && lhs._m_bApplyToPlayer == rhs._m_bApplyToPlayer
        && lhs._m_bApplyToComputer == rhs._m_bApplyToComputer
        && lhs._m_firstOccurence == rhs._m_firstOccurence
        && lhs._m_subsequentInterval == rhs._m_subsequentInterval;
}
