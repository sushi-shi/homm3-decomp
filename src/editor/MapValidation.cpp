// MapValidation.cpp - the map validation function, the editor's "Validate
// map" report (h3maped 0x47c4f5..0x47ff04; Loki h3maped object 77). Every
// problem becomes one line of text; the victory and loss conditions are
// checked by visiting them. The Windows release formats its notes with
// CString::Format where Loki's port prints into a 512-byte buffer, and
// drops the asserts.
//
// Ported so far: the construction, the note list and the text helpers;
// the report itself and the condition visits are open (gog-editor-plan.md
// section 10).
#include "editor/stdafx.h"

#include "va.h"
#include "editor/GameMap.h"
#include "editor/MapEditorText.h"
#include "editor/MapValidation.h"
#include "editor/Town.h"

VA(0x0047c6d4, 0x33)
TMapValidationFunc::TMapValidationFunc(const TGameMap& gameMap) : _m_map(gameMap), _m_numNotes(0)
{
}

VA(0x0047d594, 0x86)
CString TMapValidationFunc::_createTownStr(const TTown& town, const TTilePoint& loc, bool bSecondLayer)
{
    CString result;
    result.Format(kObjectAtLocationFmtStr, town.getTownTypeTraits().m_pName, loc.x(), loc.y(), bSecondLayer);
    return result;
}

VA(0x0047d7e7, 0x62)
CString TMapValidationFunc::_createTeamStr(unsigned int teamNum)
{
    CString result;
    result.Format(kTeamFmtStr, teamNum + 1);
    return result;
}

VA(0x0047d849, 0x26)
void TMapValidationFunc::_addNote(const CString& note)
{
    _m_notes += note;
    _m_notes += "\r\n";
    ++_m_numNotes;
}

VA(0x0047da3e, 0x86)
bool TMapValidationFunc::_playerMayBeHuman(TPlayer whichPlayer) const
{
    const TTeamInfo& teamInfo = _m_map.getTeamInfo();
    if (teamInfo.getBHasTeams()) {
        unsigned int team = teamInfo.getPlayerTeam(whichPlayer);
        for (unsigned int player = 0; player < kNumPlayers; ++player)
            if (_m_map.isPlayerPresent(TPlayer(player)) && teamInfo.getPlayerTeam(TPlayer(player)) == team
                && _m_map.getPlayers()[player].getBHumanPlayable())
                return true;
        return false;
    }
    return _m_map.getPlayers()[whichPlayer].getBHumanPlayable();
}
