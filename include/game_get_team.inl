VA(0x004a5960, 0x16)  // exact selected events.obj COMDAT, dc 0x37fbc
int getTeam(int playerNum) const
{
    if (playerNum < 0)
        return playerNum;
    return m_mapHeader.m_teamInfo[playerNum];
}
