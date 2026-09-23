// Dreamcast Game.h:1410 names this ordinary inline query and retains a
// selected out-of-line copy in ai_player.obj. THallWindow expands the
// same source operation to the retail town-vector lookup.
inline bool game::townAlreadyBuiltOn(int townId) const
{
    return m_towns[townId].m_builtThisTurn != 0;
}
