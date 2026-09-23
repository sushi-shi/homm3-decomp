// Game.h:1390 in the DC roster. Retail expands this short calendar
// accessor at every game.obj call site and retains no standalone row.
inline short game::getCurrentTurn() const
{
    return (m_month * 4 + m_week - 5) * 7 + m_day;
}
