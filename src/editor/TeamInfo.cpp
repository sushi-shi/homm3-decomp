// TeamInfo.cpp - the alliances' player assignments (h3maped
// 0x4ba65e..0x4ba694, between SpellsDlg.cpp and TerrainPlacement.cpp; Loki
// defines them in GameMap.cpp). The file's name is not recorded. The object
// opens with the standard library's ctype<wchar_t> id initializer and
// carries no terrain masks, so it does not include the editor's common
// header. The release drops Loki's asserts; setNumTeams, a lone store,
// folds onto an identical setter elsewhere.
#include <bitset>
#include <string>

// The editor's headers spell the library's names without std:: (as the
// common header arranges).
using namespace std;

#include "va.h"
#include "editor/GameMap.h"

VA(0x004ba67a, 0xf)
void TTeamInfo::setPlayerTeam(TPlayer player, unsigned int newTeam)
{
    _m_aPlayerTeam[player] = newTeam;
}

VA(0x004ba689, 0xb)
unsigned int TTeamInfo::getPlayerTeam(TPlayer player) const
{
    return _m_aPlayerTeam[player];
}
