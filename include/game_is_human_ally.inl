// Dreamcast Game.h:1370-1371, is_human_ally (dc 0x37fd8). The two named
// calls are GetTeam followed by IsHumanTeam; this wrapper takes a player,
// not a team. No standalone Windows VA is claimed for the wrapper.
inline bool game::isHumanAlly(int playerNum) const
{
    return isHumanTeam(getTeam(playerNum));
}
