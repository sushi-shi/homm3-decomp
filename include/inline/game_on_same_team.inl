    // Dreamcast's public symbol is `?OnSameTeam@game@@QBA_NHH@Z`: bool,
    VA(0x005296d0, 0x37)  // hd-crossbuild + anchor-callee x3, dc 0x1febc
    bool onSameTeam(int player1, int player2) const
    {
        if (player1 < 0 || player2 < 0)
            return 0;
        return m_mapHeader.m_teamInfo[player1] == m_mapHeader.m_teamInfo[player2];
    }
