    hero* getCurrHero()
    {
        if (g_currentPlayer->m_currHeroId != -1)
            return &m_heroes[g_currentPlayer->m_currHeroId];
        return 0;
    }
