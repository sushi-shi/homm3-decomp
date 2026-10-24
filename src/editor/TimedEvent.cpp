// TimedEvent.cpp - the timed map event (h3maped 0x4c016c..0x4c097e; Loki
// h3maped object 34): its constructor, setters, binary form and map text.
// The Windows record adds the apply-to-human flag (maps from version 28,
// written from Shadow of Death on); a Restoration of Erathia map keeps 300
// characters of the message. The release drops Loki's asserts.
#include "editor/stdafx.h"

#include <assert.h>
#include <ctype.h>
#include <algorithm>
#include <functional>

#include "va.h"
#include "editor/GameMap.h"
#include "editor/MapEditorText.h"
#include "editor/RawStream.h"
#include "editor/TimedEvent.h"

// The reserved byte count of the map format record.
const unsigned int kNumTimedEventReserved = 16;

VA(0x004c01c8, 0x6d)
TTimedEvent::TTimedEvent()
    : _m_bApplyToPlayer(~TPlayerMask(0)), _m_bApplyToHuman(true), _m_bApplyToComputer(false), _m_firstOccurence(0),
      _m_subsequentInterval(0)
{
}

VA(0x004c0235, 0x13)
void TTimedEvent::setBApplyToPlayer(TPlayer player, bool bApply)
{
    assert(player >= 0 && player < kNumPlayers);
    _m_bApplyToPlayer.set(player, bApply);
}

VA(0x004c0248, 0xa)
void TTimedEvent::setFirstOccurence(unsigned int newDay)
{
    assert(newDay < kNumDaysPerYear * 2);
    _m_firstOccurence = newDay;
}

VA(0x004c0252, 0xa)
void TTimedEvent::setSubsequentInterval(unsigned int newInterval)
{
    assert(newInterval <= kNumDaysPerYear);
    _m_subsequentInterval = newInterval;
}

// Retail reuses the byte/short extraction slots, as readValue does. Keep the
// player mask as an addressable byte: retail tests its stack slot each iteration.
// Remaining: VC6 retains zero in EBX rather than hoisting the player-mask address.
// Naming the loop bool is flat; reading players with readValue changes the frame
// and holds the mask in a register instead of reproducing the native loop.
VA(0x004c025c, 0x150)
void TTimedEvent::read(TRawIStream* pIStream, int version)
{
    std::string text;
    *pIStream >> text;
    setName(text);
    *pIStream >> text;
    if (version <= akMapFileVersion[GAME_VERSION_ROE] && text.size() > s_kMaxMessageLen)
        text.erase(s_kMaxMessageLen);
    setMessage(text);
    *pIStream >> _m_resourceQuantities;
    unsigned char players;
    *pIStream >> players;
    for (unsigned int player = 0; player < kNumPlayers; player++)
        setBApplyToPlayer(TPlayer(player), (players & 1 << player) != 0);
    if (version >= 28) {
        setBApplyToHuman(readValue<signed char>(*pIStream) != 0);
    } else {
        setBApplyToHuman(true);
    }
    setBApplyToComputer(readValue<signed char>(*pIStream) != 0);
    setFirstOccurence(readValue<short>(*pIStream));
    setSubsequentInterval(readValue<short>(*pIStream));
    signed char aReserved[kNumTimedEventReserved];
    *pIStream >> aReserved;
}

VA(0x004c03ac, 0x303)
void TTimedEvent::importText(std::istream* pIStream, EGameVersion version)
{
    assert(pIStream != NULL);
    std::string line;
    getline(*pIStream, line);
    if (line != std::string(kNameStr) + ':')
        throw TImportTextFailure();
    getline(*pIStream, line);
    replace(line.begin(), line.end(), '\t', ' ');
    if (find_if(line.begin(), line.end(), not1(ptr_fun(isspace))) == line.end())
        throw TImportTextFailure();
    setName(line);
    getline(*pIStream, line);
    if (line != std::string(kMessageStr) + ':')
        throw TImportTextFailure();
    getline(*pIStream, line);
    if (version == GAME_VERSION_ROE && line.size() > s_kMaxMessageLen)
        line.erase(s_kMaxMessageLen);
    replace(line.begin(), line.end(), '\t', '\n');
    setMessage(line);
}

VA(0x004c06c7, 0x30)
bool TTimedEvent::getBApplyToPlayer(TPlayer player) const
{
    assert(player >= 0 && player < kNumPlayers);
    return _m_bApplyToPlayer.test(player);
}

VA(0x004c06f7, 0xda)
void TTimedEvent::write(TRawOStream* pOStream, int version) const
{
    *pOStream << _m_name << _m_message << _m_resourceQuantities;
    unsigned char players = 0;
    for (unsigned int player = 0; player < kNumPlayers; player++) {
        if (_m_bApplyToPlayer.test(player))
            players |= 1 << player;
    }
    *pOStream << players;
    if (version >= GAME_VERSION_SOD)
        *pOStream << static_cast<signed char>(_m_bApplyToHuman);
    *pOStream << static_cast<signed char>(_m_bApplyToComputer);
    *pOStream << static_cast<short>(_m_firstOccurence);
    *pOStream << static_cast<short>(_m_subsequentInterval);
    signed char aReserved[kNumTimedEventReserved];
    fill_n(aReserved, sizeof(aReserved), 0);
    *pOStream << aReserved;
}

VA(0x004c07d1, 0x113)
void TTimedEvent::exportText(std::ostream* pOStream, EGameVersion version) const
{
    assert(pOStream != NULL);
    assert(std::find_if(_m_name.begin(), _m_name.end(), std::not1(std::ptr_fun(::isspace))) != _m_name.end());
    *pOStream << kNameStr << ':' << '\n' << _m_name << '\n';
    std::string message = _m_message;
    replace(message.begin(), message.end(), '\n', '\t');
    *pOStream << kMessageStr << ':' << '\n' << message << '\n';
}

VA(0x004c08e4, 0x7a)
bool operator==(const TTimedEvent& lhs, const TTimedEvent& rhs)
{
    return lhs._m_name == rhs._m_name && lhs._m_message == rhs._m_message
           && lhs._m_resourceQuantities == rhs._m_resourceQuantities
           && lhs._m_bApplyToPlayer == rhs._m_bApplyToPlayer && lhs._m_bApplyToHuman == rhs._m_bApplyToHuman
           && lhs._m_bApplyToComputer == rhs._m_bApplyToComputer && lhs._m_firstOccurence == rhs._m_firstOccurence
           && lhs._m_subsequentInterval == rhs._m_subsequentInterval;
}
