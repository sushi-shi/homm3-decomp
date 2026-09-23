    VA(0x005bde40, 0x31)  // exact body + sole caller above, dc 0x2c668
    int getPrimarySkill(int skill) const
    {
        if (m_stats[skill] > 99)
            return 99;
        if (m_stats[skill] > 0)
            return m_stats[skill];
        return skill >= 2 ? 1 : 0;
    }
