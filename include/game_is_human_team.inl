// Dreamcast Game.h:839-850, IsHumanTeam (dc 0x37f64): reject a negative
// team, scan its eight player slots, and call gpGame->IsHuman on a member.
// Windows 0x42b9e0 and Mac 0:0x2d3e4 retain this same guarded scan.
// This is distinct from is_human_ally at dc 0x37fd8, which takes a player
// number and calls IsHumanTeam(GetTeam(player_number)).
VA(0x0042b9e0, 0x45)  // guarded team scan + named IsHuman callee, dc 0x37f64
bool isHumanTeam(int teamNum) const
{
    if (teamNum >= 0) {
        for (int player = 0; player < 8; ++player) {
            if (m_mapHeader.m_teamInfo[player] == teamNum
                && g_game->isHuman(player))
                return true;
        }
    }
    return false;
}
