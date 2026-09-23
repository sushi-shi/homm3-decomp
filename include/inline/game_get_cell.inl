// Game.h:1380. DispatchEvent expands this cell accessor; the
// out-of-line copy is ai_player.obj's, 0x42ed80.
// E:\gamedcs\game.h:1380. Retail retains this header-inline copy in
// ai_player.obj; all consumers use the same canonical body.
VA(0x0042ed80, 0x4D)  // anchor-global, dc 0x38000
inline NewmapCell* game::getCell(type_point point)
{
    return m_worldMap.cell(point.m_x, point.m_y, point.m_z);
}
